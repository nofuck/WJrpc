#pragma once

#include<zookeeper/zookeeper.h>
#include<string>
#include<mutex>
#include<condition_variable>
#include<glog/logging.h>

//zookeeper的客户端，用来与zookeeper服务器建立连接、获取数据等操作。
class ZkClinet{
private:
    //客户端句柄
    zhandle_t* m_handle;

    //互斥锁
    std::mutex m_zkmutex;
    //条件变量，用于线程间通信
    std::condition_variable m_zk;
    //连接状态
    bool is_connected;

public:
    ZkClinet();
    ~ZkClinet();
    //用于建立与zookeeper的连接
    void Connect();
    // 在指定路径创建节点
    void Create(const char *path, const char *data, int datalen, int state = 0); 
    //获取指定路径的信息
    std::string GetData(const char *path); 
    //zookeeper的全局监听函数
    void global_watcher(zhandle_t *zh, int type, int state, const char *path,void *watcherCtx);
};