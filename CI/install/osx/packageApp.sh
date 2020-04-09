rm -rf ./EBS.app

mkdir EBS.app
mkdir EBS.app/Contents
mkdir EBS.app/Contents/MacOS
mkdir EBS.app/Contents/PlugIns
mkdir EBS.app/Contents/Resources

BUILD_CONFIG=RELEASE
cp -r rundir/$BUILD_CONFIG/bin/ ./EBS.app/Contents/MacOS
cp -r rundir/$BUILD_CONFIG/data ./EBS.app/Contents/Resources
cp ../CI/install/osx/obs.icns ./EBS.app/Contents/Resources
cp -r rundir/$BUILD_CONFIG/obs-plugins/ ./EBS.app/Contents/PlugIns
cp ../CI/install/osx/Info.plist ./EBS.app/Contents

../CI/install/osx/dylibBundler -b -cd -d ./EBS.app/Contents/Frameworks -p @executable_path/../Frameworks/ \
-s ./EBS.app/Contents/MacOS \
-s /usr/local/opt/mbedtls/lib/ \
-x ./EBS.app/Contents/PlugIns/coreaudio-encoder.so \
-x ./EBS.app/Contents/PlugIns/decklink-ouput-ui.so \
-x ./EBS.app/Contents/PlugIns/frontend-tools.so \
-x ./EBS.app/Contents/PlugIns/image-source.so \
-x ./EBS.app/Contents/PlugIns/linux-jack.so \
-x ./EBS.app/Contents/PlugIns/mac-avcapture.so \
-x ./EBS.app/Contents/PlugIns/mac-capture.so \
-x ./EBS.app/Contents/PlugIns/mac-decklink.so \
-x ./EBS.app/Contents/PlugIns/mac-syphon.so \
-x ./EBS.app/Contents/PlugIns/mac-vth264.so \
-x ./EBS.app/Contents/PlugIns/obs-ffmpeg.so \
-x ./EBS.app/Contents/PlugIns/obs-filters.so \
-x ./EBS.app/Contents/PlugIns/obs-transitions.so \
-x ./EBS.app/Contents/PlugIns/obs-vst.so \
-x ./EBS.app/Contents/PlugIns/rtmp-services.so \
-x ./EBS.app/Contents/MacOS/ebs \
-x ./EBS.app/Contents/MacOS/obs-ffmpeg-mux \
-x ./EBS.app/Contents/PlugIns/obs-x264.so \
-x ./EBS.app/Contents/PlugIns/text-freetype2.so \
-x ./EBS.app/Contents/PlugIns/obs-libfdk.so
# -x ./EBS.app/Contents/PlugIns/obs-outputs.so \

/usr/local/Cellar/qt/5.10.1/bin/macdeployqt ./EBS.app

mv ./EBS.app/Contents/MacOS/libobs-opengl.so ./EBS.app/Contents/Frameworks

# put qt network in here becasuse streamdeck uses it
cp -r /usr/local/opt/qt/lib/QtNetwork.framework ./EBS.app/Contents/Frameworks
chmod +w ./EBS.app/Contents/Frameworks/QtNetwork.framework/Versions/5/QtNetwork
install_name_tool -change /usr/local/Cellar/qt/5.10.1/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/Frameworks/QtNetwork.framework/Versions/5/QtNetwork

# decklink ui qt
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/PlugIns/decklink-ouput-ui.so
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/PlugIns/decklink-ouput-ui.so
install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/PlugIns/decklink-ouput-ui.so

# frontend tools qt
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/PlugIns/frontend-tools.so
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/PlugIns/frontend-tools.so
install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/PlugIns/frontend-tools.so

# vst qt
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/PlugIns/obs-vst.so
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/PlugIns/obs-vst.so
install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/PlugIns/obs-vst.so
install_name_tool -change /usr/local/opt/qt/lib/QtMacExtras.framework/Versions/5/QtMacExtras @executable_path/../Frameworks/QtMacExtras.framework/Versions/5/QtMacExtras ./EBS.app/Contents/PlugIns/obs-vst.so
