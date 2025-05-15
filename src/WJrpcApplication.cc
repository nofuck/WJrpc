#include"WJrpcApplication.h"
#include<iostream>

WJrpcApplication *WJrpcApplication::instance = nullptr;
WJrpcconfigManager WJrpcApplication::m_config;
std::mutex WJrpcApplication::m_mutex;


//获取单例对象
WJrpcApplication& WJrpcApplication::getInstance(){
    if(nullptr == instance){
        std::lock_guard lock(m_mutex);
        if(nullptr == instance){
            instance = new WJrpcApplication();
            atexit(deleteInstance); //程序退出时，自动调用delete函数
        }
    }
    return *instance;
}
//删除单例对象
void WJrpcApplication::deleteInstance(){
    if(nullptr != instance){
        delete instance;
    }
}
//初始化相关信息
void WJrpcApplication::Init(int argv,char **args){

    if(argv < 2){
        std::cout << "格式: command -i <配置文件路径>" << std::endl;
        exit(EXIT_FAILURE);
    }
    int opt;
    char *config_path = nullptr;
    while(-1 != (opt = getopt(argv,args,"i:"))){
        switch (opt){
            case 'i':
                config_path = optarg;
                break;
            case ':':
                std::cout << "格式: command -i <配置文件路径>" << std::endl;
                exit(EXIT_FAILURE);
            case '?':
                std::cout << "格式: command -i <配置文件路径>" << std::endl;
                exit(EXIT_FAILURE);
            default:
                break;
        }
    }
    if(config_path != nullptr){
        m_config.LoadConfigFile(config_path);
    }else{
        exit(EXIT_FAILURE);
    }
    
}
//获取配置信息
WJrpcconfigManager& WJrpcApplication::getConfig(){
    return m_config;
}