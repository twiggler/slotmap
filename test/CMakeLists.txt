include(FetchContent)

# Pinned by release tarball and hash; SYSTEM keeps GoogleTest's headers out of our -Werror warnings.
FetchContent_Declare(googletest
	URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.tar.gz
	URL_HASH SHA256=6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5
	SYSTEM)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(slotmap_tests
	slotmap_test.cpp
	skipfield_test.cpp)
target_link_libraries(slotmap_tests PRIVATE slotmap::slotmap GTest::gtest_main)
slotmap_configure_target(slotmap_tests)

include(GoogleTest)
gtest_discover_tests(slotmap_tests)
