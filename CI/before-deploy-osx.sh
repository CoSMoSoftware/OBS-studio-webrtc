hr() {
  echo "───────────────────────────────────────────────────"
  echo $1
  echo "───────────────────────────────────────────────────"
}

# Exit if something fails
set -e

# Generate file name variables
export GIT_HASH=$(git rev-parse --short HEAD)
export FILE_DATE=$(date +%Y-%m-%d.%H:%M:%S)
export FILENAME=$FILE_DATE-$GIT_HASH-$TRAVIS_BRANCH-osx.pkg
export DEPLOY_VERSION=2.5

cd ./build

# Package everything into a nice .app
hr "Packaging .app"
STABLE=false
if [ -n "${TRAVIS_TAG}" ]; then
  STABLE=true
fi

#sudo python ../CI/install/osx/build_app.py --public-key ../CI/install/osx/EBSPublicDSAKey.pem --sparkle-framework ../../sparkle/Sparkle.framework --stable=$STABLE

../CI/install/osx/packageApp.sh

# fix obs outputs
cp /usr/local/opt/mbedtls/lib/libmbedtls.12.dylib ./EBS.app/Contents/Frameworks/
cp /usr/local/opt/mbedtls/lib/libmbedcrypto.3.dylib ./EBS.app/Contents/Frameworks/
cp /usr/local/opt/mbedtls/lib/libmbedx509.0.dylib ./EBS.app/Contents/Frameworks/
install_name_tool -change /usr/local/opt/mbedtls/lib/libmbedtls.12.dylib @executable_path/../Frameworks/libmbedtls.12.dylib ./EBS.app/Contents/Plugins/obs-outputs.so
install_name_tool -change /usr/local/opt/mbedtls/lib/libmbedcrypto.3.dylib @executable_path/../Frameworks/libmbedcrypto.3.dylib ./EBS.app/Contents/Plugins/obs-outputs.so
install_name_tool -change /usr/local/opt/mbedtls/lib/libmbedx509.0.dylib @executable_path/../Frameworks/libmbedx509.0.dylib ./EBS.app/Contents/Plugins/obs-outputs.so
install_name_tool -change /usr/local/opt/curl/lib/libcurl.4.dylib @executable_path/../Frameworks/libcurl.4.dylib ./EBS.app/Contents/Plugins/obs-outputs.so
install_name_tool -change @rpath/libobs.0.dylib @executable_path/../Frameworks/libobs.0.dylib ./EBS.app/Contents/Plugins/obs-outputs.so

# copy sparkle into the app
hr "Copying Sparkle.framework"
cp -r ../../sparkle/Sparkle.framework ./EBS.app/Contents/Frameworks/
install_name_tool -change @rpath/Sparkle.framework/Versions/A/Sparkle @executable_path/../Frameworks/Sparkle.framework/Versions/A/Sparkle ./EBS.app/Contents/MacOS/obs

# Copy Chromium embedded framework to app Frameworks directory
# hr "Copying Chromium Embedded Framework.framework"
# sudo mkdir -p EBS.app/Contents/Frameworks
# sudo cp -r ../../cef_binary_${CEF_BUILD_VERSION}_macosx64/Release/Chromium\ Embedded\ Framework.framework EBS.app/Contents/Frameworks/

# install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/Plugins/obs-browser.so
# install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/Plugins/obs-browser.so
# install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/Plugins/obs-browser.so

cp ../CI/install/osx/EBSPublicDSAKey.pem EBS.app/Contents/Resources

# edit plist
plutil -insert CFBundleVersion -string $DEPLOY_VERSION ./EBS.app/Contents/Info.plist
plutil -insert CFBundleShortVersionString -string $DEPLOY_VERSION ./EBS.app/Contents/Info.plist
# plutil -insert EBSFeedsURL -string https://obsproject.com/osx_update/feeds.xml ./EBS.app/Contents/Info.plist
# plutil -insert SUFeedURL -string https://obsproject.com/osx_update/stable/updates.xml ./EBS.app/Contents/Info.plist
plutil -insert SUPublicDSAKeyFile -string EBSPublicDSAKey.pem ./EBS.app/Contents/Info.plist

#dmgbuild -s ../CI/install/osx/settings.json "EBS" ebs.dmg
dmgbuild "EBS" ebs.dmg

if [ -v "$TRAVIS" ]; then
	# Signing stuff
	hr "Decrypting Cert"
	openssl aes-256-cbc -K $encrypted_dd3c7f5e9db9_key -iv $encrypted_dd3c7f5e9db9_iv -in ../CI/osxcert/Certificates.p12.enc -out Certificates.p12 -d
	hr "Creating Keychain"
	security create-keychain -p mysecretpassword build.keychain
	security default-keychain -s build.keychain
	security unlock-keychain -p mysecretpassword build.keychain
	security set-keychain-settings -t 3600 -u build.keychain
	hr "Importing certs into keychain"
	security import ./Certificates.p12 -k build.keychain -T /usr/bin/productsign -P ""
	# macOS 10.12+
	security set-key-partition-list -S apple-tool:,apple: -s -k mysecretpassword build.keychain
fi

# Package app
hr "Generating .pkg"
packagesbuild ../CI/install/osx/CMakeLists.pkgproj

# Move to the folder that travis uses to upload artifacts from
hr "Moving package to nightly folder for distribution"
mkdir -p nightly
sudo mv OBS.pkg ./nightly/$FILENAME
cd ..
