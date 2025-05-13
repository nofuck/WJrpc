#include"zookeeperutil.h"


ZkClinet::ZkClinet():m_handle(nullptr),is_connected(false){

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
    std::string host = "";
    std::string port = "";
}

// 在指定路径创建节点
void ZkClinet::Create(const char *path, const char *data, int datalen, int state = 0){

}

//获取指定路径的信息
std::string ZkClinet::GetData(const char *path){

}

void ZkClinet::global_watcher(zhandle_t *zh, int type, int state, const char *path,void *watcherCtx){
    
    if(type == ZOO_SESSION_EVENT){//处理连接事件
        if(state == ZOO_CONNECTED_STATE){//已连接状态
            std::unique_lock lock(m_zkmutex);
            is_connected = true;
        }
    }
    //唤醒所有因 m_zkmutex 等待的线程
    m_zk.notify_all();
}