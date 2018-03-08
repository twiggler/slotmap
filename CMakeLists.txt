add_library(slotmap INTERFACE)
target_include_directories(slotmap INTERFACE include/)

# see https://cmake.org/cmake/help/latest/module/FindBoost.html
find_package(Boost REQUIRED)

include_directories(${Boost_INCLUDE_DIR})

