#include "WJrpccontroller.h"

WJrpccontroller::WJrpccontroller() : m_failed(false), m_errText("")
{
}

// 重置信息为初始值
void WJrpccontroller::Reset()
{
    m_failed = false;
    m_errText = "";
}

// 判断连接是否失败
bool WJrpccontroller::Failed() const
{
    return m_failed == true;
}

std::string WJrpccontroller::ErrorText() const
{
    return m_errText;
}

// 设置RPC调用失败，并记录失败原因
void WJrpccontroller::SetFailed(const std::string &reason)
{
    m_failed = true;
    m_errText = reason;
}

// 目前未实现具体的功能
void WJrpccontroller::StartCancel()
{
}

bool WJrpccontroller::IsCanceled() const
{
    return false;
}

void WJrpccontroller::NotifyOnCancel(google::protobuf::Closure *callback)
{
}