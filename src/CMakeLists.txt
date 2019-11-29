set(HEADER_LIST "${GarlicModel_SOURCE_DIR}/include/garlic/garlic.h")

add_library(GarlicModel garlic.cpp ${HEADER_LIST})

set_target_properties(GarlicModel PROPERTIES VERSION ${PROJECT_VERSION} PUBLIC_HEADER "${HEADER_LIST}")

target_include_directories(GarlicModel
  INTERFACE "$<BUILD_INTERFACE:${GarlicModel_SOURCE_DIR}/include>" "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>"
  PRIVATE "src"
)

target_compile_features(GarlicModel PUBLIC cxx_std_17)

# For IDEs to put this in a nice folder.
source_group(TREE "${PROJECT_SOURCE_DIR}/include" PREFIX "Header Files" FILES ${HEADER_LIST})
