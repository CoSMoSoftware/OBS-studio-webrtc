rm -rf ./EBS.app

mkdir EBS.app
mkdir EBS.app/Contents
mkdir EBS.app/Contents/MacOS
mkdir EBS.app/Contents/Plugins
mkdir EBS.app/Contents/Resources

BUILD_CONFIG=RELEASE

cp -r rundir/$BUILD_CONFIG/bin/ ./EBS.app/Contents/MacOS
cp -r rundir/$BUILD_CONFIG/data ./EBS.app/Contents/Resources
cp ../CI/install/osx/obs.icns ./EBS.app/Contents/Resources
cp -r rundir/$BUILD_CONFIG/obs-plugins/ ./EBS.app/Contents/Plugins
cp ../CI/install/osx/Info.plist ./EBS.app/Contents

../CI/install/osx/dylibBundler -b -cd -d ./EBS.app/Contents/Frameworks -p @executable_path/../Frameworks/ \
-s ./EBS.app/Contents/MacOS \
-s /usr/local/opt/mbedtls/lib/ \
-x ./EBS.app/Contents/Plugins/coreaudio-encoder.so \
-x ./EBS.app/Contents/Plugins/decklink-ouput-ui.so \
-x ./EBS.app/Contents/Plugins/frontend-tools.so \
-x ./EBS.app/Contents/Plugins/image-source.so \
-x ./EBS.app/Contents/Plugins/linux-jack.so \
-x ./EBS.app/Contents/Plugins/mac-avcapture.so \
-x ./EBS.app/Contents/Plugins/mac-capture.so \
-x ./EBS.app/Contents/Plugins/mac-decklink.so \
-x ./EBS.app/Contents/Plugins/mac-syphon.so \
-x ./EBS.app/Contents/Plugins/mac-vth264.so \
-x ./EBS.app/Contents/Plugins/obs-ffmpeg.so \
-x ./EBS.app/Contents/Plugins/obs-filters.so \
-x ./EBS.app/Contents/Plugins/obs-transitions.so \
-x ./EBS.app/Contents/Plugins/obs-vst.so \
-x ./EBS.app/Contents/Plugins/rtmp-services.so \
-x ./EBS.app/Contents/MacOS/obs \
-x ./EBS.app/Contents/MacOS/obs-ffmpeg-mux \
-x ./EBS.app/Contents/Plugins/obs-x264.so \
-x ./EBS.app/Contents/Plugins/text-freetype2.so \
-x ./EBS.app/Contents/Plugins/obs-libfdk.so
# -x ./EBS.app/Contents/Plugins/obs-outputs.so \

/usr/local/Cellar/qt/5.10.1/bin/macdeployqt ./EBS.app

mv ./EBS.app/Contents/MacOS/libobs-opengl.so ./EBS.app/Contents/Frameworks

# put qt network in here becasuse streamdeck uses it
cp -r /usr/local/opt/qt/lib/QtNetwork.framework ./EBS.app/Contents/Frameworks
chmod +w ./EBS.app/Contents/Frameworks/QtNetwork.framework/Versions/5/QtNetwork
install_name_tool -change /usr/local/Cellar/qt/5.10.1/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/Frameworks/QtNetwork.framework/Versions/5/QtNetwork

# decklink ui qt
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/Plugins/decklink-ouput-ui.so
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/Plugins/decklink-ouput-ui.so
install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/Plugins/decklink-ouput-ui.so

# frontend tools qt
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/Plugins/frontend-tools.so
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/Plugins/frontend-tools.so
install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/Plugins/frontend-tools.so

# vst qt
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/Plugins/obs-vst.so
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/Plugins/obs-vst.so
install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/Plugins/obs-vst.so
install_name_tool -change /usr/local/opt/qt/lib/QtMacExtras.framework/Versions/5/QtMacExtras @executable_path/../Frameworks/QtMacExtras.framework/Versions/5/QtMacExtras ./EBS.app/Contents/Plugins/obs-vst.so
