add_executable(slotmap_demo slotmap.cpp)
target_link_libraries(slotmap_demo PRIVATE slotmap::slotmap)
slotmap_configure_target(slotmap_demo)

add_test(NAME demo COMMAND slotmap_demo)
