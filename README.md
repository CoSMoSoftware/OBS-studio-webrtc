
# OBS-studio WebRTC

This project is a fork of OBS-studio with support for WebRTC. WebRTC supports comes from the inclusion of the open source implementation from webrtc.org used (at least in part) by chrome, firefox, and safari.

The implementation is in the "plugins / obs-outputs" directory, co-existing with the flash and FTL output plugins. The WebRTCStream files contain the high-level implementation, while the xxxx-stream files contain the specific implementation for a given service.

For the time being the following services and sites are supported:
- Janus
- Millicast.com
- Spank.live (SpankChain)

This is mainly a community project with a best effort service.

If you need more support, or specific developement, please contact [CoSMo software](http://www.cosmosoftware.io/contact.html).

# Table of content

- [Binaries](#binaries)
- [Usage](#usage)
  * [Configure JANUS](#configure-janus)
  * [OBS settings](#obs-settings)

## Binaries

Pre-built and tested Binaries are available [here](https://github.com/CoSMoSoftware/OBS-studio-webrtc/releases).

## Windows linux or macos

### Prerequisite

Get the libwebrtc code from webrtc.org and compile it for your target OS and architecture.

### Compilation, Installation and Packaging

Follow the original compilation, Installation and packaging guide https://github.com/obsproject/obs-studio

## Usage with a Janus server

### Configure JANUS

https://github.com/meetecho/janus-gateway.
Configure a JANUS server using the video room plugin with websocket protocol activated. You can use their html demo.

### OBS settings

Launch OBS, go to settings, select the stream tab and change the URL to point to your JANUS server.
