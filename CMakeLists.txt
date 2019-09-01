cmake_minimum_required (VERSION 2.6)
project (tentoolkit)

# compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE")

# dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules (libwim REQUIRED IMPORTED_TARGET wimlib)
pkg_check_modules (libxml2 REQUIRED IMPORTED_TARGET libxml-2.0)
pkg_check_modules (libcurl REQUIRED IMPORTED_TARGET libcurl)
pkg_check_modules (libarchive REQUIRED IMPORTED_TARGET libarchive)

# esdurl
add_executable (esdurl esdurl.c)
target_link_libraries (esdurl PkgConfig::libxml2 PkgConfig::libcurl
	PkgConfig::libarchive)

# mkmedia
add_executable (mkmedia mkmedia.c)
target_link_libraries (mkmedia PkgConfig::libwim)
