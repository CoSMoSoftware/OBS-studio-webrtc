# Installer



# Compilation and build 


## I. Install libwebrtc on your system

Get the installer for libwebrtc version 61 provided by Comsmo company.
https://cosmosoftware.atlassian.net/wiki/spaces/CL/pages/22052880/Installers


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


# II. Compiling obs-studio

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
cmake .. -DQTDIR=C:\Qt\5.9.1\msvc2015_64 -DDepsPath=C:\Users\XXX\Downloads\dependencies2015\win64
         -DDepsPath=C:\cosmo\dependencies2015\win64
         -DOPENSSL_ROOT_DIR=C:\cosmo\openssl-1.1.0g
         -DCMAKE_BUILD_TYPE=RELEASE
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

For now there is no room management: OBS will connect automatically to the default room 1234.
Launch OBS, go to settings, select the stream tab and change the URL to your JANUS : example wss://janus.cosmosoftware.io.
Note: Put random string on Stream key, Username, and Password (is not used yet). 

After you can start streaming, OBS will connect to the default room and if you have any suscriber present in the room, you will see the OBS stream.

## Docs (in progress..)

### WebsocketClientImpl.cpp

That is where we receive the message and define the protocol API with JANUS. 

### WebRTCStream.cpp inside the folder /plugin/output/

There is where we handle the message, get and setup the offer and answer from libwebrtc. Also, is there we initial/remove the stream.


