# Installer



# Compilation and build 


## I. Install libwebrtc on your system

Get the installer for libwebrtc version 62 provided by CoSMo company.  
[Windows installer](https://s3-ap-southeast-1.amazonaws.com/webrtc-installer/version62/libwebrtc-62.252-x64-Release-rtti-msvc2015.exe)  
[MAC installer](https://s3-ap-southeast-1.amazonaws.com/webrtc-installer/version62/libwebrtc-62.000-x64-Release-rtti.dmg)


#### Compiler

Be careful to make sure correct Tools / Windows 10 SDK Installed.

* Windows 7 x64 or later,
* Visual Studio 2015 update 3

Make sure that you install the following components for Visual Studio:
* section Programming Languages: check Visual C++, which will select all the three sub-categories Common Tools, MFC and Windows XP Support
* section Windows and Web Development / Universal Windows Apps Development Tools: check Tools (1.4.1) and Windows 10 SDK (**10.0.14393**)

Additional components to install:
* [Windows 10 SDK][w10sdk] with **Debugging Tools for Windows** or
  [Windows Driver Kit 10][wdk10] installed in the same Windows 10 SDK
  installation directory.

### c. Mac

TBD 

### d. Linux

TBD


## II. Install OpenSSL

Latest release version available: openssl-1.1.0g (2 November 2017)
https://www.openssl.org/

### a. Windows

IMPORTANT: Add path to OpenSSL DLLs to your PATH environment variable.
C:\Program Files\OpenSSL\bin


# III. Compiling obs-studio

## Compilation Windows

- install OpenSSL in 64 bits mode: start a VS2015 x64 Native Tools Command Prompt in administrator mode
```
 $ "c:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
 $ perl Configure VC-WIN64A
 $ nmake
 $ nmake test
 $ nmake install
```

- install QT (5.9)
- install WIX Toolset http://wixtoolset.org/
- download OBS studio pre compiled [dependencies](https://obsproject.com/downloads/dependencies2015.zip) and extract them (e.g. at the root of the cloned dir)
- start a command line, and setup VS2015 environment variables to get compilations in 64 bits mode:
```
 $ "c:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
```
- configure the project

```
git submodule update --init
mkdir MY_BUILD
cd MY_BUILD
cmake
  -DDepsPath=<full_path_to_dependencies>\win64
  -DQTDIR=<qt_install_full_path>
  -DOPENSSL_ROOT_DIR=<openssl_install_full_path>
  -DCMAKE_BUILD_TYPE=<DEBUG|RELEASE|RelWithDebInfo>
  -G "NMake Makefiles"
  ..
```

example:

```
cmake .. -DQTDIR=C:\Qt\5.9.1\msvc2015_64
         -DDepsPath=C:\cosmo\dependencies2015\win64
         -DOPENSSL_ROOT_DIR="C:\Program Files\OpenSSL"
         -DCMAKE_BUILD_TYPE=Release
         -G "NMake Makefiles"
```
- compile the project
```
nmake
```

## Packaging on Windows

Inside MY_BUILD just run the command :

```
cpack
```

## USAGE

### Configure JANUS

https://github.com/meetecho/janus-gateway.
Configure a JANUS server using the video room plugin with websocket protocol activate. You can use their html demo.


### OBS settings

Launch OBS, go to settings, select the stream tab and change the URL to your JANUS : wss://janus1.cosmosoftware.io  
Put 1234 for the room.

After you can start streaming, OBS will connect to the default room and if you have any suscriber present in the room, you will see the OBS stream.
https://clientweb.cosmosoftware.io/janusweb/videoroomtest.html

## Docs (in progress..)

### WebsocketClientImpl.cpp

That is where we receive the message and define the protocol API with JANUS. 

### WebRTCStream.cpp inside the folder /plugin/output/

There is where we handle the message, get and setup the offer and answer from libwebrtc. Also, is there we initial/remove the stream.


