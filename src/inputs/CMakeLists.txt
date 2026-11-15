# modified using Claude Code with gpt-6-luna by H. Schellman on 2026-11-14 at 22:57:30 UTC
# modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-14 at 23:23:57 UTC
# modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-14 at 23:27:11 UTC
# modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 18:06:25 UTC
# modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 19:46:11 UTC
# Library wrapping the ROOT MakeClass tree readers in this directory.
# Local/standalone: this file does not modify any other CMakeLists.txt.
# It can be pulled in with add_subdirectory(...) from the top-level build,
# or configured on its own (it will locate ROOT itself in that case).

# --- Standalone bootstrap (skipped when included as a subdirectory) ---
if( CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR )
  cmake_minimum_required( VERSION 3.17 )
  project( inputs CXX )

  set( CMAKE_CXX_STANDARD 14 )
  set( CMAKE_CXX_STANDARD_REQUIRED ON )

  option( USE_STATIC_LINKS "Use static links for generated libraries" ON )
endif()

# Make sure ROOT is available (the top-level build normally provides this).
if( NOT ROOT_LIBRARIES )
  find_package( ROOT REQUIRED COMPONENTS RIO Net Tree )
  include( ${ROOT_USE_FILE} )
endif()

# --- Library ---
set( LIB_NAME "inputsLibs" )

set( SRC_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/mcneutrinos.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mcparticles.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/mctruths.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/simides.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/tpstream.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/FileMap.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Digger.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/TPSClass.cpp
)

if( USE_STATIC_LINKS )
  add_library( ${LIB_NAME} STATIC ${SRC_FILES} )
else()
  add_library( ${LIB_NAME} SHARED ${SRC_FILES} )
endif()

target_include_directories(
  ${LIB_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib
  ${CMAKE_CURRENT_SOURCE_DIR}/../objects
  # Submodule headers pulled in via ../lib/Global.h (Logger.h, CmdLineParser.h,
  # GenericToolbox.h). The top-level build provides these; add them here so a
  # standalone configure of this directory also finds them.
  ${CMAKE_CURRENT_SOURCE_DIR}/../../submodules/simple-cpp-logger/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../../submodules/simple-cpp-cmd-line-parser/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../../submodules/cpp-generic-toolbox/include
)

target_link_libraries(
  ${LIB_NAME} PUBLIC
  ${ROOT_LIBRARIES}
)

# FileMap.cpp references the global `debugMode` defined in src/lib (globalLib).
# When built as part of the top-level project, link it so the symbol resolves.
# In a standalone configure of this directory globalLib does not exist, so this
# is guarded and the caller must otherwise provide that symbol.
if( TARGET globalLib )
  target_link_libraries( ${LIB_NAME} PUBLIC globalLib )
endif()

add_executable( read_trigger_trees ${CMAKE_CURRENT_SOURCE_DIR}/read_trigger_trees.cpp )
target_link_libraries( read_trigger_trees PRIVATE ${LIB_NAME} ${ROOT_LIBRARIES} )
if( NOT TARGET globalLib )
  target_compile_definitions( read_trigger_trees PRIVATE INPUTS_STANDALONE )
endif()
get_filename_component( ONLINE_POINTING_UTILS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE )
set_target_properties(
  read_trigger_trees PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${ONLINE_POINTING_UTILS_ROOT}"
)

install( TARGETS ${LIB_NAME} DESTINATION lib )
