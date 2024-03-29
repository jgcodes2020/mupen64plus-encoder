if (REFRESH_MUPEN_API OR NOT MUPEN_API_DOWNLOADED)
  set(MUPEN_API_DOWNLOADED TRUE CACHE INTERNAL "True if Mupen has been downloaded.")
  set(branch_name "capture-backend")
  unset(REFRESH_MUPEN_API CACHE)
  # Download rerecording (in case of API differences)
  file(DOWNLOAD "https://github.com/jgcodes2020/mupen64plus-core-rr/archive/${branch_name}.tar.gz"
    "${PROJECT_BINARY_DIR}/CMakeFiles/CMakeTmp/mupen.tar.gz"
  )
  # Extract the correct dir
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar -xzvf "mupen.tar.gz" mupen64plus-core-rr-${branch_name}/src/api
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/CMakeFiles/CMakeTmp"
  )
  # Clear old API files
  file(REMOVE_RECURSE 
    "${PROJECT_BINARY_DIR}/mupen64plus-api_download/include/mupen64plus")
  # List API headers
  file(GLOB api_header_list 
    RELATIVE "${PROJECT_BINARY_DIR}/CMakeFiles/CMakeTmp/mupen64plus-core-rr-${branch_name}/src/api/" 
    "${PROJECT_BINARY_DIR}/CMakeFiles/CMakeTmp/mupen64plus-core-rr-${branch_name}/src/api/m64p_*.h"
  )

  file(MAKE_DIRECTORY 
    "${PROJECT_BINARY_DIR}/mupen64plus-api_download/include/mupen64plus")
  foreach (i IN LISTS api_header_list)
    file(RENAME
      "${PROJECT_BINARY_DIR}/CMakeFiles/CMakeTmp/mupen64plus-core-rr-${branch_name}/src/api/${i}"
      "${PROJECT_BINARY_DIR}/mupen64plus-api_download/include/mupen64plus/${i}"
    )
  endforeach()

  # Clear temporary files
  file(REMOVE 
    "${PROJECT_BINARY_DIR}/CMakeFiles/CMakeTmp/mupen.tar.gz")
  file(REMOVE_RECURSE 
    "${PROJECT_BINARY_DIR}/CMakeFiles/CMakeTmp/mupen64plus-core-rr-${branch_name}")
endif()

add_library(m64p_api_internal_do_not_use INTERFACE)
target_include_directories(m64p_api_internal_do_not_use INTERFACE "${PROJECT_BINARY_DIR}/mupen64plus-api_download/include/")

add_library(m64p::api ALIAS m64p_api_internal_do_not_use)