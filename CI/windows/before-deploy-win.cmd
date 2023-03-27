REM install WiX Toolset
REM choco -y install wixtoolset

REM Parameter %1 = vendor name
cd build64_%1
"C:\Program Files\CMake\bin\cpack.exe" -C %BUILD_TYPE% -G NSIS

mkdir ..\build_%1
move *.exe ..\build_%1
cd ..
