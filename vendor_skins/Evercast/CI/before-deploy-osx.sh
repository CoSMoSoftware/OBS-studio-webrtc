# Copyright Dr. Alex. Gouaillard (2015, 2020)

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

cd ./build

# Package everything into a nice .app
hr "Packaging .app"
STABLE=false
if [ -n "${TRAVIS_TAG}" ]; then
  STABLE=true
fi

sudo python ../CI/install/osx/build_app.py --public-key ../CI/install/osx/OBSPublicDSAKey.pem --sparkle-framework ../../sparkle/Sparkle.framework  --stable=$STABLE


# NOTE ALEX: switch from package to DMG

# Package app
hr "Generating .pkg"
packagesbuild ../CI/install/osx/CMakeLists.pkgproj

# Move to the folder that travis uses to upload artifacts from
hr "Moving package to nightly folder for distribution"
mkdir -p ./nightly
sudo mv EBS.pkg ./nightly/$FILENAME

# Make sure to update travis deployment env:
echo rvm_autoupdate_flag=2 >> ~/.rvmrc
