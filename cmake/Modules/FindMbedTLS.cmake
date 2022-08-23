# Once done these will be defined:
#
#  MBEDTLS_FOUND
#  MBEDTLS_INCLUDE_DIRS
#  MBEDTLS_LIBRARIES
#

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_MBEDTLS QUIET mbedtls)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_lib_suffix 64)
else()
	set(_lib_suffix 32)
endif()

# If we're on MacOS or Linux, please try to statically-link mbedtls.
if(STATIC_MBEDTLS AND (APPLE OR UNIX))
	set(_MBEDTLS_LIBRARIES libmbedtls.a)
	set(_MBEDCRYPTO_LIBRARIES libmbedcrypto.a)
	set(_MBEDX509_LIBRARIES libmbedx509.a)
endif()

# Enable Mac M1 compilation
if (APPLE AND (CMAKE_SYSTEM_PROCESSOR STREQUAL arm64 OR CMAKE_OSX_ARCHITECTURES STREQUAL arm64))
	message(STATUS "Mac M1 build: Searching for mbedtls@2 in homebrew")
	set(_MBEDTLS_SEARCH_PATHS /opt/homebrew/)
	set(_MBEDTLS_HINTS "")
else ()
	set(_MBEDTLS_SEARCH_PATHS /usr /usr/local /opt/local /sw)
	set(_MBEDTLS_HINTS 
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		${mbedtlsPath${_lib_suffix}}
		${mbedtlsPath}
		${DepsPath${_lib_suffix}}
		${DepsPath}
	)
endif()
find_path(MBEDTLS_INCLUDE_DIR
	NAMES mbedtls/ssl.h
	HINTS
		ENV mbedtlsPath${_lib_suffix}
		ENV mbedtlsPath
		${_MBEDTLS_HINTS}
		${_MBEDTLS_INCLUDE_DIRS}
	PATHS
		${_MBEDTLS_SEARCH_PATHS}
	PATH_SUFFIXES
		include)

find_library(MBEDTLS_LIB
	NAMES ${_MBEDTLS_LIBRARIES} mbedtls libmbedtls
	HINTS
		ENV mbedtlsPath${_lib_suffix}
		ENV mbedtlsPath
		${_MBEDTLS_HINTS}
		${_MBEDTLS_LIBRARY_DIRS}
	PATHS
		${_MBEDTLS_SEARCH_PATHS}

	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin
		../lib${_lib_suffix} ../lib
		../libs${_lib_suffix} ../libs
		../bin${_lib_suffix} ../bin

	REQUIRED)

find_library(MBEDCRYPTO_LIB
	NAMES ${_MBEDCRYPTO_LIBRARIES} mbedcrypto libmbedcrypto
	HINTS
		ENV mbedcryptoPath${_lib_suffix}
		ENV mbedcryptoPath
		${_MBEDTLS_HINTS}
		${_MBEDCRYPTO_LIBRARY_DIRS}
	PATHS
		${_MBEDTLS_SEARCH_PATHS}
	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin
		../lib${_lib_suffix} ../lib
		../libs${_lib_suffix} ../libs
		../bin${_lib_suffix} ../bin)

find_library(MBEDX509_LIB
	NAMES ${_MBEDX509_LIBRARIES} mbedx509 libmbedx509
	HINTS
		ENV mbedx509Path${_lib_suffix}
		ENV mbedx509Path
		${_MBEDTLS_HINTS}
		${_MBEDX509_LIBRARY_DIRS}
	PATHS
		${_MBEDTLS_SEARCH_PATHS}
	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin
		../lib${_lib_suffix} ../lib
		../libs${_lib_suffix} ../libs
		../bin${_lib_suffix} ../bin)

# Sometimes mbedtls is split between three libs, and sometimes it isn't.
# If it isn't, let's check if the symbols we need are all in MBEDTLS_LIB.
if(MBEDTLS_LIB AND NOT MBEDCRYPTO_LIB AND NOT MBEDX509_LIB)
	set(CMAKE_REQUIRED_LIBRARIES ${MBEDTLS_LIB})
	set(CMAKE_REQUIRED_INCLUDES ${MBEDTLS_INCLUDE_DIR})
	check_symbol_exists(mbedtls_x509_crt_init "mbedtls/x509_crt.h" MBEDTLS_INCLUDES_X509)
	check_symbol_exists(mbedtls_sha256_init "mbedtls/sha256.h" MBEDTLS_INCLUDES_CRYPTO)
	unset(CMAKE_REQUIRED_INCLUDES)
	unset(CMAKE_REQUIRED_LIBRARIES)
endif()

# If we find all three libraries, then go ahead.
if(MBEDTLS_LIB AND MBEDCRYPTO_LIB AND MBEDX509_LIB)
	set(MBEDTLS_INCLUDE_DIRS ${MBEDTLS_INCLUDE_DIR})
	set(MBEDTLS_LIBRARIES ${MBEDTLS_LIB} ${MBEDCRYPTO_LIB} ${MBEDX509_LIB})

# Otherwise, if we find MBEDTLS_LIB, and it has both CRYPTO and x509
# within the single lib (i.e. a windows build environment), then also
# feel free to go ahead.
elseif(MBEDTLS_LIB AND MBEDTLS_INCLUDES_CRYPTO AND MBEDTLS_INCLUDES_X509)
	set(MBEDTLS_INCLUDE_DIRS ${MBEDTLS_INCLUDE_DIR})
	set(MBEDTLS_LIBRARIES ${MBEDTLS_LIB})
endif()

# Now we've accounted for the 3-vs-1 library case:
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MbedTLS DEFAULT_MSG MBEDTLS_LIBRARIES MBEDTLS_INCLUDE_DIRS)
mark_as_advanced(MBEDTLS_INCLUDE_DIR MBEDTLS_LIB MBEDCRYPTO_LIB MBEDX509_LIB)

