# Library containing backtracking functions

set( LIB_NAME "backtrackingLibs" )

set( SRC_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/Backtracking.cpp
)

if( USE_STATIC_LINKS )
  add_library( ${LIB_NAME} STATIC ${SRC_FILES} )
else()
  add_library( ${LIB_NAME} SHARED ${SRC_FILES} )
endif()

target_include_directories(
    ${LIB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/lib
    ${CMAKE_SOURCE_DIR}/src/objects
)

target_link_libraries(
    ${LIB_NAME} PUBLIC
    objectsLibs
    globalLib
    ${ROOT_LIBRARIES}
)

install( TARGETS ${LIB_NAME} DESTINATION lib )

target_link_libraries( ${LIB_NAME} PRIVATE globalLib )