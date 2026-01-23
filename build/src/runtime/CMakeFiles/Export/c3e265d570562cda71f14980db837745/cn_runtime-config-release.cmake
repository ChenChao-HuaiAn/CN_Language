#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "cn_runtime::cn_runtime" for configuration "Release"
set_property(TARGET cn_runtime::cn_runtime APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(cn_runtime::cn_runtime PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/cn_runtime.lib"
  )

list(APPEND _cmake_import_check_targets cn_runtime::cn_runtime )
list(APPEND _cmake_import_check_files_for_cn_runtime::cn_runtime "${_IMPORT_PREFIX}/lib/cn_runtime.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
