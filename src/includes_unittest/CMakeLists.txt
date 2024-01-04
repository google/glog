cmake_minimum_required (VERSION 3.16)
project (glog_includes LANGUAGES CXX)

find_package (glog REQUIRED NO_MODULE)

add_executable (glog_includes_logging glog_includes_logging.cc)
target_link_libraries (glog_includes_logging PRIVATE glog::glog)

add_executable (glog_includes_vlog_is_on glog_includes_vlog_is_on.cc)
target_link_libraries (glog_includes_vlog_is_on PRIVATE glog::glog)

add_executable (glog_includes_raw_logging glog_includes_raw_logging.cc)
target_link_libraries (glog_includes_raw_logging PRIVATE glog::glog)

add_executable (glog_includes_stl_logging glog_includes_stl_logging.cc)
target_link_libraries (glog_includes_stl_logging PRIVATE glog::glog)
