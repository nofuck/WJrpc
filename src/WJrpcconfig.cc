#include "WJrpcconfig.h"
#include <memory>
#include <algorithm>

WJrpcconfigManager::WJrpcconfigManager()
{
}

WJrpcconfigManager::~WJrpcconfigManager()
{
}

// 根据路径加载对应的文件内容
void WJrpcconfigManager::LoadConfigFile(const char *config_file)
{
    // 利用智能指针管理文件指针的回收
    std::unique_ptr<FILE, decltype(&fclose)> fp(fopen(config_file, "r"), &fclose);
    if (nullptr == fp)
    {
        LOG(ERROR) << "文件打开失败";
        exit(EXIT_FAILURE); // 退出程序
    }
    char buf[1024];
    while (fgets(buf, sizeof(buf), fp.get()) != nullptr)
    {
        std::string read_buf(buf);

        //检验是否为一整行
        if (read_buf.find('\n') == std::string::npos)
        {
            LOG(WARNING) << "配置文件一行内容太长 " << read_buf;
            continue;
        }
        //删除行尾
        DeleteEnd(read_buf);
        Trim(read_buf);

        // 忽略注释和空行
        if (read_buf[0] == '#' || read_buf.empty())
        {
            continue;
        }
        // 找到分界符
        size_t index = read_buf.find('=');
        if (index == std::string::npos)
        { // 忽略不合法的行
            LOG(WARNING) << "配置文件不合法 " << read_buf;
            continue;
        }

        std::string key, value;
        key = read_buf.substr(0, index);
        value = read_buf.substr(index + 1);

        if (key.empty() || value.empty())
        {
            LOG(WARNING) << "配置文件key或value为空 " << read_buf;
            continue;
        }

        // 去除行尾换行符
        size_t endlineindex = value.find('\n');
        value = value.substr(0, endlineindex);

        Trim(key);
        Trim(value);

        LOG(INFO) << "key = " << key << "   value = " << value;
        m_configs.insert({key, value});
    }
}

// 根据key查找对应的value
std::string WJrpcconfigManager::Load(const std::string &key)
{
    auto value = m_configs.find(key);
    if (value != m_configs.end())
    {
        return value->second;
    }
    // 不存在返回空字符串
    return "";
}

// 处理字符串前后空格
void WJrpcconfigManager::Trim(std::string &str)
{

    size_t index = str.find_first_not_of(' ');
    if (index != std::string::npos)
    {
        str.erase(0, index);
    }
    else
    {
        // 全是空格
        str = "";
        return;
    }

    index = str.find_last_not_of(' ');
    if (index != std::string::npos)
    {
        str.erase(index + 1);
    }
    else
    {
        str = "";
        return;
    }
}

// 删除末尾空格
void WJrpcconfigManager::DeleteEnd(std::string &str)
{
    if (str.empty())
        return;
    size_t index = str.find_last_not_of("\r\n");
    if (index != std::string::npos)
    {
        str.erase(index + 1);
    }
    else
    { // 全是\r\n的特殊字符
        str.clear();
    }
}