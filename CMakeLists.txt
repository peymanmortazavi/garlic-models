# CMake Settings
cmake_minimum_required(VERSION 3.11)

# Project Settings
project(GarlicModel VERSION 0.0.1 DESCRIPTION "Library for parsing and schema validation.")
include(GNUInstallDirs)

# If conan file is there, load it up.
IF(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  set(CONAN_DISABLE_CHECK_COMPILER disable)
  include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
ENDIF()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(GarlicModel INTERFACE)
target_include_directories(
    GarlicModel
      INTERFACE
        "$<BUILD_INTERFACE:${GarlicModel_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>"
)

# Installation
include(CMakePackageConfigHelpers)  # To load the write_basic_package_version_file macro.
configure_file(garlicmodel.pc.in garlicmodel.pc @ONLY)
set(PROJECT_TARGET_EXPORT_NAME ${PROJECT_NAME}Targets)  # The project export target name.
set(CMAKE_FILES_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")  # This is where the cmake files would live, after installation.
set(GENERATED ${CMAKE_CURRENT_BINARY_DIR}/generated)  # Generated files live here until installation.
install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_TARGET_EXPORT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/garlic)
install(EXPORT ${PROJECT_TARGET_EXPORT_NAME}
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION ${CMAKE_FILES_INSTALL_DIR})
install(FILES ${CMAKE_BINARY_DIR}/garlicmodel.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/garlic" DESTINATION include FILES_MATCHING PATTERN "*.h")
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
  enable_testing()
  add_subdirectory(tests)
  add_test(GarlicModelTests GarlicModelTests)
  set_tests_properties(GarlicModelTests PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif ()
