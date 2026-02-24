# Library containing the cpp objects used throughout the repo

set( LIB_NAME "globalLib" )

set( SRC_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/Global.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Functions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ParametersManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Geometry.cpp
  ${CMAKE_SOURCE_DIR}/src/io/InputOutput.cpp
)

if( USE_STATIC_LINKS )
  add_library( ${LIB_NAME} STATIC ${SRC_FILES} )
else()
  add_library( ${LIB_NAME} SHARED ${SRC_FILES} )
endif()

target_include_directories(
    ${LIB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/src/io
    ${CMAKE_SOURCE_DIR}/src/objects
)


target_link_libraries(
    ${LIB_NAME} PUBLIC
    ${ROOT_LIBRARIES}
)

install( TARGETS ${LIB_NAME} DESTINATION lib )
