set (RUNS 3)

foreach (iter RANGE 1 ${RUNS})
  set (ENV{GOOGLE_LOG_DIR} ${TEST_DIR})
  execute_process (COMMAND ${LOGCLEANUP} RESULT_VARIABLE _RESULT)

  if (NOT _RESULT EQUAL 0)
    message (FATAL_ERROR "Failed to run logcleanup_unittest (error: ${_RESULT})")
  endif (NOT _RESULT EQUAL 0)
endforeach (iter)

file (GLOB LOG_FILES ${TEST_DIR}/*.foobar)
list (LENGTH LOG_FILES NUM_FILES)

if (WIN32)
  # On Windows open files cannot be removed and will result in a permission
  # denied error while unlinking such file. Therefore, the last file will be
  # retained.
  set (_expected 1)
 else (WIN32)
  set (_expected 0)
endif (WIN32)

if (NOT NUM_FILES EQUAL _expected)
  message (SEND_ERROR "Expected ${_expected} log file in log directory but found ${NUM_FILES}")
endif (NOT NUM_FILES EQUAL _expected)
