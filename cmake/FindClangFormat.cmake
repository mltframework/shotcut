#
# .rst: FindClangFormat
# ---------------
#
# The module defines the following variables
#
# ``CLANGFORMAT_EXECUTABLE`` Path to clang-format executable
# ``CLANGFORMAT_FOUND`` True if the clang-format executable was found.
# ``CLANGFORMAT_VERSION`` The version of clang-format found
#
# Example usage:
#
# .. code-block:: cmake
#
# find_package(ClangFormat)
# if(CLANGFORMAT_FOUND)
# message("clang-format executable found: ${CLANGFORMAT_EXECUTABLE}\n" "version: ${CLANGFORMAT_VERSION}")
# endif()

include(FindPackageHandleStandardArgs)

function(_ClangFormat_get_version clangformat_version result_var clangformat_path)
  execute_process(
    COMMAND "${clangformat_path}" --version
    OUTPUT_VARIABLE full_clangformat_version
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE version_result
  )

  # full_clangformat_version sample: "clang-format version 3.9.1-4ubuntu3~16.04.1 (tags/RELEASE_391/rc2)"
  # clean clangformat_version sample: "3.9.1"
  string(REGEX
         REPLACE "[^0-9]*([.0-9]+).*"
                 "\\1"
                 clean_clangformat_version
                 "${full_clangformat_version}")

  set(${result_var} ${version_result} PARENT_SCOPE)
  set(${clangformat_version} ${clean_clangformat_version} PARENT_SCOPE)
endfunction()

function(_ClangFromat_version_validator version_match clangformat_path)
    if(NOT DEFINED ClangFormat_FIND_VERSION)
        set(${is_valid_version} TRUE PARENT_SCOPE)
    else()
        _ClangFormat_get_version(candidate_version version_result "${clangformat_path}")

        if(version_result)
            message(DEBUG "Unable to determine candidate clang-format version at ${clangformat_path}: ${version_result}")
        endif()

        find_package_check_version("${candidate_version}" valid_clangformat_version
            HANDLE_VERSION_RANGE
        )

        set(${version_match} "${valid_clangformat_version}" PARENT_SCOPE)
    endif()
endfunction()

find_program(CLANGFORMAT_EXECUTABLE
             NAMES clang-format
                   clang-format-16
                   clang-format-15
                   clang-format-14
                   clang-format-13
                   clang-format-12
                   clang-format-11
                   clang-format-10
             DOC "clang-format executable"
             VALIDATOR _ClangFromat_version_validator
            )
mark_as_advanced(CLANGFORMAT_EXECUTABLE)

if(CLANGFORMAT_EXECUTABLE)
  _ClangFormat_get_version(CLANGFORMAT_VERSION _Clangformat_version_result "${CLANGFORMAT_EXECUTABLE}")

  if(_Clangformat_version_result)
    set(CLANGFORMAT_FOUND FALSE)
    message(WARNING "Unable to determine clang-format version: ${_Clangformat_version_result}")
  else()
    set(CLANGFORMAT_FOUND TRUE)
  endif()
endif()

find_package_handle_standard_args(ClangFormat
  FOUND_VAR CLANGFORMAT_FOUND
  REQUIRED_VARS
    CLANGFORMAT_EXECUTABLE
    CLANGFORMAT_VERSION
  VERSION_VAR CLANGFORMAT_VERSION
)
