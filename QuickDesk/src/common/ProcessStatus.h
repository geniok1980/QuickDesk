// Copyright 2026 QuickDesk Authors
// Process and Server status enums

#ifndef QUICKDESK_COMMON_PROCESSSTATUS_H
#define QUICKDESK_COMMON_PROCESSSTATUS_H

#include <QObject>

namespace quickdesk {

/**
 * @brief Process status enum
 */
class ProcessStatus : public QObject {
    Q_OBJECT
    
public:
    enum Status {
        NotStarted,     // 未启动
        Starting,       // 启动中
        Running,        // 已启动
        Failed,         // 启动失败
        Restarting      // 重启中
    };
    Q_ENUM(Status)
    
private:
    ProcessStatus() = delete;
};

/**
 * @brief Server connection status enum
 */
class ServerStatus : public QObject {
    Q_OBJECT
    
public:
    enum Status {
        Disconnected,   // 未连接
        Connecting,     // 连接中
        Connected,      // 已连接
        Failed,         // 连接失败
        Reconnecting    // 重连中
    };
    Q_ENUM(Status)
    
private:
    ServerStatus() = delete;
};

/**
 * @brief WebRTC P2P connection status enum
 */
class RtcStatus : public QObject {
    Q_OBJECT
    
public:
    enum Status {
        Disconnected,   // 未连接
        Connecting,     // 连接中
        Connected,      // 已连接
        Failed          // 连接失败
    };
    Q_ENUM(Status)
    
private:
    RtcStatus() = delete;
};

/**
 * @brief Host launch mode enum
 */
class HostLaunchMode : public QObject {
    Q_OBJECT
    
public:
    enum Mode {
        Unknown,        // Not determined yet
        Service,        // Running via Windows Service
        ChildProcess    // Running as a child process of Qt GUI
    };
    Q_ENUM(Mode)
    
private:
    HostLaunchMode() = delete;
};

} // namespace quickdesk

#endif // QUICKDESK_COMMON_PROCESSSTATUS_H
