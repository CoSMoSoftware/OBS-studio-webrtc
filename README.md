
# OBS-studio WebRTC

This project is a fork of OBS-studio with support for WebRTC. WebRTC supports comes from the inclusion of the open source implementation from webrtc.org used (at least in part) by chrome, firefox, and safari.

The implementation is in the "plugins / obs-outputs" directory, co-existing with the flash and FTL output plugins. The WebRTCStream files contain the high-level implementation, while the xxxx-stream files contain the specific implementation for a given service.

For the time being the following services and sites are supported:
- Janus "videoRoom" server
- Millicast.com PaaS
- meedoze "SFU" server

This is mainly a community project with a best effort service. The maintainers of this repository are all based in Singapore, so please take the timezone into account. If you file a ticket at 2am, you won't get an answer for at least 7 hours.

Do not forget to share the love with the original OBS-Studio project and its fantastic team [there](https://obsproject.com/blog/progress-report-february-2019).

## Binaries

Pre-built and tested Binaries are available [here](https://github.com/CoSMoSoftware/OBS-studio-webrtc/releases).

## Windows linux or macos

### Prerequisite: libwebrtc

You do NOT depend on any external libwebrtc package one might provide. However, compilation of libwebrtc is an under documented process with strong ties to dark magic. If you think the compilation of the code from webrtc.org is too difficult, and it makes depending projects NOT easily reproducible, please let Google know, Their bug tracker is there:
https://bugs.chromium.org/p/webrtc/issues/list

Be aware of the [limitations when it comes to H.264 support](https://github.com/CoSMoSoftware/OBS-studio-webrtc/issues/33).

Be aware that most of the stream settings do not apply yet to webrtc streams. We are working to make the setting UI better, and will revisit this part later.

#### do it yourself

The current recommended way of compiling libwebrtc is documented on the [official webrtc.org website](https://webrtc.org/native-code/development/prerequisite-sw/). The recommended version of MSVC 2017 is older that the one you would download today, but we can confirm that the latest one (MSVC 2019) works (as of february 2019). There is no specific problem on linux or mac.

This repository most recent code is based on the stable branch 84 of webrtc.org.

The code calls "find_package(libwebrtc)" which supposes that you either wrote your own Findlibwebrtc.cmake module, or have an installation of libwebrtc with a cmake configuration file. The corresponding cmake documentation is [here](https://cmake.org/cmake/help/v3.13/manual/cmake-developer.7.html#find-modules). An alternative is to remove the find_package and add all the include path, lib path, definition, etc by yourself. Expect to be done in 2 weeks to 3 months.

### Compilation, Installation and Packaging

Follow the original compilation, Installation and packaging guide https://github.com/obsproject/obs-studio

In addition, please read https://github.com/CoSMoSoftware/OBS-studio-webrtc/wiki/Dev-process to adapt the build process when required.
