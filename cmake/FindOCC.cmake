find_path(OCC_CMAKE_CONFIG NAMES OpenCASCADEConfig.cmake
  HINTS ${OCC_ROOT} ${CAS_ROOT}
  PATHS ENV LD_LIBRARY_PATH DYLD_LIBRARY_PATH
  # ubuntu path
  PATHS /usr/local/Cellar/opencascade/7.2.0/
  PATHS /usr/
  PATH_SUFFIXES lib Lib cmake cmake/opencascade lib/cmake/opencascade
  NO_DEFAULT_PATH)

if(OCC_CMAKE_CONFIG STREQUAL "OCC_CMAKE_CONFIG-NOTFOUND")
  set(OCC_FOUND FALSE)
else()
  set(OCC_FOUND TRUE)
  message(STATUS "Found OCC in ${OCC_CMAKE_CONFIG}")
  include(${OCC_CMAKE_CONFIG}/OpenCASCADEConfig.cmake)
endif()
