# CMake (for sergio)

## fully automated (command line MSVC)
- start "cmd", if possible with MSVC environement, possibly as administrator
- mkdir MYBUILD
- cd MBUILD
- cmake -G "NMake Makefiles" ..
- nmake

## IDE (MSVC with GUI)
- start "cmd", if possible with MSVC environement, possibly as administrator
- mkdir MYBUILD
- cd MBUILD
- cmake -G "NMake Makefiles" ..
- nmake

## TODO
- make it compile

#Dependencies:
- websocketpp : header only, copied in repo
- json.hpp : header only, copied in repo
- ASIO: http://think-async.com/ dep from websocketpp
- OpenSSL 1.0.x: Windows binaries here https://indy.fulgan.com/SSL/ dependency from ASIO

#Aditional definitions
WEBSOCKETCLIENT_EXPORTS
