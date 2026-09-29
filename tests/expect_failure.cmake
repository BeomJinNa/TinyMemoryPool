execute_process(
    COMMAND "${TEST_PROGRAM}" "${TEST_CASE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
    TIMEOUT 20
)

if(NOT "${result}" STREQUAL "86")
    message(FATAL_ERROR "Expected terminate exit 86, got ${result}\n${output}\n${error}")
endif()

string(FIND "${error}" "[TinyMemoryPool Fatal] ${EXPECTED_MESSAGE}" position)
if(position EQUAL -1)
    message(FATAL_ERROR "Expected allocator fatal diagnostic, got:\n${output}\n${error}")
endif()
