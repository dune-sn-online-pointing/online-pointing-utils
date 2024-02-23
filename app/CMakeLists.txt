# create the executables for the apps

cmessage( STATUS "Creating cluster_to_root app..." )
add_executable( cluster_to_root ${CMAKE_CURRENT_SOURCE_DIR}/cluster_to_root.cpp )
target_link_libraries( cluster_to_root utils-libraries )
install( TARGETS cluster_to_root DESTINATION bin )

cmessage( STATUS "Creating superimpose_root_files.cpp app..." )
add_executable( superimpose_root_files.cpp ${CMAKE_CURRENT_SOURCE_DIR}/superimpose_root_files.cpp )
target_link_libraries( superimpose_root_files.cpp utils-libraries )
install( TARGETS superimpose_root_files.cpp DESTINATION bin )
