![FTL-SDK](https://dl.dropboxusercontent.com/u/20701844/tachyon.png) FTL-SDK

FTL-SDK is a cross platform SDK written in C to enable sending audio/video to mixer using our FTL service

### Support Platforms

 - [x] Windows
 - [x] iOS/OSX
 - [x] Android/Linux

### Requirements

Due to the nature of WebRTC the following audio and video formats are required

#### Audio
 - Opus at 48khz

#### video
 - H.264 (most profiles are supported including baseline, main and high)
 - for the lowest delay B Frames should be disabled

### Building

Prerequisites:

 - cmake 2.8.0 or later
 - gcc or Visual Studio 2015 Community (or better) and many other tool chains

getting the code:

```
git clone https://github.com/mixer/ftl-sdk
cd ftl-sdk
git submodule update --init
```

#### Linux/Android/OSX/etc command line instructions
in the directory containing CMakeList.txt (ftl-sdk/) create a folder
```
mkdir build
cd build
cmake ..
make 
```

#### Windows (specifically for Visual Studio 2015) command line instructions 
in the directory containing CMakeList.txt (ftl-sdk) create a folder
```
mkdir build
cd build
cmake -G "Visual Studio 14 2015 Win64" ..
msbuild /p:Configuration=Release ALL_BUILD.vcxproj OR open libftl.sln in Visual Studio
```
*ftl_app.exe will be placed in build/release directory*

### Running Test Application

download the following test files:

 - sintel.h264: https://www.dropbox.com/s/ruijibs0lgjnq51/sintel.h264
 - sintel.opus: https://www.dropbox.com/s/s2r6lggopt9ftw5/sintel.opus

In the directory containing ftl_app

```
ftl_app -i auto -s "<mixer stream key>" -v path\to\sintel.h264 -a path\to\sintel.opus -f 24
```
