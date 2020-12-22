REM install WiX Toolset
choco -y install wixtoolset

REM Parameter %1 = vendor name
cd build64_%1
cpack -G WIX

move *.msi ..\build_%1
cd ..
