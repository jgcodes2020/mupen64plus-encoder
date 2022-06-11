cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(mupen64plus-encoder
  LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(M64PApi)

add_executable(main
  "src/main.cpp"
  "src/m64pp/fixed_string.hpp"
  "src/m64pp/ntuple.hpp"
  "src/m64pp/core.hpp"
  "src/oslib/strconv.hpp"
  "src/oslib/plibdl.hpp"
  "src/config/paths.hpp"
)
target_compile_features(main PRIVATE cxx_std_20)
target_link_libraries(main PRIVATE m64p::api)
