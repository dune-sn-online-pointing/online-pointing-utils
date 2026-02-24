###################
# link submodules
###################

cmessage( STATUS "Adding submodules to the project..." )
set( SUBMODULE_DIR ${CMAKE_SOURCE_DIR}/submodules )

# Function to make sure if a given submodule is present:
function( checkSubmodule )
  list( GET ARGV 0 SELECTED_SUBMODULE )
  cmessage( WARNING "Checking submodule: ${SELECTED_SUBMODULE}" )

  # A submodule directory may exist but still be effectively empty (e.g. only a
  # `.git` file with no checked-out worktree). Prefer a content-based check.
  set( SUBMODULE_PATH "${SUBMODULE_DIR}/${SELECTED_SUBMODULE}" )
  set( SUBMODULE_INCLUDE_DIR "${SUBMODULE_PATH}/include" )

  if( EXISTS "${SUBMODULE_INCLUDE_DIR}" )
    file( GLOB SUBMODULE_INCLUDE_CONTENT "${SUBMODULE_INCLUDE_DIR}/*" )
  else()
    set( SUBMODULE_INCLUDE_CONTENT "" )
  endif()

  if( SUBMODULE_INCLUDE_CONTENT )
    cmessage( STATUS "Git submodule ${SELECTED_SUBMODULE} is present" )
  else()
    cmessage( ERROR "Git submodule ${SELECTED_SUBMODULE} is not checked out (missing/empty include/). Please run: \"git submodule update --init --remote --recursive\"" )
    cmessage( FATAL_ERROR "CMake fatal error." )
  endif()
endfunction( checkSubmodule )

###################################
# Simple C++ cmd line parser
###################################
cmessage( STATUS "Looking for simple-cpp-cmd-line-parser submodule..." )
checkSubmodule( simple-cpp-cmd-line-parser )
include_directories( ${SUBMODULE_DIR}/simple-cpp-cmd-line-parser/include )


###################################
# Simple C++ logger
###################################
cmessage( STATUS "Looking for simple-cpp-logger submodule..." )
checkSubmodule( simple-cpp-logger )
include_directories( ${SUBMODULE_DIR}/simple-cpp-logger/include )
add_definitions( -D LOGGER_MAX_LOG_LEVEL_PRINTED=6 )
add_definitions( -D LOGGER_PREFIX_LEVEL=3 )
add_definitions( -D LOGGER_TIME_FORMAT="\\\"%d/%m/%Y %H:%M:%S"\\\" )
add_definitions( -D LOGGER_ENABLE_COLORS=1 )
add_definitions( -D LOGGER_ENABLE_COLORS_ON_USER_HEADER=1 )
#add_definitions( -D LOGGER_PREFIX_FORMAT="\\\"{TIME} {USER_HEADER} {FILELINE}"\\\" )
add_definitions( -D LOGGER_PREFIX_FORMAT="\\\"{TIME} {USER_HEADER}"\\\" )


###################################
# C++ generic toolbox
###################################
cmessage( STATUS "Looking for cpp-generic-toolbox submodule..." )
checkSubmodule( cpp-generic-toolbox )
include_directories( ${SUBMODULE_DIR}/cpp-generic-toolbox/include )
add_definitions( -D PROGRESS_BAR_FILL_TAG="\\\"SNOP\#"\\\" )
add_definitions( -D PROGRESS_BAR_ENABLE_RAINBOW=1 )

