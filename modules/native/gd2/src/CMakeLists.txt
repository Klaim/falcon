####################################################################
# The Falcon Programming Language
#
# CMake configuration file for gd2
####################################################################

FALCON_DEFINE_MODULE( CURRENT_MODULE gd2 )

# Inclusion settings
INCLUDE_DIRECTORIES(.)
# Enable this include if the project has a private include
#INCLUDE_DIRECTORIES( "${PROJECT_SOURCE_DIR}/include" )

#
# Falcon generic directories
#

INCLUDE_DIRECTORIES( ${Falcon_INCLUDE_DIRS} )
LINK_DIRECTORIES( ${FALCON_LIB_PATH} )

FIND_PACKAGE(GD REQUIRED)
INCLUDE_DIRECTORIES(GD_INCLUDE_DIR)

#sources
FILE( GLOB SRC_FILES "*.cpp" )

# Target
ADD_LIBRARY( ${CURRENT_MODULE} MODULE
   ${SRC_FILES}
)

#Link
#FALCON_LINK_MODULE( ${CURRENT_MODULE} )

#If your module needs some lib more, use the followings:
#
TARGET_LINK_LIBRARIES( ${CURRENT_MODULE} falcon_engine ${GD_LIBRARIES} fontconfig freetype m)
FALCON_INSTALL_MODULE( ${CURRENT_MODULE} )
#