REM install WiX Toolset
choco -y install wixtoolset

cd build64
cpack -G WIX

move *.msi ..\build