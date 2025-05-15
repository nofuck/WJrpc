#include"zookeeperutil.h"

//互斥锁
std::mutex ZkClinet::m_zkmutex;
//条件变量，用于线程间通信
std::condition_variable ZkClinet::m_zk;
//连接状态
bool ZkClinet::is_connected = false;

ZkClinet::ZkClinet():m_handle(nullptr){

}

ZkClinet::~ZkClinet(){
    if(m_handle != nullptr){
        zookeeper_close(m_handle);
        LOG(INFO) << " 断开 Zookeeper 连接";
    }
}

//用于建立与zookeeper的连接
void ZkClinet::Connect(){
    
    //获取zookeeper服务端的地址
    std::string host = WJrpcApplication::getInstance().getConfig().Load("zookeeperip");
    std::string port = WJrpcApplication::getInstance().getConfig().Load("zookeeperport");
    std::string connstr = host + ":" + port;
    LOG(INFO) << "加载host和post成功,host: "<< host << " port: " << port ;

    if(host.empty() || port.empty()){
        LOG(ERROR) << " zookeeper 连接失败,原因host或者port为空";
        exit(EXIT_FAILURE);
    }

    //创建zk客户端，用于连接zk服务器(异步建立与服务器的连接)
    m_handle = zookeeper_init(connstr.c_str(),global_watcher,2000,nullptr,nullptr,0);
    if(nullptr == m_handle){
        LOG(ERROR) << "zookeeper_init 失败";
        exit(EXIT_FAILURE);
    }

    //加锁等待初始化完成
    std::unique_lock lock(m_zkmutex);
    //阻塞等待连接建立成功
    m_zk.wait(lock, []() { return is_connected == true; });
    LOG(INFO) << "zookeeper 初始化完成";
    
}

// 在指定路径创建节点，state = 0：普通持久节点，ZOO_EPHEMERAL：临时节点（会话断开自动删除），ZOO_SEQUENCE：顺序节点（自动追加序号）
void ZkClinet::Create(const char *path, const char *data, int datalen, int state){
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);

    //path
    int flag = zoo_exists(m_handle,path,0,nullptr);
    if(ZNONODE == flag){//节点不存在
        //记录创建状态
        flag = zoo_create(m_handle, path, data, datalen, &ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);
        if(ZOK == flag){//创建成功
            LOG(INFO) << "zookeeper 创建节点成功 path:"<< path;
        }else{
            LOG(ERROR) << "zookeeper 创建节点失败 path:" << path;
            exit(EXIT_FAILURE);
        }
    }
    
}

//获取指定路径的信息
std::string ZkClinet::GetData(const char *path){
    //通过zooget获取对应路径信息
    char buf[128];
    int bufferlen = sizeof(buf);
    std::string answer = "";
    int flag = zoo_get(m_handle, path, 0, buf, &bufferlen, nullptr);
    if(ZOK == flag){
        //返回获取得到的信息
        //必须添加'\0'，因为zoo_get不会在末尾添加。
        buf[bufferlen] = '\0';
        answer = std::string(buf);
        LOG(INFO) << "获取节点信息成功 value: " << answer;
    }else{
        LOG(ERROR) << "获取节点信息失败 path:" << path;
    }
    return answer;
}

void ZkClinet::global_watcher(zhandle_t *zh, int type, int state, const char *path,void *watcherCtx){
    
    if(type == ZOO_SESSION_EVENT){//处理连接事件
        if(state == ZOO_CONNECTED_STATE){//已连接状态
            std::lock_guard lock(m_zkmutex);
            is_connected = true;
        }
    }
    //唤醒所有因 m_zkmutex 等待的线程
    m_zk.notify_all();
}