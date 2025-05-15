#pragma once 

#include"WJrpcconfig.h"
#include<mutex>

//单例模式
class WJrpcApplication{
private:
    static WJrpcconfigManager m_config;
    static WJrpcApplication *instance;
    static std::mutex m_mutex;

    //私有化初始构造函数
    WJrpcApplication(){}
    ~WJrpcApplication(){}
    //删除两种语义下的拷贝构造
    WJrpcApplication(const WJrpcApplication& ) = delete;
    WJrpcApplication(WJrpcApplication&& ) = delete;

public:
    //获取单例对象
    static WJrpcApplication& getInstance();
    //删除单例对象
    static void deleteInstance();
    //初始化相关信息
    static void Init(int argv,char **args);
    //获取配置信息
    WJrpcconfigManager& getConfig();

};