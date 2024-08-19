file (TOUCH test_cleanup_info_20240312-111111.2222.allloglevels)
file (TOUCH test_cleanup_warning_20240312-111111.2222.allloglevels)
file (TOUCH test_cleanup_error_20240312-111111.2222.allloglevels)
file (TOUCH test_cleanup_info_20240312-111111.2222.allloglevels.notalog)
execute_process (COMMAND ${LOGCLEANUP} RESULT_VARIABLE _RESULT)

if (NOT _RESULT EQUAL 0)
  message (FATAL_ERROR "Failed to run logcleanup_unittest (error: ${_RESULT})")
endif (NOT _RESULT EQUAL 0)

file (GLOB LOG_FILES ${TEST_DIR}/test_cleanup_*.allloglevels)
list (LENGTH LOG_FILES NUM_LOG_FILES)

if (WIN32)
  # On Windows open files cannot be removed and will result in a permission
  # denied error while unlinking such file. Therefore, the last file will be
  # retained.
  set (_expected 1)
 else (WIN32)
  set (_expected 0)
endif (WIN32)

if (NOT NUM_LOG_FILES EQUAL _expected)
  message (SEND_ERROR "Expected ${_expected} log file in log directory but found ${NUM_LOG_FILES}")
endif (NOT NUM_LOG_FILES EQUAL _expected)

file (GLOB NON_LOG_FILES ${TEST_DIR}/test_cleanup_*.notalog)
list (LENGTH NON_LOG_FILES NUM_NON_LOG_FILES)

if (NOT NUM_NON_LOG_FILES EQUAL 1)
  message (SEND_ERROR "Expected 1 non-log file in log directory but found ${NUM_NON_LOG_FILES}")
endif (NOT NUM_NON_LOG_FILES EQUAL 1)

file (REMOVE test_cleanup_info_20240312-111111.2222.allloglevels.notalog)
