#include "WJrpcchannel.h"
#include <mutex>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <google/protobuf/descriptor.h> // Include the header for MethodDescriptor
#include <google/protobuf/message.h>    // Include the header for Message
#include "WJrpcheader.pb.h"

std::mutex g_query_mutex;

// RPC调用的核心方法，负责将客户端的请求序列化并发送到服务端，同时接收服务端的响应
void WJrpcchannel::CallMethod(const ::google::protobuf::MethodDescriptor *method,
                              ::google::protobuf::RpcController *controller, const ::google::protobuf::Message *request,
                              ::google::protobuf::Message *response, ::google::protobuf::Closure *done)
{

    // 验证连接是否建立
    if (-1 == m_clientfd)
    {
        const google::protobuf::ServiceDescriptor *sd = method->service();
        m_service_name = sd->name();
        m_service_menthod = method->name();

        // 通过zk客户端查询对应的rpc服务地址
        ZkClinet zkclinet;
        zkclinet.Connect();
        std::string host = queryServiceHost(&zkclinet, m_service_name, m_service_menthod);
        LOG(INFO) << m_service_name << "/" << m_service_menthod << " 对应的host地址"
                  << host;

        int idx = host.find(":");
        m_ip = host.substr(0, idx);
        m_port = atoi(host.substr(idx + 1).c_str());
        bool flag = newConnect(m_ip.c_str(), m_port);

        if (flag)
        {
            LOG(INFO) << "连接rpc服务器成功 " << m_ip << " " << m_port;
        }
        else
        {
            LOG(ERROR) << "连接rpc服务器失败" << m_ip << " " << m_port;
            return;
        }
    }

    // 将请求参数序列化为字符串
    std::string args_str;
    if (!request->SerializeToString(&args_str))
    {
        LOG(ERROR) << " 请求参数序列化失败";
        controller->SetFailed("serialize request_args fail");
        return;
    }

    // 定义RPC请求的头部信息
    WJrpc::rpcHeader rpcheader;
    rpcheader.set_server_name(m_service_name);
    rpcheader.set_server_method(m_service_menthod);
    rpcheader.set_args_size(args_str.size());

    // 将RPC头部信息序列化为字符串
    std::string header_str;
    if (!rpcheader.SerializeToString(&header_str)) 
    {
        LOG(ERROR) << " 请求头序列化失败";
        controller->SetFailed("serialize request_header fail");
        return;
    }

    // 构造RPC请求报文，报文格式:头长度，头，报文
    std::string send_rpc_str;
    google::protobuf::io::StringOutputStream string_output(&send_rpc_str);
    google::protobuf::io::CodedOutputStream code_output(&string_output);
    code_output.WriteVarint32(static_cast<uint32_t>(header_str.size()));
    code_output.WriteString(header_str);
    send_rpc_str += args_str;

    // 发送RPC请求到服务器
    if (-1 == send(m_clientfd, send_rpc_str.c_str(), send_rpc_str.size(), 0))
    {
        close(m_clientfd);
        char errortxt[512] = {};
        strerror_r(errno, errortxt, sizeof(errortxt));
        LOG(ERROR) << "发送信息到服务器失败， strerror: " << errortxt;
        return;
    }

    // 接收服务器的响应
    char recv_buf[1024] = {0};
    int recv_size = 0;
    if (-1 == (recv_size = recv(m_clientfd, recv_buf, 1024, 0)))
    {
        close(m_clientfd);
        char errortxt[512] = {};
        strerror_r(errno, errortxt, sizeof(errortxt));
        LOG(ERROR) << "接收服务器信息失败， strerror: " << errortxt;
        return;
    }

    // 将接收到的响应数据反序列化为resopne对象
    if (!response->ParseFromArray(recv_buf, recv_size))
    {
        close(m_clientfd);
        char errortxt[512] = {};
        strerror_r(errno, errortxt, sizeof(errortxt));
        LOG(ERROR) << "反序列化response失败， strerror: " << errortxt;
        return;
    }

    // 关闭soket连接
    close(m_clientfd);
}

// 用于创建与服务端的连接
bool WJrpcchannel::newConnect(const char *ip, uint16_t port)
{
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);

    if (-1 == clientfd)
    {
        char errtxt[512] = {0};
        std::cout << "socket error" << strerror_r(errno, errtxt, sizeof(errtxt)) << std::endl; // 打印错误信息
        LOG(ERROR) << "客户端 socket创建失败, 错误信息:" << errtxt;
        return false;
    }

    // 创建服务器相关信息
    struct sockaddr_in server_addr;
    server_addr.sin_addr.s_addr = inet_addr(ip); // ip地址
    server_addr.sin_port = htons(port);
    server_addr.sin_family = AF_INET;

    if (-1 == connect(clientfd, (struct sockaddr *)&server_addr, sizeof(server_addr)))
    {
        close(clientfd);
        char errortext[512] = {0};
        strerror_r(errno, errortext, sizeof(errortext)); // 记录相关信息
        LOG(ERROR) << "连接服务器失败";
        return false;
    }

    // 连接成功
    m_clientfd = clientfd;
    return true;
}

// 查询zookeeper上对应的host
std::string WJrpcchannel::queryServiceHost(ZkClinet *zkclient, std::string service_name, std::string service_method)
{
    // 拼接host名字
    std::string query_path = "/" + service_name + "/" + service_method;
    LOG(INFO) << "zookeeper 查询路径: " << query_path << " (服务名: " << service_name << ", 方法名: " << service_method << ")";

    if (zkclient == nullptr)
    {
        LOG(ERROR) << "zookeeper 客户端无效";
        return "";
    }

    std::string host;
    try
    {
        // 加锁，防止多个同时查询
        std::lock_guard lock(g_query_mutex);
        host = zkclient->GetData(query_path.c_str());
    }
    catch (const std::exception &e)
    {
        LOG(ERROR) << "查询 zookeeper 数据时发生异常: " << e.what();
    }
    catch (...)
    {
        LOG(ERROR) << "查询 zookeeper 数据时发生未知异常";
    }

    if (host == "")
    {
        LOG(ERROR) << query_path << " 查找节点值失败";
    }
    else if (host.find(":") == std::string::npos)
    {
        LOG(WARNING) << query_path << " 值错误,缺少分隔符";
        host = "";
    }
    else
    {
        LOG(INFO) << "查找host成功,host=" << host;
    }

    return host;
}

WJrpcchannel::WJrpcchannel(bool connectionNow) : m_clientfd(-1), m_port(0)
{
    if (!connectionNow)
        return;

    int connCount = 3;
    bool flag = newConnect(m_ip.c_str(), m_port);
    while (!flag && connCount > 0)
    {
        flag = newConnect(m_ip.c_str(), m_port);
    }
}

WJrpcchannel::~WJrpcchannel()
{
}