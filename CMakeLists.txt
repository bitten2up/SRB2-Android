cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

if("${CMAKE_CURRENT_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
	message(FATAL_ERROR "In-source builds are blocked. Please build from a separate directory.")
endif()

# Set up CMAKE path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include(CMakeDependentOption)
include(CheckCXXCompilerFlag)

file(STRINGS src/version.h SRB2_VERSION)
string(REGEX MATCH "[0-9]+\\.[0-9.]+" SRB2_VERSION ${SRB2_VERSION})

# DO NOT CHANGE THIS SRB2 STRING! Some variable names depend on this string.
# Version change is fine.
project(SRB2
	VERSION ${SRB2_VERSION}
	LANGUAGES C CXX)

##### PACKAGE CONFIGURATION #####

set(SRB2_CPACK_GENERATOR "" CACHE STRING "Generator to use for making a package. E.g., ZIP, TGZ, DragNDrop (OSX only). Leave blank for default generator.")

if("${SRB2_CPACK_GENERATOR}" STREQUAL "")
	if("${CMAKE_SYSTEM_NAME}" MATCHES "Windows")
		set(SRB2_CPACK_GENERATOR "ZIP")
	elseif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
		set(SRB2_CPACK_GENERATOR "TGZ")
	elseif("${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")
		set(SRB2_CPACK_GENERATOR "TGZ")
	endif()
endif()

set(CPACK_GENERATOR ${SRB2_CPACK_GENERATOR})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Sonic Robo Blast 2" CACHE STRING "Program name for display purposes")
set(CPACK_PACKAGE_VENDOR "Sonic Team Jr." CACHE STRING "Vendor name for display purposes")
#set(CPACK_PACKAGE_DESCRIPTION_FILE )
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR ${SRB2_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${SRB2_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${SRB2_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMAKE_VERSION_MAJOR}.${CMAKE_VERSION_MINOR}")
SET(CPACK_OUTPUT_FILE_PREFIX package)
include(CPack)

# Options
if("${CMAKE_SYSTEM_NAME}" MATCHES Windows)
	if(DEFINED VCPKG_TARGET_TRIPLET)
		set(SRB2_CONFIG_SYSTEM_LIBRARIES_DEFAULT ON)
	else()
		set(SRB2_CONFIG_SYSTEM_LIBRARIES_DEFAULT OFF)
	endif()
else()
	set(SRB2_CONFIG_SYSTEM_LIBRARIES_DEFAULT ON)
endif()

# Clang tidy options will be ignored if CMAKE_<LANG>_CLANG_TIDY are set.
option(SRB2_CONFIG_ENABLE_CLANG_TIDY_C "Enable default clang-tidy check configuration for C" OFF)
option(SRB2_CONFIG_ENABLE_CLANG_TIDY_CXX "Enable default clang-tidy check configuration for C++" OFF)
option(
	SRB2_CONFIG_STATIC_STDLIB
	"Link static version of standard library. All dependencies must also be static"
	ON
)
option(SRB2_CONFIG_HWRENDER "Enable hardware render (OpenGL) support" ON)
option(SRB2_CONFIG_HAVE_GLES "Enable GLES1 OpenGL renderer" OFF)
option(SRB2_CONFIG_HAVE_GLES2 "Enable GLES2 OpenGL renderer" ON)
option(SRB2_CONFIG_USE_GME "Enable GME playback support" OFF)
option(SRB2_CONFIG_STATIC_OPENGL "Enable static linking GL (do not do this)" OFF)
option(SRB2_CONFIG_ERRORMODE "Compile C code with warnings treated as errors." OFF)
option(SRB2_CONFIG_DEBUGMODE "Compile with PARANOIA, ZDEBUG, RANGECHECK and PACKETDROP defined." OFF)
option(SRB2_CONFIG_MOBJCONSISTANCY "Compile with MOBJCONSISTANCY defined." OFF)
option(SRB2_CONFIG_PACKETDROP "Compile with PACKETDROP defined." OFF)
option(SRB2_CONFIG_EXECINFO "Enable stack trace dump support." ON)
option(SRB2_CONFIG_DEBUGSYMBOLS_SEPARATE "Strips the main executable of debug symbols and allows a seperate executable to be linked with them instead." OFF)
option(SRB2_CONFIG_ZDEBUG "Compile with ZDEBUG defined." OFF)
option(SRB2_CONFIG_FORCE_NO_MS_BITFIELDS "Compile without -mno-ms-bitfields compiler flag" OFF)
# SRB2_CONFIG_PROFILEMODE is probably superceded by some CMake setting.
option(SRB2_CONFIG_PROFILEMODE "Compile for profiling (GCC only)." OFF)
set(SRB2_CONFIG_ASSET_DIRECTORY "" CACHE PATH "Path to directory that contains all asset files for the installer. If set, assets will be part of installation and cpack.")

# Dependencies
add_subdirectory(thirdparty)

if(SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES)
	set(SRB2_INTERNAL_LIBRARY_TYPE SHARED)
	set(NOT_SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES OFF)
else()
	set(SRB2_INTERNAL_LIBRARY_TYPE STATIC)
	set(NOT_SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES ON)
endif()


find_package(ZLIB REQUIRED)

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(PNG_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/thirdparty/libpng/iOS/include")
    set(PNG_LIBRARIES "${CMAKE_SOURCE_DIR}/thirdparty/libpng/iOS/libpng.xcframework")

    if(NOT TARGET PNG::PNG)
        add_library(PNG::PNG STATIC IMPORTED GLOBAL)

        set_target_properties(PNG::PNG PROPERTIES
            IMPORTED_LOCATION "${PNG_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${PNG_INCLUDE_DIRS}"
			INTERFACE_LINK_LIBRARIES ZLIB::ZLIB
        )
    endif()
else()
    find_package(PNG REQUIRED)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(CURL_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/thirdparty/curl/iOS/include")
    set(CURL_LIBRARIES "${CMAKE_SOURCE_DIR}/thirdparty/curl/iOS/curl.xcframework")

    if(NOT TARGET CURL::libcurl)
        add_library(CURL::libcurl STATIC IMPORTED GLOBAL)

        set_target_properties(CURL::libcurl PROPERTIES
            IMPORTED_LOCATION "${CURL_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIRS}"
            # Built with CURL_USE_SECTRANSP (Apple's native TLS) instead of
            # a separately cross-compiled OpenSSL -- see
            # srb2-legacy-ioslibs/build.sh. SecureTransport needs
            # Security.framework; curl's Apple network-change/proxy
            # detection code needs SystemConfiguration.framework.
            INTERFACE_LINK_LIBRARIES "-framework Security;-framework SystemConfiguration"
        )
    endif()
else()
    find_package(CURL REQUIRED)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(OPENMPT_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/thirdparty/libopenmpt/iOS/include")
    set(OPENMPT_LIBRARIES "${CMAKE_SOURCE_DIR}/thirdparty/libopenmpt/iOS/libopenmpt.xcframework")

    if(NOT TARGET libopenmpt::libopenmpt)
        add_library(libopenmpt::libopenmpt STATIC IMPORTED GLOBAL)

        set_target_properties(libopenmpt::libopenmpt PROPERTIES
            IMPORTED_LOCATION "${OPENMPT_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${OPENMPT_INCLUDE_DIRS}"
        )
    endif()
else()
    find_package(libopenmpt QUIET)
endif()

if("${SRB2_CONFIG_USE_GME}")
    find_package(libgme QUIET)
endif()
if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    # Not cross-compiled for iOS (no xcframework); find_package() would
    # otherwise pick up a host Homebrew miniupnpc whose CMake package
    # doesn't add its include dir explicitly, so it silently breaks under
    # iOS's isysroot-based cross-compilation. UPnP port-forwarding is
    # optional -- src/CMakeLists.txt already handles "not found" cleanly.
    find_package(miniupnpc QUIET)
endif()


if(${PROJECT_SOURCE_DIR} MATCHES ${PROJECT_BINARY_DIR})
	message(FATAL_ERROR "In-source builds will bring you a world of pain. Please make a separate directory to invoke CMake from.")
endif()

if ((${SRB2_USE_CCACHE}) AND (${CMAKE_C_COMPILER} MATCHES "clang"))
	message(WARNING "Using clang and CCache: You may want to set environment variable CCACHE_CPP2=yes to prevent include errors during compile.")
endif()

# bitness check
set(SRB2_SYSTEM_BITS 0)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(STATUS "Target is 64-bit")
	set(SRB2_SYSTEM_BITS 64)
endif()
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
	message(STATUS "Target is 32-bit")
	set(SRB2_SYSTEM_BITS 32)
endif()
if(${SRB2_SYSTEM_BITS} EQUAL 0)
	message(STATUS "Target bitness is unknown")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Set EXE names so the assets CMakeLists can refer to its target
set(SRB2_SDL2_EXE_NAME "" CACHE STRING "Override executable binary output name")
set(SRB2_SDL2_EXE_SUFFIX "" CACHE STRING "Optional executable suffix, separated by an underscore")

include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)

add_subdirectory(src)

# This must run before add_subdirectory(assets): that directory's
# CMakeLists.txt reads SRB2SDL2's OUTPUT_NAME property (via
# get_target_property) to know the real .app bundle name for the iOS/macOS
# asset-install destination, so the override below has to land first, or it
# reads the pre-override name src/sdl/CMakeLists.txt set (e.g. "Sonic Robo
# Blast 2" on Apple platforms) instead of the final git-branch-suffixed one.
set(GIT_EXECUTABLE "git" CACHE FILEPATH "Path to git binary")
include(GitUtilities)

if("${SRB2_SDL2_EXE_NAME}" STREQUAL "")
	# cause a reconfigure if the branch changes
	get_git_dir(SRB2_GIT_DIR)
	configure_file("${SRB2_GIT_DIR}/HEAD" HEAD COPYONLY)

	git_current_branch(SRB2_GIT_REVISION)

	if("${SRB2_GIT_REVISION}" STREQUAL "")
		# use abbreviated commit hash if on detached HEAD
		git_latest_commit(SRB2_GIT_REVISION)
	endif()

	if("${CMAKE_SYSTEM_NAME}" MATCHES "Windows")
		list(APPEND EXE_NAME_PARTS "srb2win")
	elseif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
		list(APPEND EXE_NAME_PARTS "lsdlsrb2")
	else()
		list(APPEND EXE_NAME_PARTS "srb2")
	endif()

	if(NOT "${SRB2_GIT_REVISION}" STREQUAL "master")
		list(APPEND EXE_NAME_PARTS ${SRB2_GIT_REVISION})
	endif()
else()
	list(APPEND EXE_NAME_PARTS ${SRB2_SDL2_EXE_NAME})
endif()

list(APPEND EXE_NAME_PARTS ${SRB2_SDL2_EXE_SUFFIX})

list(JOIN EXE_NAME_PARTS "_" EXE_NAME)
set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME ${EXE_NAME})

add_subdirectory(assets)
