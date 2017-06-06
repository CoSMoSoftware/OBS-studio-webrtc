# I. Install libwebrtc on your system

This is to be replaced once the installers are available. [TODO: David + Alex]

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

#### Env. Variables

make sure to have the environement variable: set

### c. Mac

Make sure to have a recent version of XCode installed.
Install OpenSSL: "sudo port install openssl"


### d. Linux

TBD

## B. Compiling libwebrtc

https://github.com/agouaillard/libwebrtc-cmake

Clone the repository, initialize the submodules if `depot_tools` is not
installed on your system or not defined inside your `PATH` environment variable.
Create an output directory, browse inside it, then run CMake.

```
$ git clone --recursive https://github.com/agouaillard/libwebrtc-cmake.git
$ cd libwebrtc-cmake
$ mkdir MY_BUILD
$ cd MY_BUILD
$ cmake ..
```

## C. Installing libwebrtc

You need to run the "install" target
Depending on your build environement this could be one of the following
- make install
- nmake install

# II. Compiling obs-studio

## Windows

- install OpenSSL
- install QT
- download OBS studio pre compiled [dependencies](https://obsproject.com/downloads/dependencies2015.zip) and extract them (e.g. at the root of the cloned dir)
- configure the project

```
mkdir MY_BUILD
cd MY_BUILD
cmake
  -DDepsPath=<full_path_to_dependencies>\win64
  -DQTDIR=<qt_install_full_path>
  -DOPENSSL_ROOT_DIR=<openssl_install_full_path>
  ..
```

  - example: -DDepsPath=C:\DEVEL\obs-studio\win64\
  - example: -DQTDIR=C:\Qt\5.6\msvc2015_64

- compile the project
```
nmake
```

## Mac [TODO Ben + alex]

- Install at least ffmpeg and openSSL
- configure the project
```
mkdir MY_BUILD
cd MY_BUILD
cmake
  -DQTDIR=<qt_install_full_path>
  ..
```
  - example: -DQTDIR=/Users/cosmo/Qt/5.6/clang_64/lib/cmake/Qt5
- compile the project (Less than 3mn on 2016 MBA)
```
make
```

### current errors

a.

```
Undefined symbols for architecture x86_64:
  "typeinfo for webrtc::videocapturemodule::VideoCaptureImpl", referenced from:
      typeinfo for rtc::RefCountedObject<webrtc::videocapturemodule::VideoCaptureImpl> in WebRTCStream.cpp.o
  "typeinfo for cricket::WebRtcVideoCapturer", referenced from:
      typeinfo for VideoCapturer in WebRTCStream.cpp.o
ld: symbol(s) not found for architecture x86_64
```
Alex: from experience, this is typically the sign of a library built with the no-rtti flag.

https://bugs.chromium.org/p/webrtc/issues/detail?id=6468

b. after rtti compilation

```
[ 85%] Linking CXX shared module obs-outputs.so
cd /Users/cosmo/DEVEL/OBS-studio-webrtc/MYBUILD/plugins/obs-outputs && /opt/local/bin/cmake -E cmake_link_script CMakeFiles/obs-outputs.dir/link.txt --verbose=1
/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++  -Wall -Wextra -Wno-unused-function -Werror-implicit-function-declaration -Wno-missing-field-initializers  -std=gnu++11 -fno-strict-aliasing -stdlib=libc++ -O2 -g -DNDEBUG -bundle -Wl,-headerpad_max_install_names  -o obs-outputs.so CMakeFiles/obs-outputs.dir/obs-outputs.c.o CMakeFiles/obs-outputs.dir/rtmp-stream.cpp.o CMakeFiles/obs-outputs.dir/rtmp-windows.c.o CMakeFiles/obs-outputs.dir/AudioDeviceModuleWrapper.cpp.o CMakeFiles/obs-outputs.dir/VideoCapturer.cpp.o CMakeFiles/obs-outputs.dir/WebRTCStream.cpp.o CMakeFiles/obs-outputs.dir/net-if.c.o -Wl,-rpath,@loader_path/ -Wl,-rpath,@executable_path/ -lwebrtc -lmetrics_default -lfield_trial_default -framework Foundation -framework AVFoundation -framework CoreMedia -framework CoreGraphics -framework CoreVideo -framework CoreAudio -framework AudioToolbox ../../libobs/libobs.0.dylib ../websocket-client/libwebsocketclient.dylib /opt/local/lib/libssl.dylib /opt/local/lib/libcrypto.dylib 
Undefined symbols for architecture x86_64:
  "webrtc::videocapturemodule::VideoCaptureImpl::RegisterCaptureDataCallback(rtc::VideoSinkInterface<webrtc::VideoFrame>*)", referenced from:
      vtable for VideoCapture in WebRTCStream.cpp.o
  "webrtc::videocapturemodule::VideoCaptureImpl::VideoCaptureImpl()", referenced from:
      WebRTCStream::WebRTCStream(obs_output*) in WebRTCStream.cpp.o
  "webrtc::VideoCaptureFactory::CreateDeviceInfo()", referenced from:
      WebRTCStream::CreateDeviceInfo() in WebRTCStream.cpp.o
      non-virtual thunk to WebRTCStream::CreateDeviceInfo() in WebRTCStream.cpp.o
  "cricket::WebRtcVideoCapturer::OnFrame(webrtc::VideoFrame const&)", referenced from:
      vtable for VideoCapturer in WebRTCStream.cpp.o
  "non-virtual thunk to cricket::WebRtcVideoCapturer::OnFrame(webrtc::VideoFrame const&)", referenced from:
      vtable for VideoCapturer in WebRTCStream.cpp.o
  "non-virtual thunk to cricket::VideoCapturer::RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>*)", referenced from:
      vtable for VideoCapturer in WebRTCStream.cpp.o
  "non-virtual thunk to cricket::VideoCapturer::AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>*, rtc::VideoSinkWants const&)", referenced from:
      vtable for VideoCapturer in WebRTCStream.cpp.o
ld: symbol(s) not found for architecture x86_64
clang: error: linker command failed with exit code 1 (use -v to see invocation)
make[2]: *** [plugins/obs-outputs/obs-outputs.so] Error 1
make[1]: *** [plugins/obs-outputs/CMakeFiles/obs-outputs.dir/all] Error 2
make: *** [all] Error 2
```
