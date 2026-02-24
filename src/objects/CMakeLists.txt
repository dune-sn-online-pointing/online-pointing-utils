# Library containing the cpp objects used throughout the repo

set( LIB_NAME "objectsLibs" )

set( SRC_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/Cluster.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Neutrino.cpp
)

if( USE_STATIC_LINKS )
  add_library( ${LIB_NAME} STATIC ${SRC_FILES} )
else()
  add_library( ${LIB_NAME} SHARED ${SRC_FILES} )
endif()


target_include_directories(
    ${LIB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/backtracking
)


target_link_libraries(
    ${LIB_NAME} PUBLIC
    backtrackingLibs
    globalLib
    ${ROOT_LIBRARIES}
)

install( TARGETS ${LIB_NAME} DESTINATION lib )
