###################
# link submodules
###################

cmessage( STATUS "Adding submodules to the project..." )
set( SUBMODULE_DIR ${CMAKE_SOURCE_DIR}/submodules )

# Function to make sure if a given submodule is present:
function( checkSubmodule )
  list( GET ARGV 0 SELECTED_SUBMODULE )
  cmessage( WARNING "Checking submodule: ${SELECTED_SUBMODULE}" )

  file( GLOB FILES_IN_DIRECTORY "${SUBMODULE_DIR}/${SELECTED_SUBMODULE}/*")

  if( FILES_IN_DIRECTORY )
    cmessage( STATUS "Git submodule ${SELECTED_SUBMODULE} is present" )
  else()
    cmessage( ERROR "Git submodule ${SELECTED_SUBMODULE} is not present, please checkout: \"git submodule update --init --remote --recursive\"" )
    cmessage( FATAL_ERROR "CMake fatal error." )
  endif()
endfunction( checkSubmodule )

###################################
# Simple C++ cmd line parser
###################################
cmessage( STATUS "Looking for simple-cpp-cmd-line-parser submodule..." )
checkSubmodule( simple-cpp-cmd-line-parser )
include_directories( ${SUBMODULE_DIR}/simple-cpp-cmd-line-parser/include )