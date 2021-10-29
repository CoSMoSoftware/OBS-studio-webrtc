REM install WiX Toolset
choco -y install wixtoolset

PATH

echo ========================
echo $Env:Path
echo ========================

REM Parameter %1 = vendor name
cd build64_%1
'$Env:Systemroot\Program Files\CMake\bin\cpack.exe' -G WIX

mkdir ..\build_%1
dir
move *.msi ..\build_%1
cd ..
