cmessage( STATUS "Configuring ana library..." )

set( HEADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR} )
set( CODE_DIR ${CMAKE_CURRENT_SOURCE_DIR} )

set( HEADER_FILES
  Display.h
  Plotting.h
)

set( SOURCE_FILES
  Display.cpp
  Plotting.cpp
)

add_library( anaLib SHARED ${SOURCE_FILES} )
target_include_directories( anaLib PUBLIC ${HEADERS_DIR} )
target_link_libraries( anaLib PUBLIC globalLib ROOT::Hist ROOT::Graf )
