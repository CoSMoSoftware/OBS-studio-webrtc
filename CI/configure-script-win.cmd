rem ---------------------------------------------------------------------
cmake ^
  -G "Visual Studio 15 2017 Win64" ^
  -DENABLE_SCRIPTING=OFF ^
  -DCOPIED_DEPENDENCIES=false -DCOPY_DEPENDENCIES=true ^
  -DBUILD_CAPTIONS=false ^
  -DCOMPILE_D3D12_HOOK=true ^
  -DBUILD_BROWSER=false ^
  -Dlibwebrtc_DIR=%libwebrtcPath% ^
  -DOPENSSL_ROOT_DIR=%opensslPath% ^
  -DQt5_DIR=%QTDIR64%\lib\cmake\Qt5 ^
  -DCMAKE_BUILD_TYPE=%build_config% ^
  -DOBS_WEBRTC_VENDOR_NAME=Millicast ^
  -DOBS_VERSION_OVERRIDE=23.2.0 ^
  ..
