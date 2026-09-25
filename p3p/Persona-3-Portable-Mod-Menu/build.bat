set compiler=path\to\AtlusScriptCompiler.exe
set pakpack=path\to\PAKPack.exe
set output_path=Output

set pack=yes

if not exist original call get_original_files.bat
if exist "%output_path%\data" rd /s /q "%output_path%\data"

"%compiler%" .\hook\field_bf\field.flow -Compile -OutFormat V1 -Library P3P -Encoding P3 -Out "%output_path%\data\init_free\field\script\field.bf" -Hook
for /f %%a in (filelists\field_ids.txt) do (
	"%compiler%" .\hook\h%%a.flow -Compile -OutFormat V1 -Library P3P -Encoding P3 -Out "%output_path%\data\field2d\bg\b%%a\h%%a.bf" -Hook
)
echo f | xcopy /y "%output_path%\data\field2d\bg\b07_02\h07_02.bf" "%output_path%\data\field2d\p07_02\bg\b07_02\h07_02.bf"

if /I "%pack%" == "yes" (
	for /f %%a in (filelists\field_ids.txt) do (
		"%pakpack%" replace "original\abin\b%%a.abin" h%%a.bf "%output_path%\data\field2d\bg\b%%a\h%%a.bf" "%output_path%\data\field2d\bg\b%%a.abin"
		rd /s /q "%output_path%\data\field2d\bg\b%%a"
	)
	"%pakpack%" replace "original\abin\p07_02.bin" bg/b07_02.abin "%output_path%\data\field2d\bg\b07_02.abin" "%output_path%\data\field2d\p07_02.bin"
	"%pakpack%" replace "original\init_free\init_free.bin" field/script/field.bf "%output_path%\data\init_free\field\script\field.bf" "%output_path%\data\init_free.bin"
	rd /s /q "%output_path%\data\init_free"
	rd /s /q "%output_path%\data\field2d\p07_02"
)
