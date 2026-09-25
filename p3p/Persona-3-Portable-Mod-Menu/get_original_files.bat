set datadir=E:\Roms\PSP (+)\cpk_extracted\data
set pakpack=E:\_PersonaModding\PAKPack\PAKPack.exe

if exist original rd /s /q original
mkdir original

echo f | xcopy /y "%datadir%\init_free.bin" "original\init_free\init_free.bin"
echo f | xcopy /y "%datadir%\field2d\p07_02.bin" "original\abin\p07_02.bin"

"%pakpack%" unpack "original\init_free\init_free.bin" temp
"%pakpack%" unpack "original\abin\p07_02.bin" temp

for /f %%a in (filelists\field_ids.txt) do (
	echo f | xcopy /y "%datadir%\field2d\bg\b%%a.abin" "original\abin\b%%a.abin"
	"%pakpack%" unpack "original\abin\b%%a.abin" temp
)

move /y "temp\field\script\field.bf" "original\init_free\field.bf"
for /f %%a in (filelists\field_ids.txt) do (
	move /y "temp\h%%a.bf" "original\h%%a.bf"
)

rd /s /q temp