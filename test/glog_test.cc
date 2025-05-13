#include<glog/logging.h>

void Trim(std::string &str){
    size_t index = str.find_first_not_of(' ');
    if(index != std::string::npos){
        str.erase(0,index);
    }else{
        str = "";
        return;
    }

    index = str.find_last_not_of(' ');
    if(index != std::string::npos){
        str.erase(index + 1);
    }else{
        str = "";
        return;
    }
}

int main(){
    google::InitGoogleLogging("glog_test");
    FLAGS_colorlogtostderr = true; // 启用彩色日志
    FLAGS_logtostderr = false;      // 默认输出标准错误
    FLAGS_log_dir = ".";
    LOG(INFO) << "info message ";
    LOG(WARNING) << "warning message ";
    std::string str = "    r  e   ";
    Trim(str);
    LOG(INFO) << str;
    return 0;
}