#pragma once 
#include<google/protobuf/service.h>
#include"zookeeperutil.h"

// 此类是继承自google::protobuf::RpcChannel
// 目的是为了给客户端进行方法调用的时候，统一接收的

class WJrpcchannel:public ::google::protobuf::RpcChannel{
private:
    int m_clientfd;
    std::string m_service_name;
    std::string m_service_menthod;
    std::string m_ip;
    int m_port;

private:
    bool newConnect(const char *ip, uint16_t port);
    

public:
    std::string queryServiceHost(ZkClinet *zkclient,std::string service_name,std::string service_method);

    void CallMethod(const ::google::protobuf::MethodDescriptor* method,
        ::google::protobuf::RpcController* controller, const ::google::protobuf::Message* request,
        ::google::protobuf::Message* response, ::google::protobuf::Closure* done) override;

    WJrpcchannel(bool connectionNow);
    virtual ~WJrpcchannel();
};