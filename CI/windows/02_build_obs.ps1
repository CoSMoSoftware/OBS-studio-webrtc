Param(
    [Switch]$Help = $(if (Test-Path variable:Help) { $Help }),
    [Switch]$Quiet = $(if (Test-Path variable:Quiet) { $Quiet }),
    [Switch]$Verbose = $(if (Test-Path variable:Verbose) { $Verbose }),
    [String]$BuildDirectory = $(if (Test-Path variable:BuildDirectory) { "${BuildDirectory}" } else { "build" }),
    [ValidateSet('x86', 'x64')]
    [String]$BuildArch = $(if (Test-Path variable:BuildArch) { "${BuildArch}" } else { ('x86', 'x64')[[System.Environment]::Is64BitOperatingSystem] }),
    [ValidateSet("Release", "RelWithDebInfo", "MinSizeRel", "Debug")]
    [String]$BuildConfiguration = $(if (Test-Path variable:BuildConfiguration) { "${BuildConfiguration}" } else { "RelWithDebInfo" }),
    [ValidateSet("Millicast", "Wowza", "RemoteFilming", "RemoteFilming-A", "RemoteFilming-B", "RemoteFilming-C", "RemoteFilming-D")]
    [String]$Vendor = $(if (Test-Path variable:Vendor) { "${Vendor}" } else { "Millicast" }),
    [String]$Ndi = $(if (Test-Path variable:Ndi) { "${Ndi}" } else { "OFF"})
)

##############################################################################
# Windows libobs library build function
##############################################################################
#
# This script file can be included in build scripts for Windows or run
# directly
#
##############################################################################

$ErrorActionPreference = "Stop"

