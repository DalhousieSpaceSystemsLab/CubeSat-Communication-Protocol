cmake_minimum_required(VERSION 3.18)


################################################################################
#  OPTIONS GO HERE
################################################################################
option(BUILD_TESTING "[ON/OFF] Build tests in addition to library" ON)
option(BUILD_EXAMPLES "[ON/OFF] Build examples in addition to library" ON)


################################################################################
#  PROJECT INIT
################################################################################
project(
    libcsp
    VERSION 1.0
    DESCRIPTION "LIBCSP"
    LANGUAGES C CXX
)
set(LIB "${PROJECT_NAME}") # this is PROJECT_NAME, NOT CMAKE_PROJECT_NAME
message("CONFIGURING TARGET : ${LIB}")

if(TARGET ${LIB})
    message(FATAL_ERROR "Target ${LIB} already exists in this project!")
else()
    add_library(${LIB} STATIC IMPORTED GLOBAL)
    set_property(TARGET ${LIB} PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/libcsp/build/libcsp.a)
endif(TARGET ${LIB})

target_include_directories(${LIB} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/build/include)