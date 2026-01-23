#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "cn_runtime::cn_runtime" for configuration "MinSizeRel"
set_property(TARGET cn_runtime::cn_runtime APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(cn_runtime::cn_runtime PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "C"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/cn_runtime.lib"
  )

list(APPEND _cmake_import_check_targets cn_runtime::cn_runtime )
list(APPEND _cmake_import_check_files_for_cn_runtime::cn_runtime "${_IMPORT_PREFIX}/lib/cn_runtime.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
