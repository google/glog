cmake_minimum_required (VERSION 3.16)
project (glog_log_severity LANGUAGES CXX)

find_package (glog REQUIRED NO_MODULE)

add_executable (glog_dcheck glog_dcheck.cc)
target_link_libraries (glog_dcheck PRIVATE glog::glog)
