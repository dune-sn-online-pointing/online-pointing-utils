set( LIB_NAME "calculatePositionLibs" )

set( SRC_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/position_calculator.cpp
)

if( USE_STATIC_LINKS )
  add_library( ${LIB_NAME} STATIC ${SRC_FILES} )
else()
  add_library( ${LIB_NAME} SHARED ${SRC_FILES} )
endif()


target_include_directories(
    ${LIB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)


target_link_libraries(
    ${LIB_NAME} PUBLIC
    ${ROOT_LIBRARIES}
)

install( TARGETS ${LIB_NAME} DESTINATION lib )
