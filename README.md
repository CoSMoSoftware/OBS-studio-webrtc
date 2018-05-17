# Installer


# Compilation and build 


## I. Install libwebrtc on your system

Get the installer for libwebrtc version 65 provided by CoSMo:  
[Windows installer](https://drive.google.com/file/d/1EM0OXGS0Xm61m5Nhb-2nNNJo1JpbBZnB/view?usp=sharing)  
[Linux installer](https://drive.google.com/open?id=1374iQ7b53LdQeUZZDF41hrmKY64MIzz0)  

#### Compiler

Be careful to make sure correct Tools / Windows 10 SDK Installed.

* Windows 7 x64 or later,
* Visual Studio 2017

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

* Set up the build environment:
```
sudo apt-get install build-essential pkg-config cmake git-core checkinstall
```

* Get the packages:
```
 sudo apt-get install libx11-dev libgl1-mesa-dev libvlc-dev libpulse-dev libxcomposite-dev \
          libxinerama-dev libv4l-dev libudev-dev libfreetype6-dev \
          libfontconfig-dev qtbase5-dev libqt5x11extras5-dev libx264-dev \
          libxcb-xinerama0-dev libxcb-shm0-dev libjack-jackd2-dev libcurl4-openssl-dev
```

* Get ffmpeg:
```
  sudo apt-get install zlib1g-dev yasm
  git clone --depth 1 git://source.ffmpeg.org/ffmpeg.git
  cd ffmpeg
  ./configure --enable-shared --prefix=/usr
  make -j4
  sudo checkinstall --pkgname=FFmpeg --fstrans=no --backup=no \
          --pkgversion="$(date +%Y%m%d)-git" --deldoc=yes
```

* Get clang:
```
sudo apt-get install clang libc++-dev
```

* Set up environment variables:
```
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/local/lib:/usr/lib/obs-plugins:/usr/local/bin:/usr/local/lib/obs-plugins:/usr/bin/obs"
```

* Recompile libcurl with openssl 1.1.0g:
By default, OBS-studio on linux uses the system's libcurl which uses openssl 1.0.0 leading to conflict with our version of openssl. To solve this issue, it is necessary to recompile libcurl.
```
Download openssl 1.1.0g then extract it: 
https://www.openssl.org/source/old/1.1.0/openssl-1.1.0g.tar.gz

Compile and installation:
./config <options ...> --openssldir=/usr/local/ssl
make
sudo make install

Download the latest version of libcurl then extract it:
https://curl.haxx.se/download.html

Compile with openssl and installation:
./configure --with-ssl=/usr/local/ssl
make
make install

```


## II. Install OpenSSL

Latest release version available: openssl-1.1.0g (2 November 2017)
https://www.openssl.org/

### a. Windows

IMPORTANT: Add path to OpenSSL DLLs to your PATH environment variable.
C:\Program Files\OpenSSL\bin


# III. Compiling obs-studio

## Compilation Windows

- install OpenSSL in 64 bits mode: start a VS2017 x64 Native Tools Command Prompt in administrator mode
```
 $ "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
 $ perl Configure VC-WIN64A
 $ nmake
 $ nmake test
 $ nmake install
```

- install QT (5.10.1)
- install WIX Toolset http://wixtoolset.org/
- download OBS studio pre compiled [dependencies](https://obsproject.com/downloads/dependencies2015.zip) and extract them (e.g. at the root of the cloned dir)
- start a command line, and setup VS2017 environment variables to get compilations in 64 bits mode:
```
 $ "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
```
- configure the project

```
git submodule update --init
mkdir build
cd build
cmake
  -DDepsPath=<full_path_to_dependencies>\win64
  -DQTDIR=<qt_install_full_path>
  -DCMAKE_BUILD_TYPE=<Debug|Release|RelWithDebInfo>
  -G "NMake Makefiles"
  ..
```

example:

```
cmake -DQTDIR=C:\Qt\5.10.1\msvc2017_64 
		-DDepsPath=C:\Dependencies\win64 
		-DCMAKE_BUILD_TYPE=Release 
		-G "NMake Makefiles" ..
```
- compile the project
```
nmake
```

## Compilation Linux

Build and install OBS:

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
sudo make install
```

## Packaging on Windows and Linux

Inside build just run the command :

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


