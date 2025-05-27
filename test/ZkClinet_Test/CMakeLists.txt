cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找glog库
find_package(glog REQUIRED)

find_package(Protobuf REQUIRED)


add_executable(ZKClinet ZkClinet_test.cc 
../../src/zookeeperutil.cc 
../../src/WJrpcApplication.cc 
../../src/WJrpcconfig.cc
../../src/WJrpcchannel.cc
../../src/WJrpcheader.pb.cc
)

target_link_libraries(ZKClinet glog zookeeper_mt protobuf)

