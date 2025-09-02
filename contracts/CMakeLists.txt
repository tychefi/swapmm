cmake_minimum_required(VERSION 3.5)

project(contracts)

find_package(flon.cdt)

set(CDT_VERSION_MIN "0.3")
set(CDT_VERSION_SOFT_MAX "2.0")
# set(CDT_VERSION_HARD_MAX "")

# Check the version of CDT
if(SYSTEM_ENABLE_CDT_VERSION_CHECK)
  set(VERSION_MATCH_ERROR_MSG "")
  CDT_CHECK_VERSION(VERSION_OUTPUT "${CDT_VERSION}" "${CDT_VERSION_MIN}" "${CDT_VERSION_SOFT_MAX}"
                      "${CDT_VERSION_HARD_MAX}" VERSION_MATCH_ERROR_MSG)
  if(VERSION_OUTPUT STREQUAL "MATCH")
    message(STATUS "Using CDT version ${CDT_VERSION}")
  elseif(VERSION_OUTPUT STREQUAL "WARN")
    message(
      WARNING
        "Using CDT version ${CDT_VERSION} even though it exceeds the maximum supported version of ${CDT_VERSION_SOFT_MAX}; continuing with configuration, however build may fail.\nIt is recommended to use CDT version ${CDT_VERSION_SOFT_MAX}.x"
    )
  else() # INVALID OR MISMATCH
    message(
      FATAL_ERROR
        "Found CDT version ${CDT_VERSION} but it does not satisfy version requirements: ${VERSION_MATCH_ERROR_MSG}\nPlease use CDT version ${CDT_VERSION_SOFT_MAX}.x"
    )
  endif(VERSION_OUTPUT STREQUAL "MATCH")
endif()

if(NOT "${CONTRACT_COMPILE_OPTIONS_FILE}" STREQUAL "")
  file(READ ${CONTRACT_COMPILE_OPTIONS_FILE} CONTRACT_COMPILE_OPTIONS)
  message(STATUS "Using CONTRACT_COMPILE_OPTIONS=${CONTRACT_COMPILE_OPTIONS}")
  add_compile_options("${CONTRACT_COMPILE_OPTIONS}")
endif()

include_directories(${CONTRACT_VERSION_DIR})

add_subdirectory(libs)
add_subdirectory(bot.mm)
add_subdirectory(tokenx.mm)
add_subdirectory(buylowsellhi)