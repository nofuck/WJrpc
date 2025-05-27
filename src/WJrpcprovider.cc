#include"WJrpcprovider.h"
#include "WJrpcheader.pb.h"
#include<glog/logging.h>
#include <iostream> 
#include<muduo/net/TcpServer.h>
#include"WJrpcApplication.h"
#include"zookeeperutil.h"
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

WJrpcprovider::WJrpcprovider(){

}

WJrpcprovider::~WJrpcprovider(){
    //关闭事件监听
    event_loop.quit();
}

void WJrpcprovider::OnConnectionCallback(const TcpConnectionPtr &conn){
    if(!conn->connected()){
        conn->shutdown();
    }
}

void WJrpcprovider::OnMessageCallback(const TcpConnectionPtr& conn,Buffer* buffer,::muduo::Timestamp time){

    int len = buffer->readableBytes();
    std::string recv_buf = buffer->retrieveAsString(len);
    
    //数据如果有\0，c_str()会导致字符串被截断
    google::protobuf::io::ArrayInputStream array_input(recv_buf.data(),recv_buf.size());
    google::protobuf::io::CodedInputStream code_input(&array_input);
    // google::protobuf::io::CodedInputStream code_input(reinterpret_cast<const uint8_t*>(recv_buf.data()),recv_buf.size());

    //处理请求头
    uint32_t header_size;
    std::string header_str;
    WJrpc::rpcHeader rpcheader;
    std::string service_name;
    std::string servive_method;

    code_input.ReadVarint32(&header_size);

    google::protobuf::io::CodedInputStream::Limit msg_limit = code_input.PushLimit(header_size);
    code_input.ReadString(&header_str,header_size);
    // 恢复之前的限制，以便安全地继续读取其他数据
    code_input.PopLimit(msg_limit);

    if(!rpcheader.ParseFromString(header_str)){
        LOG(ERROR) << "解析 header 失败";
        return;
    }
    service_name = rpcheader.server_name();
    servive_method = rpcheader.server_method();
    uint32_t args_size = rpcheader.args_size();
    std::string args_str;
    if(!code_input.ReadString(&args_str,args_size)){
        LOG(ERROR) << "读取参数args 失败";
        return;
    }

    //查找内存中对应的service
    auto svc = service_map.find(service_name);
    if(svc == service_map.end()){
        LOG(ERROR) << service_name << " rpc服务不存在";
        return;
    }

    auto m_svc = svc->second.menthod_map.find(servive_method);
    if(m_svc == svc->second.menthod_map.end()){
        if(svc == service_map.end()){
            LOG(ERROR) << service_name <<  " 服务中的 " << servive_method << " rpc方法不存在";
            return;
        }
    }

    google::protobuf::Service *service = svc->second.service;
    const google::protobuf::MethodDescriptor *method_dsp = m_svc->second;

    google::protobuf::Message *request = service->GetRequestPrototype(method_dsp).New();
    if(!request->ParseFromString(args_str)){
        LOG(ERROR) <<"服务参数解析失败";
        return;
    }
    google::protobuf::Message *response = service->GetResponsePrototype(method_dsp).New();

    // 绑定回调函数，用于在方法调用完成后发送响应
    google::protobuf::Closure *done = google::protobuf::NewCallback<WJrpcprovider,const muduo::net::TcpConnectionPtr&,google::protobuf::Message *>
    (this,&WJrpcprovider::SendRpcMessage,conn,response);    
                                            
    // 在框架上根据远端RPC请求，调用当前RPC节点上发布的方法
    service->CallMethod(method_dsp,nullptr,request,response,done);
}

//回调函数，处理完方法逻辑之后，传输对应的回复给客户端，可以在这里对信息进行处理
void WJrpcprovider::SendRpcMessage(const muduo::net::TcpConnectionPtr &conn
    , google::protobuf::Message *response){
    std::string send_str;
    if(response->SerializeToString(&send_str)){
        conn->send(send_str);
    }else{
        LOG(ERROR) << "发送信息到客户端失败";
    }
}

//处理监听事件
void WJrpcprovider::Run(){

    std::string ip = WJrpcApplication::getInstance().getConfig().Load("rpcserverip");
    int port = atoi(WJrpcApplication::getInstance().getConfig().Load("rpcserverport").c_str());
    InetAddress inet_addr(ip,port);
    //创建tcpserver
    TcpServer server(&event_loop,inet_addr,"rpcserver");
    

    //绑定监听函数
    server.setConnectionCallback(std::bind(&WJrpcprovider::OnConnectionCallback,this,_1));
    server.setMessageCallback(std::bind(&WJrpcprovider::OnMessageCallback,this,_1,_2,_3));

    // 设置muduo库的线程数量
    server.setThreadNum(4);

    // 将当前RPC节点上要发布的服务全部注册到ZooKeeper上，让RPC客户端可以在ZooKeeper上发现服务
    ZkClinet zkclient;
    zkclient.Connect(); // 连接ZooKeeper服务器
    // service_name为永久节点，method_name为临时节点
    for(auto &service:service_map){
        std::string service_path = "/" + service.first;
        zkclient.Create(service_path.c_str(),nullptr,0);
        for(auto &method:service.second.menthod_map){
            std::string method_path = service_path + "/" + method.first;
            char data[128] = {};
            sprintf(data,"%s:%d",ip.c_str(),port);
            zkclient.Create(method_path.c_str(),data,sizeof(data),ZOO_EPHEMERAL);//创建临时节点
        }
    }
    LOG(INFO) << "服务 ip:port " << ip << ":" << port << "启动成功";

    //开始服务和事件监听
    server.start();
    event_loop.loop();
}

//注册服务对象及其方法，以便服务端能够处理客户端的RPC请求
void WJrpcprovider::NotifyService(google::protobuf::Service *service){
    //定义存储信息的数据结构
    ServiceInfo service_info;
    service_info.service = service;

    const google::protobuf::ServiceDescriptor* service_des = service->GetDescriptor();
    int method_count = service_des->method_count();
    std::string service_name = service_des->name();

    LOG(INFO) << "注册服务 " << service_name;

    for(int i = 0;i < method_count;i++){
        const google::protobuf::MethodDescriptor *method = service_des->method(i);
        LOG(INFO) << "注册服务方法 " << method->name();
        service_info.menthod_map.emplace(method->name(),method);
    }

    //将当前的服务对象存入map中
    service_map.emplace(service_name,service_info);
}