function Build-OBS {
    Param(
        [String]$BuildDirectory = $(if (Test-Path variable:BuildDirectory) { "${BuildDirectory}" }),
        [String]$BuildArch = $(if (Test-Path variable:BuildArch) { "${BuildArch}" }),
        [String]$BuildConfiguration = $(if (Test-Path variable:BuildConfiguration) { "${BuildConfiguration}" }),
        [String]$Vendor = $(if (Test-Path variable:Vendor) { "${Vendor}" }),
        [String]$Ndi = $(if (Test-Path variable:Ndi) { "${Ndi}" })
    )

    $NumProcessors = (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors

    if ( $NumProcessors -gt 1 ) {
        $env:UseMultiToolTask = $true
        $env:EnforceProcessCountAcrossBuilds = $true
    }

    Write-Status "Build OBS"

    Configure-OBS

    Ensure-Directory ${CheckoutDir}
    Write-Step "Build OBS targets..."

    $BuildDirectoryActual = "${BuildDirectory}$(if (${BuildArch} -eq "x64") { "64" } else { "32" })_${Vendor}"
    Write-Step "build directory = ${BuildDirectoryActual}"

    Invoke-External cmake --build "${BuildDirectoryActual}" --config ${BuildConfiguration}
}

function Configure-OBS {
    Ensure-Directory ${CheckoutDir}
    Write-Status "Configuration of OBS build system..."

    $NumProcessors = (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors

    if ( $NumProcessors -gt 1 ) {
        $env:UseMultiToolTask = $true
        $env:EnforceProcessCountAcrossBuilds = $true
    }

    # TODO: Clean up archive and directory naming across dependencies
    $CmakePrefixPath = Resolve-Path -Path "${CheckoutDir}/../obs-build-dependencies/windows-deps-${WindowsDepsVersion}-${BuildArch}"
    $CefDirectory = Resolve-Path -Path "${CheckoutDir}/../obs-build-dependencies/cef_binary_${WindowsCefVersion}_windows_${BuildArch}"
    $OpensslDirectory = Resolve-Path -Path "${CheckoutDir}/../obs-build-dependencies/openssl-1.1/x64"
    $BuildDirectoryActual = "${BuildDirectory}$(if (${BuildArch} -eq "x64") { "64" } else { "32" })_${Vendor}"
    $GeneratorPlatform = "$(if (${BuildArch} -eq "x64") { "x64" } else { "Win32" })"

    $CmakeCommand = @(
        "-G", ${CmakeGenerator}
        "-DCMAKE_GENERATOR_PLATFORM=`"${GeneratorPlatform}`"",
        "-DCMAKE_SYSTEM_VERSION=`"${CmakeSystemVersion}`"",
        "-DCMAKE_PREFIX_PATH:PATH=`"${CmakePrefixPath}`"",
        "-DCEF_ROOT_DIR:PATH=`"${CefDirectory}`"",
        "-DENABLE_BROWSER=ON",
        "-DVLC_PATH:PATH=`"${CheckoutDir}/../obs-build-dependencies/vlc-${WindowsVlcVersion}`"",
        "-DENABLE_VLC=ON",
        "-DCMAKE_INSTALL_PREFIX=`"${BuildDirectoryActual}/install`"",
        "-DVIRTUALCAM_GUID=`"${Env:VIRTUALCAM-GUID}`"",
        "-DTWITCH_CLIENTID=`"${Env:TWITCH_CLIENTID}`"",
        "-DTWITCH_HASH=`"${Env:TWITCH_HASH}`"",
        "-DRESTREAM_CLIENTID=`"${Env:RESTREAM_CLIENTID}`"",
        "-DRESTREAM_HASH=`"${Env:RESTREAM_HASH}`"",
        "-DYOUTUBE_CLIENTID=`"${Env:YOUTUBE_CLIENTID}`"",
        "-DYOUTUBE_CLIENTID_HASH=`"${Env:YOUTUBE_CLIENTID_HASH}`"",
        "-DYOUTUBE_SECRET=`"${Env:YOUTUBE_SECRET}`"",
        "-DYOUTUBE_SECRET_HASH=`"${Env:YOUTUBE_SECRET_HASH}`"",
        "-DCOPIED_DEPENDENCIES=OFF",
        "-DCOPY_DEPENDENCIES=ON",
        "-DBUILD_FOR_DISTRIBUTION=ON",
        "$(if (Test-Path Env:CI) { "-DOBS_BUILD_NUMBER=${Env:GITHUB_RUN_ID}" })",
        "$(if (Test-Path Variable:$Quiet) { "-Wno-deprecated -Wno-dev --log-level=ERROR" })",
        "-Dlibwebrtc_DIR=`"C:/Program Files/libwebrtc/cmake`"",
        "-DOPENSSL_ROOT_DIR=`"$OpensslDirectory`"",
        "$(if (${Vendor} -ne 'Millicast') { "-DOBS_WEBRTC_VENDOR_NAME=${Vendor}" })",
        "-DOBS_VERSION_OVERRIDE=`"${Env:OBS_VERSION}`"",
        "-DBUILD_NDI=${Ndi}",
        "-Dlibobs_DIR=`"${BuildDirectoryActual}/libobs`"",
        "-DLIBOBS_INCLUDE_DIRS=`"${BuildDirectoryActual}/libobs`"",
        "-DLIBOBS_LIB=`"${BuildDirectoryActual}/libobs/${CMAKE_BUILD_TYPE}/obs.dll`""
    )

# Set-PSDebug -Trace 1

Write-Status "********************* location"
Get-Location
if ((Test-Path $BuildDirectoryActual/libobs)) {
    Write-Status "********************* dir"
    Get-ChildItem ./${BuildDirectoryActual}
}
if ((Test-Path $BuildDirectoryActual/libobs/libobsConfig.cmake)) {
    Write-Status "********************* $BuildDirectoryActual/libobs/libobsConfig.cmake EXISTS"
} else {
    Write-Status "********************* $BuildDirectoryActual/libobs/libobsConfig.cmake does not exist"
}
Write-Status "********************* obs version"
echo "OBS version = ${Env:OBS_VERSION}"
Write-Status "********************* cmake command"
echo "commande = ${CmakeCommand}"

    Invoke-External cmake -S . -B  "${BuildDirectoryActual}" @CmakeCommand

    Ensure-Directory ${CheckoutDir}
}

function Build-OBS-Standalone {
    $ProductName = "OBS-Studio"

    $CheckoutDir = Resolve-Path -Path "$PSScriptRoot\..\.."
    $DepsBuildDir = "${CheckoutDir}/../obs-build-dependencies"
    $ObsBuildDir = "${CheckoutDir}/../obs-studio"

    . ${CheckoutDir}/CI/include/build_support_windows.ps1

    Build-OBS
}

function Print-Usage {
    $Lines = @(
        "Usage: ${_ScriptName}",
        "-Help                    : Print this help",
        "-Quiet                   : Suppress most build process output",
        "-Verbose                 : Enable more verbose build process output",
        "-BuildDirectory          : Directory to use for builds - Default: build64 on 64-bit systems, build32 on 32-bit systems",
        "-BuildArch               : Build architecture to use (x86 or x64) - Default: local architecture",
        "-BuildConfiguration      : Build configuration to use - Default: RelWithDebInfo",
        "-Vendor                  : Vendor name - Default: Millicast",
        "-Ndi                     : Enable plugin obs-ndi (default: OFF)"
    )

    $Lines | Write-Host
}

if(!(Test-Path variable:_RunObsBuildScript)) {
    $_ScriptName = "$($MyInvocation.MyCommand.Name)"
    if($Help.isPresent) {
        Print-Usage
        exit 0
    }

    Build-OBS-Standalone
}
