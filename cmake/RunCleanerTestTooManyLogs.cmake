file (TOUCH test_cleanup_info_20240730-111111.2222.toomanylogs)
file (TOUCH test_cleanup_info_20240730-111112.2222.toomanylogs)
file (TOUCH test_cleanup_info_20240730-111113.2222.toomanylogs)
file (TOUCH test_cleanup_info_20240730-111114.2222.toomanylogs)
execute_process (COMMAND ${LOGCLEANUP} RESULT_VARIABLE _RESULT)

if (NOT _RESULT EQUAL 0)
  message (FATAL_ERROR "Failed to run logcleanup_unittest (error: ${_RESULT})")
endif ()

file (GLOB LOG_FILES ${TEST_DIR}/test_cleanup_*.toomanylogs)

if ("test_cleanup_info_20240731-111111.2222.toomanylogs" IN_LIST LOG_FILES)
  message (SEND_ERROR "Expected old log file to be deleted")
endif ()

if ("test_cleanup_info_20240731-111112.2222.toomanylogs" IN_LIST LOG_FILES)
  message (SEND_ERROR "Expected old log file to be deleted")
endif ()

if ("test_cleanup_info_20240731-111113.2222.toomanylogs" IN_LIST LOG_FILES)
  message (SEND_ERROR "Expected old log file to be deleted")
endif ()

if (NOT "test_cleanup_info_20240731-111111.2222.toomanylogs" IN_LIST LOG_FILES)
  message (SEND_ERROR "Expected newest log file to be retained")
endif ()
