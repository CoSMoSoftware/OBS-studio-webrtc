
# OBS-studio WebRTC

This project is a fork of OBS-studio with an implementation of WebRTC.

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

### Compilation

Follow the original compilation guide https://github.com/obsproject/obs-studio

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

## Usage with a Janus server

### Configure JANUS

https://github.com/meetecho/janus-gateway.
Configure a JANUS server using the video room plugin with websocket protocol activated. You can use their html demo.

### OBS settings

Launch OBS, go to settings, select the stream tab and change the URL to point to your JANUS server.
