#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "cn_runtime::cn_runtime" for configuration ""
set_property(TARGET cn_runtime::cn_runtime APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(cn_runtime::cn_runtime PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libcn_runtime.a"
  )

list(APPEND _cmake_import_check_targets cn_runtime::cn_runtime )
list(APPEND _cmake_import_check_files_for_cn_runtime::cn_runtime "${_IMPORT_PREFIX}/lib/libcn_runtime.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
