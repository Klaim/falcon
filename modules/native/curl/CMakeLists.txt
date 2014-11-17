####################################################################
# FALCON - The Falcon Programming Language.
#
# CMake configuration file for curl
####################################################################

cmake_minimum_required(VERSION 2.6)

if(NOT FALCON_BUILD_CURL ) 
	message( STATUS "NOT building module: curl") 
	return() 
endif() 
message( STATUS "Building module: curl" ) 


FalconDeclareCanonical( CURL curl )

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(CURL 7.18.2 REQUIRED)

## process source directories
# add other subdirectories that contain libraries on which the final module depends here..
add_subdirectory(src)
