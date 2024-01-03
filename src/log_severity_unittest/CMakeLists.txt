cmake_minimum_required (VERSION 3.16)
project (glog_log_severity LANGUAGES CXX)

find_package (glog REQUIRED NO_MODULE)

add_executable (glog_log_severity_constants glog_log_severity_constants.cc)
target_link_libraries (glog_log_severity_constants PRIVATE glog::glog)

add_executable (glog_log_severity_conversion glog_log_severity_conversion.cc)
target_link_libraries (glog_log_severity_conversion PRIVATE glog::glog)
