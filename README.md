# Compilation and build 


# I. Install libwebrtc on your system

Choose an installer for libwebrtc version 61 or later

https://cosmosoftware.atlassian.net/wiki/spaces/CL/pages/22052880/Installers

## A. pre-requesites / Build environement

long read: https://webrtc.org/native-code/development/prerequisite-sw/

### a. depot_tools

https://www.chromium.org/developers/how-tos/install-depot-tools

### b. Windows

#### Compiler

Be careful to make sure correct Tools / Windows 10 SDK Installed.

* Windows 7 x64 or later,
* Visual Studio 2015 update 3

Make sure that you install the following components:
  
* Visual C++, which will select three sub-categories including MFC
* Universal Windows Apps Development Tools
* Tools (1.4.1) and Windows 10 SDK (**10.0.14393**)
* [Windows 10 SDK][w10sdk] with **Debugging Tools for Windows** or
  [Windows Driver Kit 10][wdk10] installed in the same Windows 10 SDK
  installation directory.

### c. Mac

Make sure to have a recent version of XCode installed.

Install OpenSSL: "sudo port install openssl"

### d. Linux

TBD


# II. Compiling obs-studio

## Windows

- install OpenSSL
- install QT
- download OBS studio pre compiled [dependencies](https://obsproject.com/downloads/dependencies2015.zip) and extract them (e.g. at the root of the cloned dir)
- configure the project

```
git submodule update --init
mkdir MY_BUILD
cd MY_BUILD
cmake
  -DDepsPath=<full_path_to_dependencies>\win64
  -DQTDIR=<qt_install_full_path>
  -DOPENSSL_ROOT_DIR=<openssl_install_full_path>
  ..
```

example:
```
cmake .. -DQTDIR=C:\Qt\5.9.1\msvc2015_64 -DDepsPath=C:\Users\XXX\Downloads\dependencies2015\win64 -G "NMake Makefiles"
- compile the project
```
nmake
```


