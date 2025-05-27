#pragma once 
#include<muduo/net/EventLoop.h>
#include<muduo/net/TcpServer.h>
#include<google/protobuf/message.h>
#include<google/protobuf/service.h>
#include<unordered_map>
using namespace ::muduo::net;

class WJrpcprovider{
private:
    EventLoop event_loop;
    struct ServiceInfo{
        google::protobuf::Service *service;
        std::unordered_map<std::string,const google::protobuf::MethodDescriptor*>menthod_map;
    };
    std::unordered_map<std::string,ServiceInfo>service_map;

private:
    void OnConnectionCallback(const TcpConnectionPtr&);
    void OnMessageCallback(const TcpConnectionPtr&,
        Buffer*,
        ::muduo::Timestamp);
    
    void SendRpcMessage(const muduo::net::TcpConnectionPtr &conn
        , google::protobuf::Message *response);

public:
    void Run();
    //注册rpc服务到zk里面
    void NotifyService(google::protobuf::Service *service);

    WJrpcprovider();
    ~WJrpcprovider();
    
};