# I. Install libwebrtc on your system

This is to be replaced once the installers are available. [TODO: david]

## A. pre-requesites / Build environement

### a. depot_tools

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

Make sure to have a recent version of XCode installed

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
  -DOPENSSL_ROOT_DIR=<openssl_install_full_path> ..
```

  - example: -DDepsPath=C:\DEVEL\obs-studio\win64\
  - example: -DQTDIR=C:\Qt\5.6\msvc2015_64 ..

- compile the project
```
nmake
```
