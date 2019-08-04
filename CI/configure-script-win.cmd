rem ---------------------------------------------------------------------
set build_config=Release
mkdir build64
cd build64
cmake ^
  -G "Visual Studio 15 2017 Win64" ^
  -DENABLE_SCRIPTING=OFF ^
  -DCOPIED_DEPENDENCIES=false -DCOPY_DEPENDENCIES=true ^
  -DBUILD_CAPTIONS=false ^
  -DCOMPILE_D3D12_HOOK=true ^
  -DBUILD_BROWSER=false -DCEF_ROOT_DIR=%CEF_64% ^
  -Dlibwebrtc_DIR=%libwebrtcPath% ^
  -DOPENSSL_ROOT_DIR=%opensslPath% ^
  ..