- [Windows](#windows)
  * [Prerequisite](#prerequisite)
    + [Compiler](#compiler)
    + [OpenSSL](#openssl)
  * [Compilation](#compilation)
  * [Installation](#installation)
  * [Packaging](#packaging)
- [Linux](#linux)
  * [Prerequisite](#prerequisite-1)
  * [Compilation](#compilation-1)
  * [Installation](#installation-1)
  * [Packaging](#packaging-1)
- [Mac](#mac)
  * [Prerequisite](#prerequisite-2)
  * [Compilation](#compilation-2)
  * [Installation](#installation-2)
  * [Packaging](#packaging-2)
- [USAGE](#usage)
  * [Configure JANUS](#configure-janus)



## Windows

### Prerequisite

Get the Windows installer for libwebrtc version 65 provided by CoSMo:
[Windows installer](https://drive.google.com/file/d/1EM0OXGS0Xm61m5Nhb-2nNNJo1JpbBZnB/view?usp=sharing)

#### Compiler

Make sure the correct tools / Windows 10 SDK are installed.

- Windows 7 x64 or later
- Visual Studio 2017

Make sure the following components are installed for Visual Studio:

- Section Programming Languages: check Visual C++, which will select all the three sub-categories Common Tools, MFC and Windows XP Support
- Section Windows and Web Development / Universal Windows Apps Development Tools: check Tools (1.4.1) and Windows 10 SDK (**10.0.14393**)

Additional components to install:

- [Windows 10 SDK][w10sdk] with **Debugging Tools for Windows** or [Windows Driver Kit 10][wdk10] installed in the same Windows 10 SDK installation directory.

#### OpenSSL

Install [OpenSSL 1.1.0g](https://www.openssl.org/source/old/1.1.0/openssl-1.1.0g.tar.gz)

/!\ IMPORTANT /!\: Add OpenSSL directory containing the DLLs to your PATH environment variable.
e.g. C:\Program Files\OpenSSL\bin

### Compilation

- Install OpenSSL in 64 bits mode: start a VS2017 x64 Native Tools Command Prompt in administrator mode
```
 $ "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
 $ perl Configure VC-WIN64A
 $ nmake
 $ nmake test
 $ nmake install
```

- Install QT (5.10.1)
- Install WIX Toolset http://wixtoolset.org/
- Download OBS-studio pre compiled [dependencies](https://obsproject.com/downloads/dependencies2015.zip) and extract them (e.g. at the root of the cloned dir)
- Start a command line, and setup VS2017 environment variables to get the compilations in 64 bits mode:
```
 $ "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
```
- Configure the project

```
git clone --recursive https://github.com/CoSMoSoftware/OBS-studio-webrtc.git
cd OBS-studio-webrtc
mkdir build
cd build
cmake
  -DDepsPath=<full_path_to_dependencies>\win64
  -DQTDIR=<qt_install_full_path>
  -DCMAKE_BUILD_TYPE=<Debug|Release|RelWithDebInfo>
  -G "NMake Makefiles"
  ..
```

e.g.

```
cmake -DQTDIR=C:\Qt\5.10.1\msvc2017_64 
		-DDepsPath=C:\Dependencies\win64 
		-DCMAKE_BUILD_TYPE=Release 
		-G "NMake Makefiles" ..
```
- Compile the project
```
nmake
```

### Installation
The executable can be found at this location:

```
OBS-studio-webrtc\build\rundir\Release\bin\64bit
```

### Packaging 

```
cd build
cpack
```

## Linux

### Prerequisite

* Get the Linux installer for libwebrtc version 65 provided by CoSMo:
  [Linux installer](https://drive.google.com/open?id=1374iQ7b53LdQeUZZDF41hrmKY64MIzz0) 

1. Launch the script:

```
chmod +x libwebrtc-65.0-x64-OpenSSL-release
./libwebrtc-65.0-x64-OpenSSL-release
cd libwebrtc-65.0-x64-OpenSSL-release
```

2. Copy the folders into /usr/local: 

```
sudo cp -r cmake /usr/local
sudo cp -r include /usr/local
sudo cp -r lib /usr/local
```

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

* Set up the environment variables:
```
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/local/lib:/usr/lib/obs-plugins:/usr/local/bin:/usr/local/lib/obs-plugins:/usr/bin/obs"
```

* Recompile libcurl with openssl 1.1.0g:

OBS-studio on Linux uses the libcurl from the system which uses openssl 1.0.0 leading to conflict with our version of openssl. To solve this issue, it is necessary to recompile libcurl with the right version of OpenSSL.

1. Download openssl 1.1.0g then extract it: 

```
https://www.openssl.org/source/old/1.1.0/openssl-1.1.0g.tar.gz
tar -zxvf openssl-1.1.0g.tar.gz
```

2. Compile and installation:
```
cd openssl-1.1.0g
./config <options ...> --openssldir=/usr/local/ssl
make -j4
sudo make install
```

3. Download the latest version of libcurl then extract it:
```
https://curl.haxx.se/download.html
tar -zxvf curl-7.60.0.tar.gz
```

4. Compile with OpenSSL and install:
```
cd curl-version
./configure --with-ssl=/usr/local/ssl
make -j4
make install
```
### Compilation

```
git clone --recursive https://github.com/CoSMoSoftware/OBS-studio-webrtc.git
cd OBS-studio-webrtc
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
```

### Installation

```
sudo make install
```

### Packaging

```
cd build
cpack
```

## Mac

### Prerequisite

* Get the MacOS installer for libwebrtc version 65 provided by CoSMo:
  [MacOS installer](https://drive.google.com/file/d/11qnWRfHczOCZyXvmiVlyHa2T7ykKmbOi/view?usp=sharing)


* Install ffmpeg and Qt using Homebrew or Macport:

```
brew install ffmpeg
brew install qt
```

* Set Qt directory:

```
export QTDIR=/usr/local/Cellar/qt/<qt-version>
```

### Compilation

```
git clone --recursive https://github.com/CoSMoSoftware/OBS-studio-webrtc.git
cd OBS-studio-webrtc
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<installation_location> ..
make -j4
```

### Installation

```
sudo make install
```

### Packaging

```
cpack -G "productbuild"
```

## USAGE

### Configure JANUS

https://github.com/meetecho/janus-gateway.
Configure a JANUS server using the video room plugin with websocket protocol activated. You can use their html demo.


### OBS settings

Launch OBS, go to settings, select the stream tab and change the URL to your JANUS.

e.g.
```
wss://janus1.cosmosoftware.io  
Put 1234 for the room.
```

OBS will connect to the default room. If you have any subscriber present in the room, the stream will show up.
https://clientweb.cosmosoftware.io/janusweb/videoroomtest.html
