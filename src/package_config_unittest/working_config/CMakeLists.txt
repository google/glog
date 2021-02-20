cmake_minimum_required (VERSION 3.1)
project (glog_package_config)

find_package (glog REQUIRED NO_MODULE)

add_executable (glog_package_config glog_package_config.cc)

target_link_libraries (glog_package_config PRIVATE glog::glog)
