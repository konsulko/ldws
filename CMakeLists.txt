cmake_minimum_required (VERSION 2.6) 

PROJECT(ldws)
SET(PROJECT_VERSION "0.9")

configure_file("config.h.in" "config.h")

FIND_PACKAGE(PkgConfig REQUIRED)

# TCLAP
PKG_CHECK_MODULES(TCLAP REQUIRED tclap>=1.2.0)

# Config++
PKG_CHECK_MODULES(CONFIG++ REQUIRED libconfig++>=1.5)

# OpenCV
FIND_PACKAGE( OpenCV REQUIRED )
IF (${OpenCV_VERSION} VERSION_LESS 3.0.0)
	MESSAGE(FATAL_ERROR "OpenCV version is not compatible : ${OpenCV_VERSION}")
ENDIF()

SET(SRC
	main.cc config_store.cc lane_detector.cc
)

SET(PROJECT_NAME
 ldws
)

ADD_EXECUTABLE( ${PROJECT_NAME} ${SRC} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME}  ${OpenCV_LIBS} ${CONFIG++_LIBRARIES})
