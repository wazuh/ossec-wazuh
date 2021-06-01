# Copyright (C) 2015-2021, Wazuh Inc.
#
# This program is free software; you can redistribute it
# and/or modify it under the terms of the GNU General Public
# License (version 2) as published by the FSF - Free Software
# Foundation.

# Tests list and flags
list(APPEND tests_names "test_wm_office365")
list(APPEND tests_flags "-Wl,--wrap,_mwarn -Wl,--wrap,_merror -Wl,--wrap,_mtinfo\
                        -Wl,--wrap,access")
list(APPEND use_shared_libs 1)

list(APPEND shared_libs "../scheduling/wmodules_scheduling_helpers.h")
list(APPEND shared_libs "../scheduling/wmodules_scheduling_helpers.c")

# Generate wazuh modules library
file(GLOB office365 ../../../wazuh_modules/*.o)
list(REMOVE_ITEM office365 ../../../wazuh_modules/main.o)

add_library(OFFICE365_O STATIC ${office365})

set_source_files_properties(
  ${office365}
  PROPERTIES
  EXTERNAL_OBJECT true
  GENERATED true
  )

set_target_properties(
  OFFICE365_O
  PROPERTIES
  LINKER_LANGUAGE C
  )

target_link_libraries(OFFICE365_O ${WAZUHLIB} ${WAZUHEXT} -lpthread)


# Compiling tests
list(LENGTH tests_names count)
math(EXPR count "${count} - 1")
foreach(counter RANGE ${count})
    list(GET tests_names ${counter} test_name)
    list(GET tests_flags ${counter} test_flags)
    list(GET use_shared_libs ${counter} use_libs)

    if(use_libs EQUAL "1")
      add_executable(${test_name} ${test_name}.c ${shared_libs})
    else ()
      add_executable(${test_name} ${test_name}.c)
    endif()

    target_link_libraries(
        ${test_name}
        ${WAZUHLIB}
        ${WAZUHEXT}
        OFFICE365_O
        -lcmocka
        -ldl
        -fprofile-arcs
        -ftest-coverage
    )
    if(NOT test_flags STREQUAL " ")
        target_link_libraries(
            ${test_name}
            ${test_flags}
        )
    endif()
    add_test(${test_name} ${test_name})
endforeach()
