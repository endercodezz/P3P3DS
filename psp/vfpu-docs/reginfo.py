
# VFPU Registers are tricky, here some utilities to handle them
# Provide support for printing register names, generating registers for a
# given mode, etc

# This is quite over-complicated, due to how the register bits are used
# within the hardware to select registers. But the register naming in the
# assembler is odd, so perhaps it is more readable. Just run the script
# to see the output (and it should actually make sense!)

def genvect(regtype, vsize):
	if regtype == "single":
		for x in range(128):
			yield (x, [x])
	elif regtype == "vector":
		if vsize == 1:
			for x in range(128):
				yield (x, [x])
		elif vsize == 2:
			for i in range(32):    # C000...C730
				yield (i, [i, i+32])
			for i in range(32):    # R000...R703
				nreg = (i & ~3) + ((i & 3) * 32)
				yield (i+32, [nreg, nreg+1])
			for i in range(32):    # C002...C732
				yield (i+64, [i+64, i+96])
			for i in range(32):    # R020...R723
				nreg = (i & ~3) + ((i & 3) * 32)
				yield (i+96, [nreg+2, nreg+3])
		elif vsize == 3:
			for i in range(32):    # C000...C730
				yield (i, [i, i+32, i+64])
			for i in range(32):    # R000...R703
				nreg = (i & ~3) + ((i & 3) * 32)
				yield (i+32, [nreg, nreg+1, nreg+2])
			for i in range(32):    # C002...C732
				yield (i+64, [i+32, i+64, i+96])
			for i in range(32):    # R020...R723
				nreg = (i & ~3) + ((i & 3) * 32)
				yield (i+96, [nreg+1, nreg+2, nreg+3])
		elif vsize == 4:
			for i in range(32):    # C000...C730
				yield (i, [i, i+32, i+64, i+96])
			for i in range(32):    # R000...R703
				nreg = (i & ~3) + ((i & 3) * 32)
				yield (i+32, [nreg, nreg+1, nreg+2, nreg+3])
	elif regtype == "matrix":
		if vsize == 2:
			for i in range(0, 32, 2):    # M000, M020, M100...
				yield (i, [i, i+1, i+32, i+33])      # S000, S010, S001, S011
			for i in range(64, 96, 2):    # M002, M022, M102...
				yield (i, [i, i+1, i+32, i+33])      # S002, S012, S003, S013
			for i in range(0, 32, 4):
				yield (i+32, [i  , i+32, i+1, i+33])     # EX00: S000, S001, S010, S011
				yield (i+96, [i+2, i+34, i+3, i+35])     # EX20: S020, S021, S030, S031
				yield (i+34, [i+64, i+96, i+65, i+97])   # EX02: S002, S012, S003, S013
				yield (i+98, [i+66, i+98, i+67, i+99])   # EX22: S022, S023, S032, S033
		elif vsize == 3:
			for i in range(0, 32, 4):
				yield (i,    [i+ 0, i+ 1, i+ 2, i+32, i+33, i+34, i+64, i+65, i+66])   # MX00: S000, S010, S020, S001, S011, S021, S002, S012, S022
				yield (i+ 1, [i+ 1, i+ 2, i+ 3, i+33, i+34, i+35, i+65, i+66, i+67])   # MX10: S010, S020, S030, S011, S021, S031, S012, S022, S032
				yield (i+32, [i+ 0, i+32, i+64, i+ 1, i+33, i+65, i+ 2, i+34, i+66])   # EX00: S000, S001, S002, S010, S011, S012, S020, S021, S022
				yield (i+33, [i+32, i+64, i+96, i+33, i+65, i+97, i+34, i+66, i+98])   # EX01: S010, S020, S030, S011, S021, S031, S012, S022, S032
				yield (i+64, [i+32, i+33, i+34, i+64, i+65, i+66, i+96, i+97, i+98])   # MX01: S001, S011, S021, S002, S012, S022, S003, S013, S023
				yield (i+65, [i+33, i+34, i+35, i+65, i+66, i+67, i+97, i+98, i+99])   # MX11: S011, S021, S031, S012, S022, S032, S013, S023, S033
				yield (i+96, [i+ 1, i+33, i+65, i+ 2, i+34, i+66, i+ 3, i+35, i+67])   # EX10: S001, S011, S021, S002, S012, S022, S003, S013, S023
				yield (i+97, [i+33, i+65, i+97, i+34, i+66, i+98, i+35, i+67, i+99])   # EX11: S011, S012, S013, S021, S022, S023, S032, S032, S033
		elif vsize == 4:
			for i in range(0, 32, 4):
				yield (i, [x + y*32 + i for y in range(4) for x in range(4)])
			for i in range(0, 32, 4):
				yield (i+32, [x + y*32 + i for x in range(4) for y in range(4)])

