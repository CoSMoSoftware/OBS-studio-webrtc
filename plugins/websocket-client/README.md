# CMake (for sergio)

## fully automated (command line MSVC)
- start "cmd", if possible with MSVC environement, possibly as administrator
- mkdir MYBUILD-NMAKE
- cd MBUILD-NMAKE
- cmake -G "NMake Makefiles" ..
- nmake

## IDE (MSVC with GUI)
- start "cmd", if possible with MSVC environment, possibly as administrator
- mkdir MYBUILD-MSVC
- cd MBUILD-MSVC
- cmake ..
- msbuild

#Dependencies:
- websocketpp : header only, copied in repo
- json.hpp : header only, copied in repo
- ASIO: http://think-async.com/ dep from websocketpp
- OpenSSL 1.0.x: Windows binaries here https://indy.fulgan.com/SSL/ dependency from ASIO

#Aditional definitions
WEBSOCKETCLIENT_EXPORTS
