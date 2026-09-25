#!/usr/bin/env python3

# Copyright 2022 David Guillen Fandos <david@davidgf.net>
# Simplifies C code and re-formats it (for doc generation purposes)

import tatsu, json


# Parses the AST, and performs simplifications like constant propagation
# and constant operations.
def simplify_code(code):
	cgrammar = '''
		start = lside:lsizeexpr op:'=' rside:expr $ ;
		expr = ternexpr;

		ternexpr     = cond:ternexpr op:"?" rest:expr ":" resf:expr | boolorexpr;
		boolorexpr   = l:boolorexpr op:"||" r:boolandexpr | boolandexpr;
		boolandexpr  = l:boolandexpr op:"&&" r:bitworexpr | bitworexpr;
		bitworexpr   = l:bitworexpr op:"|" r:bitwxorexpr | bitwxorexpr;
		bitwxorexpr  = l:bitwxorexpr op:"^" r:bitwandexpr | bitwandexpr;
		bitwandexpr  = l:bitwandexpr op:"&" r:compequexpr | compequexpr;
		compequexpr  = l:compequexpr op:"==" r:cmpunequxpr | l:compequexpr op:"!=" cmpunequxpr | cmpunequxpr;
		cmpunequxpr  = l:cmpunequxpr op:"<" r:shiftexpr | l:cmpunequxpr op:">" r:shiftexpr |
		               l:cmpunequxpr op:"<=" r:shiftexpr | l:cmpunequxpr op:">=" r:shiftexpr | shiftexpr;
		shiftexpr    = l:shiftexpr op:">>" r:additionexpr | l:shiftexpr op:"<<" r:additionexpr | additionexpr;
		additionexpr = l:additionexpr op:'+' r:multexpr | l:additionexpr op:'-' r:multexpr | multexpr;
		multexpr     = l:multexpr op:'*' r:unaryexpr | l:multexpr op:'/' r:unaryexpr |
		               l:multexpr op:'%' r:unaryexpr | unaryexpr;
		unaryexpr    =  op:'-' e:simpleexpr | op:'+' e:simpleexpr | op:'~' e:simpleexpr | op:'!' e:simpleexpr | simpleexpr;
		simpleexpr = '(' paren:expr ')' | fncall | unit;
		fncall = funcname:fnname op:'(' args:fnargs ')' ;
		fnargs = | ','.{ expr }+ | expr ;
		fnname  = /[A-Za-z][A-Za-z0-9]+/ ;
		unit = consthex | constf32 | constint | vararray | variable ;
		vararray = var:varname '[' idx:expr ']' ;
		variable  = var:varname ;
		varname  = /[A-Za-z][A-Za-z_0-9]+/ ;
		constint = vali:/\d+/ ;
		consthex = valh:/0x[0-9A-Fa-f]+/ ;
		constf32 = valf:/\d+.\d+[f]/ ;
		lsizeexpr = vararray | variable;
	'''
	unops = {
		'+': lambda x:  x,
		'-': lambda x: -x,
		'~': lambda x: ~x,
		'!': lambda x: 0 if x else 1,
	}
	binops = {
		'&': lambda x, y: x & y,
		'|': lambda x, y: x | y,
		'^': lambda x, y: x ^ y,
		'+': lambda x, y: x + y,
		'-': lambda x, y: x - y,
		'*': lambda x, y: x * y,
		'/': lambda x, y: x // y,
		'%': lambda x, y: x % y,
		'<': lambda x, y: 1 if x < y else 0,
		'>': lambda x, y: 1 if x > y else 0,
		'<=': lambda x, y: 1 if x <= y else 0,
		'>=': lambda x, y: 1 if x >= y else 0,
		'==': lambda x, y: 1 if x == y else 0,
		'!=': lambda x, y: 1 if x != y else 0,
		'>>': lambda x, y: x >> y,
		'<<': lambda x, y: x << y,
		'&&': lambda x, y: (x != 0) and (y != 0),
		'||': lambda x, y: (x != 0) or (y != 0),
	}
	def _nonp(fn, *args):
		if any(arg is None for arg in args):
			return None
		return fn(*args)

	def _serialize(elem):
		if "var" in elem:
			if "idx" in elem:
				return "%s[%s]" % (elem.var, _serialize(elem.idx))
			return elem.var
		if "paren" in elem:
			return "(%s)" % _serialize(elem.paren)
		if "cond" in elem:
			return "%s ? %s : %s" % (_serialize(elem.cond), _serialize(elem.rest), _serialize(elem.resf))
		if elem.op == "=":
			return "%s = %s" % (_serialize(elem.lside), _serialize(elem.rside))
		elif elem.op == "(":
			return "%s(%s)" % (elem.funcname, ", ".join(_serialize(x) for x in elem.args))
		elif "e" in elem and elem.op in unops:
			return "%s%s" % (elem.op, _serialize(elem.e))
		elif elem.op in binops:
			return "%s %s %s" % (_serialize(elem.l), elem.op, _serialize(elem.r))
		elif "vali" in elem:
			return elem.vali
		elif "valh" in elem:
			return elem.valh
		elif "valf" in elem:
			return elem.valf
		return str(elem)

	def _eval(elem):
		if "vali" in elem:
			return int(elem.vali)
		elif "valf" in elem:
			return float(elem.valf.strip("f"))
		elif "valh" in elem:
			return int(elem.valh, 16)
		elif "paren" in elem:
			return _eval(elem.paren)
		elif "cond" in elem:
			condval = _eval(elem.cond)
			if condval is None:
				return None
			if condval:
				return _eval(elem.rest)
			return _eval(elem.resf)
		elif "e" in elem and elem.op in unops:
			ev = _eval(elem.e)
			return _nonp(unops[elem.op], ev)
		elif elem.op in binops:
			l, r = _eval(elem.l), _eval(elem.r)
			return _nonp(binops[elem.op], l, r,)
		return None

	def _makecnst(v):
		if isinstance(v, int):
			return tatsu.ast.AST({"vali": str(v)})
		if isinstance(v, float):
			return tatsu.ast.AST({"valf": "%gf" % v})
		return None
		
	def _simplify(elem):
		if "vali" in elem or "valf" in elem or "valh" in elem:
			return elem
		v = _eval(elem)
		if v is not None:
			return _makecnst(v)

		if "var" in elem:
			if "idx" in elem:
				return tatsu.ast.AST({"var": elem.var, "idx": _simplify(elem.idx)})
			return elem
		if "paren" in elem:
			return tatsu.ast.AST({"paren": _simplify(elem.paren)})
		if "cond" in elem:
			# Can simplify part of it, if subcondition is known
			condval = _eval(elem.cond)
			if condval is None:
				return tatsu.ast.AST({"cond": _simplify(elem.cond), "rest": _simplify(elem.rest), "resf": _simplify(elem.resf)})

			if condval:
				return _simplify(elem.rest)
			return _simplify(elem.resf)
		if elem.op == "=":
			return tatsu.ast.AST({"op": "=", "lside": _simplify(elem.lside), "rside": _simplify(elem.rside)})
		elif elem.op == "(":
			return tatsu.ast.AST({"op": "(", "funcname": elem.funcname, "args": [_simplify(x) for x in elem.args]})
		elif "e" in elem and elem.op in unops:
			return tatsu.ast.AST({"op": elem.op, "e": _simplify(elem.e)})
		elif elem.op in binops:
			l, r = _simplify(elem.l), _simplify(elem.r)
			if elem.op in ["-", "+", ">>", "<<"] and _eval(r) == 0:
				return l
			return tatsu.ast.AST({"op": elem.op, "l": l, "r": r})
		return elem
	try:
		ast = tatsu.parse(cgrammar, code)
	except:
		return None

	ast = _simplify(ast)
	return _serialize(ast)

