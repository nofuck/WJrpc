#include"../src/WJrpcconfig.h"

int main(){
    WJrpcconfigManager config;
    config.LoadConfigFile("./config");

    return 0;
}