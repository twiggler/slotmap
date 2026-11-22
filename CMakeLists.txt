cmake_minimum_required(VERSION 3.25)

project(slotmap
	VERSION 2.0.0
	DESCRIPTION "Header-only generational slot map with weak keys"
	HOMEPAGE_URL "https://github.com/twiggler/slotmap"
	LANGUAGES CXX)

include(GNUInstallDirs)

option(SLOTMAP_BUILD_TESTS "Build the unit tests" ${PROJECT_IS_TOP_LEVEL})
option(SLOTMAP_BUILD_DEMO "Build the demo" ${PROJECT_IS_TOP_LEVEL})
option(SLOTMAP_CLANG_TIDY "Run clang-tidy on slotmap's own targets" OFF)

add_library(slotmap INTERFACE)
add_library(slotmap::slotmap ALIAS slotmap)
target_include_directories(slotmap INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(slotmap INTERFACE cxx_std_23)

include(cmake/SlotmapDevelopment.cmake)

if(SLOTMAP_BUILD_TESTS)
	enable_testing()
	add_subdirectory(test)
endif()

if(SLOTMAP_BUILD_DEMO)
	add_subdirectory(demo)
endif()

# Installation: find_package(slotmap) then target_link_libraries(... slotmap::slotmap).
include(CMakePackageConfigHelpers)
set(SLOTMAP_CMAKE_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/cmake/slotmap")

install(TARGETS slotmap EXPORT slotmapTargets)
install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(EXPORT slotmapTargets NAMESPACE slotmap:: DESTINATION "${SLOTMAP_CMAKE_INSTALL_DIR}")

configure_package_config_file(cmake/slotmapConfig.cmake.in
	"${CMAKE_CURRENT_BINARY_DIR}/slotmapConfig.cmake"
	INSTALL_DESTINATION "${SLOTMAP_CMAKE_INSTALL_DIR}")
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/slotmapConfigVersion.cmake"
	COMPATIBILITY SameMajorVersion
	ARCH_INDEPENDENT)
install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/slotmapConfig.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/slotmapConfigVersion.cmake"
	DESTINATION "${SLOTMAP_CMAKE_INSTALL_DIR}")
