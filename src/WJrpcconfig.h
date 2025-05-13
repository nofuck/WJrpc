#pragma once

#include<unordered_map>
#include<string>
#include<glog/logging.h>

class WJrpcconfigManager{
private:
    std::unordered_map<std::string,std::string> m_configs;
    //处理字符串前后空格
    void Trim(std::string &str);
    //删除末尾空格
    void DeleteEnd(std::string &str);
public:
    WJrpcconfigManager();
    ~WJrpcconfigManager();
    //根据路径加载对应的文件内容
    void LoadConfigFile(const char* path);
    //根据key查找对应的value
    std::string Load(const std::string& key);
    

};