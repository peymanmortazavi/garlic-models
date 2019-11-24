# CMake Settings
cmake_minimum_required(VERSION 3.11)

# Project Settings
project(GarlicModel VERSION 1.0.1 DESCRIPTION "Library for parsing and schema validation.")
include(GNUInstallDirs)

# If conan file is there, load it up.
IF(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
	include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
	conan_basic_setup()
ENDIF()

set(CMAKE_CXX_STANDARD 11)

# Options
option(BUILD_SHARED_LIB "Build shared/dynamic libraries instead of static." OFF)

# Sources.
set(PUBLIC_HEADERS include/garlic.h)
set(SOURCE_FILES ${PUBLIC_HEADERS} src/garlic.cpp)

# Library
IF(BUILD_SHARED_LIB)
    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})
ELSE()
    add_library(${PROJECT_NAME} ${SOURCE_FILES})
ENDIF()

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION} PUBLIC_HEADER "${PUBLIC_HEADERS}")
target_include_directories(
        ${PROJECT_NAME} INTERFACE
				"$<BUILD_INTERFACE:${GarlicModel_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>"
        PRIVATE "src" "include")
include_directories(${PROJECT_NAME} lib)


# Installation
include(CMakePackageConfigHelpers)  # To load the write_basic_package_version_file macro.
configure_file(garlicmodel.pc.in garlicmodel.pc @ONLY)
set(PROJECT_TARGET_EXPORT_NAME ${PROJECT_NAME}Targets)  # The project export target name.
set(CMAKE_FILES_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")  # This is where the cmake files would live, after installation.
set(GENERATED ${CMAKE_CURRENT_BINARY_DIR}/generated)  # Generated files live here until installation.
install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_TARGET_EXPORT_NAME}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
install(EXPORT ${PROJECT_TARGET_EXPORT_NAME}
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_FILES_INSTALL_DIR})
install(FILES ${CMAKE_BINARY_DIR}/garlicmodel.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
set(CONFIG_FILE ${GENERATED}/${PROJECT_NAME}Config.cmake)
configure_package_config_file("cmake/Config.cmake.in"
        "${CONFIG_FILE}" INSTALL_DESTINATION ${CMAKE_FILES_INSTALL_DIR})
set(VERSION_FILE ${GENERATED}/${PROJECT_NAME}ConfigVersion.cmake)
write_basic_package_version_file(
        ${VERSION_FILE}
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
)
install(FILES ${VERSION_FILE} ${CONFIG_FILE} DESTINATION ${CMAKE_FILES_INSTALL_DIR})

# Testing
include(CTest)
if (BUILD_TESTING)
    add_subdirectory(tests)
    enable_testing()
		add_test(GarlicModelTests tests/GarlicModelTests)
endif ()
