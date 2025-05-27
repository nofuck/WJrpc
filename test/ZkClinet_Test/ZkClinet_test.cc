#include"../../src/zookeeperutil.h"
#include"../../src/WJrpcchannel.h"
#include <iostream>
#include<string>

int main(int argv,char **args){
    WJrpcApplication::Init(argv,args);

    ZkClinet *zkclinet = new ZkClinet();
    zkclinet->Connect();

    //持久节点
    zkclinet->Create("/testpath",nullptr,0);

    std::string data = "127.0.0.1:17";
    zkclinet->Create("/testpath/one11",data.c_str(),data.size(),ZOO_EPHEMERAL);

    // sleep(1);
    std::cout << zkclinet->GetData("/testpath/one11") << std::endl;

    std::cout << zkclinet->GetData("/testpath") << std::endl;

    std::cout << zkclinet->GetData("/testpat") << std::endl;

    WJrpcchannel rpcchannel(false);
    LOG(INFO) << "host: " << rpcchannel.queryServiceHost(zkclinet,"testpath","one11") << std::endl;
}