def regpname(regtype, vsize, regnum):
	vfpu_sreg_names = [   # Single elements
		"S000",  "S010",  "S020",  "S030",  "S100",  "S110",  "S120",  "S130",
		"S200",  "S210",  "S220",  "S230",  "S300",  "S310",  "S320",  "S330",
		"S400",  "S410",  "S420",  "S430",  "S500",  "S510",  "S520",  "S530",
		"S600",  "S610",  "S620",  "S630",  "S700",  "S710",  "S720",  "S730",
		"S001",  "S011",  "S021",  "S031",  "S101",  "S111",  "S121",  "S131",
		"S201",  "S211",  "S221",  "S231",  "S301",  "S311",  "S321",  "S331",
		"S401",  "S411",  "S421",  "S431",  "S501",  "S511",  "S521",  "S531",
		"S601",  "S611",  "S621",  "S631",  "S701",  "S711",  "S721",  "S731",
		"S002",  "S012",  "S022",  "S032",  "S102",  "S112",  "S122",  "S132",
		"S202",  "S212",  "S222",  "S232",  "S302",  "S312",  "S322",  "S332",
		"S402",  "S412",  "S422",  "S432",  "S502",  "S512",  "S522",  "S532",
		"S602",  "S612",  "S622",  "S632",  "S702",  "S712",  "S722",  "S732",
		"S003",  "S013",  "S023",  "S033",  "S103",  "S113",  "S123",  "S133",
		"S203",  "S213",  "S223",  "S233",  "S303",  "S313",  "S323",  "S333",
		"S403",  "S413",  "S423",  "S433",  "S503",  "S513",  "S523",  "S533",
		"S603",  "S613",  "S623",  "S633",  "S703",  "S713",  "S723",  "S733"
	]
	vfpu_vpreg_names = [  # Pair elements
		"C000",  "C010",  "C020",  "C030",  "C100",  "C110",  "C120",  "C130",
		"C200",  "C210",  "C220",  "C230",  "C300",  "C310",  "C320",  "C330",
		"C400",  "C410",  "C420",  "C430",  "C500",  "C510",  "C520",  "C530",
		"C600",  "C610",  "C620",  "C630",  "C700",  "C710",  "C720",  "C730",
		"R000",  "R001",  "R002",  "R003",  "R100",  "R101",  "R102",  "R103",
		"R200",  "R201",  "R202",  "R203",  "R300",  "R301",  "R302",  "R303",
		"R400",  "R401",  "R402",  "R403",  "R500",  "R501",  "R502",  "R503",
		"R600",  "R601",  "R602",  "R603",  "R700",  "R701",  "R702",  "R703",
		"C002",  "C012",  "C022",  "C032",  "C102",  "C112",  "C122",  "C132",
		"C202",  "C212",  "C222",  "C232",  "C302",  "C312",  "C322",  "C332",
		"C402",  "C412",  "C422",  "C432",  "C502",  "C512",  "C522",  "C532",
		"C602",  "C612",  "C622",  "C632",  "C702",  "C712",  "C722",  "C732",
		"R020",  "R021",  "R022",  "R023",  "R120",  "R121",  "R122",  "R123",
		"R220",  "R221",  "R222",  "R223",  "R320",  "R321",  "R322",  "R323",
		"R420",  "R421",  "R422",  "R423",  "R520",  "R521",  "R522",  "R523",
		"R620",  "R621",  "R622",  "R623",  "R720",  "R721",  "R722",  "R723"
	]
	vfpu_vtreg_names = [  # Triplet elements
		"C000",  "C010",  "C020",  "C030",  "C100",  "C110",  "C120",  "C130",
		"C200",  "C210",  "C220",  "C230",  "C300",  "C310",  "C320",  "C330",
		"C400",  "C410",  "C420",  "C430",  "C500",  "C510",  "C520",  "C530",
		"C600",  "C610",  "C620",  "C630",  "C700",  "C710",  "C720",  "C730",
		"R000",  "R001",  "R002",  "R003",  "R100",  "R101",  "R102",  "R103",
		"R200",  "R201",  "R202",  "R203",  "R300",  "R301",  "R302",  "R303",
		"R400",  "R401",  "R402",  "R403",  "R500",  "R501",  "R502",  "R503",
		"R600",  "R601",  "R602",  "R603",  "R700",  "R701",  "R702",  "R703",
		"C001",  "C011",  "C021",  "C031",  "C101",  "C111",  "C121",  "C131",
		"C201",  "C211",  "C221",  "C231",  "C301",  "C311",  "C321",  "C331",
		"C401",  "C411",  "C421",  "C431",  "C501",  "C511",  "C521",  "C531",
		"C601",  "C611",  "C621",  "C631",  "C701",  "C711",  "C721",  "C731",
		"R010",  "R011",  "R012",  "R013",  "R110",  "R111",  "R112",  "R113",
		"R210",  "R211",  "R212",  "R213",  "R310",  "R311",  "R312",  "R313",
		"R410",  "R411",  "R412",  "R413",  "R510",  "R511",  "R512",  "R513",
		"R610",  "R611",  "R612",  "R613",  "R710",  "R711",  "R712",  "R713"
	]
	vfpu_vqreg_names = [  # Quad elements
		"C000",  "C010",  "C020",  "C030",  "C100",  "C110",  "C120",  "C130",
		"C200",  "C210",  "C220",  "C230",  "C300",  "C310",  "C320",  "C330",
		"C400",  "C410",  "C420",  "C430",  "C500",  "C510",  "C520",  "C530",
		"C600",  "C610",  "C620",  "C630",  "C700",  "C710",  "C720",  "C730",
		"R000",  "R001",  "R002",  "R003",  "R100",  "R101",  "R102",  "R103",
		"R200",  "R201",  "R202",  "R203",  "R300",  "R301",  "R302",  "R303",
		"R400",  "R401",  "R402",  "R403",  "R500",  "R501",  "R502",  "R503",
		"R600",  "R601",  "R602",  "R603",  "R700",  "R701",  "R702",  "R703",
	]

	vfpu_mpreg_names = [  # 2x2 matrix
		"M000",  "",  "M020",  "",  "M100",  "",  "M120",  "",
		"M200",  "",  "M220",  "",  "M300",  "",  "M320",  "",
		"M400",  "",  "M420",  "",  "M500",  "",  "M520",  "",
		"M600",  "",  "M620",  "",  "M700",  "",  "M720",  "",
		"E000",  "",  "E002",  "",  "E100",  "",  "E102",  "",
		"E200",  "",  "E202",  "",  "E300",  "",  "E302",  "",
		"E400",  "",  "E402",  "",  "E500",  "",  "E502",  "",
		"E600",  "",  "E602",  "",  "E700",  "",  "E702",  "",
		"M002",  "",  "M022",  "",  "M102",  "",  "M122",  "",
		"M202",  "",  "M222",  "",  "M302",  "",  "M322",  "",
		"M402",  "",  "M422",  "",  "M502",  "",  "M522",  "",
		"M602",  "",  "M622",  "",  "M702",  "",  "M722",  "",
		"E020",  "",  "E022",  "",  "E120",  "",  "E122",  "",
		"E220",  "",  "E222",  "",  "E320",  "",  "E322",  "",
		"E420",  "",  "E422",  "",  "E520",  "",  "E522",  "",
		"E620",  "",  "E622",  "",  "E720",  "",  "E722",  ""
	]

	vfpu_mtreg_names = [  # 3x3 matrix
		"M000",  "M010",  "",  "",  "M100",  "M110",  "",  "",
		"M200",  "M210",  "",  "",  "M300",  "M310",  "",  "",
		"M400",  "M410",  "",  "",  "M500",  "M510",  "",  "",
		"M600",  "M610",  "",  "",  "M700",  "M710",  "",  "",
		"E000",  "E001",  "",  "",  "E100",  "E101",  "",  "",
		"E200",  "E201",  "",  "",  "E300",  "E301",  "",  "",
		"E400",  "E401",  "",  "",  "E500",  "E501",  "",  "",
		"E600",  "E601",  "",  "",  "E700",  "E701",  "",  "",
		"M001",  "M011",  "",  "",  "M101",  "M111",  "",  "",
		"M201",  "M211",  "",  "",  "M301",  "M311",  "",  "",
		"M401",  "M411",  "",  "",  "M501",  "M511",  "",  "",
		"M601",  "M611",  "",  "",  "M701",  "M711",  "",  "",
		"E010",  "E011",  "",  "",  "E110",  "E111",  "",  "",
		"E210",  "E211",  "",  "",  "E310",  "E311",  "",  "",
		"E410",  "E411",  "",  "",  "E510",  "E511",  "",  "",
		"E610",  "E611",  "",  "",  "E710",  "E711",  "",  ""
	]

	vfpu_mqreg_names = [  # 4x4 matrix
		"M000",  "",  "",  "",  "M100",  "",  "",  "",
		"M200",  "",  "",  "",  "M300",  "",  "",  "",
		"M400",  "",  "",  "",  "M500",  "",  "",  "",
		"M600",  "",  "",  "",  "M700",  "",  "",  "",
		"E000",  "",  "",  "",  "E100",  "",  "",  "",
		"E200",  "",  "",  "",  "E300",  "",  "",  "",
		"E400",  "",  "",  "",  "E500",  "",  "",  "",
		"E600",  "",  "",  "",  "E700",  "",  "",  "",
	]

	if regtype == "single":
		return vfpu_sreg_names[regnum]
	elif regtype == "vector":
		if vsize == 1:
			return vfpu_sreg_names[regnum]
		elif vsize == 2:
			return vfpu_vpreg_names[regnum]
		elif vsize == 3:
			return vfpu_vtreg_names[regnum]
		elif vsize == 4:
			return vfpu_vqreg_names[regnum]
	elif regtype == "matrix":
		if vsize == 2:
			return vfpu_mpreg_names[regnum]
		elif vsize == 3:
			return vfpu_mtreg_names[regnum]
		elif vsize == 4:
			return vfpu_mqreg_names[regnum]
	return None

def regcount(regtype, vsize):
	if regtype == "single":
		return 128
	elif regtype == "vector":
		if vsize == 4:
			return 64
		return 128
	elif regtype == "matrix":
		if vsize == 4:
			return 16
		return 64
	return None

if __name__ == '__main__':
	# Prints all registers and what single elements they cover
	for mode, elemsize in [("single", 1),
	                       ("vector", 2), ("vector", 3), ("vector", 4),
	                       ("matrix", 2), ("matrix", 3), ("matrix", 4)]:
		for elem in genvect(mode, elemsize):
			rl = ",".join(regpname("single", 1, x) for x in elem[1])
			print("Reg #%d (%s): %s" % (elem[0], regpname(mode, elemsize, elem[0]), rl))


