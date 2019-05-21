
# OBS-studio WebRTC

This project is a fork of OBS-studio with support for WebRTC. WebRTC supports comes from the inclusion of the open source implementation from webrtc.org used (at least in part) by chrome, firefox, and safari.

The implementation is in the "plugins / obs-outputs" directory, co-existing with the flash and FTL output plugins. The WebRTCStream files contain the high-level implementation, while the xxxx-stream files contain the specific implementation for a given service.

For the time being the following services and sites are supported:
- Janus
- Millicast.com
- Spank.live (SpankChain)

This is mainly a community project with a best effort service. The maintainers of this repository are all based in Singapore, so please take the timezone into account. If you file a ticket at 2am, you won't get an answer for at least 7 hours.

If you need more commercial support, or custom developement,for webrtc, please contact [CoSMo software](http://www.cosmosoftware.io/contact.html). A patreon page and other means of sharing the maintenance and support cost for the generic webrtc part will be made available soon.

Do not forget to share the love with the original OBS-Studio project and its fantastic team [there](https://obsproject.com/blog/progress-report-february-2019).

# Table of content

- [Binaries](#binaries)
- [Usage](#usage)
  * [Configure JANUS](#configure-janus)
  * [OBS settings](#obs-settings)

## Binaries

Pre-built and tested Binaries are available [here](https://github.com/CoSMoSoftware/OBS-studio-webrtc/releases).

## Windows linux or macos

### Prerequisite: libwebrtc

You do NOT depend on any external libwebrtc package one might provide. However, compilation of libwebrtc is an under documented process with strong ties to dark magic. If you think the compilation of the code from webrtc.org is too difficult, and it makes depending projects NOT easily reproducible, please let Google know, Their bug tracker is there:
https://bugs.chromium.org/p/webrtc/issues/list

Be aware of the [limitations when it comes to H.264 support](https://github.com/CoSMoSoftware/OBS-studio-webrtc/issues/33).

Be aware that the settings do not apply yet to webrtc streams. We are working with the OBS team to make the setting UI better, and will revisit this part later.

#### do it yourself

The current recommended way of compiling libwebrtc is documented on the [official webrtc.org website](https://webrtc.org/native-code/development/prerequisite-sw/). The recommended version of MSVC 2017 is older that the one you would download today, but we can confirm that the latest one works (as of february 2019). There is no specific problem on linux or mac.

This repository code is based on the stable branch 65 of webrtc.org. It should be updated to 73 next, by the end of April 2019.

OpenSSL build of libwebrtc might not be needed.  The flags are already in place in google's code to get that build. More specifically, if you look at the source code, you will see the SSL flags which needs to be passed to the GN GEN command that allow to use an external SSL library, if you need to (line 43): https://webrtc.googlesource.com/src/+/master/rtc_base/BUILD.gn
  * [We have pushed the modification to google's repository](https://bugs.chromium.org/p/webrtc/issues/detail?id=10160) and are waiting for a merge to happen.
  * [We are working](https://github.com/CoSMoSoftware/OBS-studio-webrtc/issues/63) on removing the need for OpenSSL altogether to make the compilation more straightforward. We are expecting to be done by the end of april 2019.

The code calls "find_package(libwebrtc)" which supposes that you either wrote your own Findlibwebrtc.cmake module, or have an installation of libwebrtc with a cmake configuration file. The corresponding cmake documentation is [here](https://cmake.org/cmake/help/v3.13/manual/cmake-developer.7.html#find-modules). An alternative is to remove the find_package and add all the include path, lib path, definition, etc by yourself. Expect to be done in 2 weeks to 3 months.

#### CoSMo's libwebrtc Community Packages

By may 2019, [free libwebrtc community packages](https://www.cosmosoftware.io/products/libwebrtc-packages) should be available from CoSMo software. They will make compiling OBS-webrtc-Studio a breeze. We strongly suggest to wait for them.

### Compilation, Installation and Packaging

Follow the original compilation, Installation and packaging guide https://github.com/obsproject/obs-studio

In addition, please read https://github.com/CoSMoSoftware/OBS-studio-webrtc/wiki/Dev-process to adapt the build process when required.

## Usage with a Janus server

### Configure JANUS

https://github.com/meetecho/janus-gateway.

Configure a JANUS server using the video room plugin with **websocket secure protocol** enabled. (you can enable TLS inside the config file janus.transport.websockets.cfg or e.g directly use Nginx).

For now OBS-Webrtc **support only connection through wss**. 
You can directly use their test webpage videoroomtest.html to receive the stream from OBS-webrtc.

### OBS settings

Launch OBS, go to settings, select the stream tab and change the URL to point to your JANUS server (wss://xxx). if using videoroomtest.html, the default "Server room" value is 1234.
