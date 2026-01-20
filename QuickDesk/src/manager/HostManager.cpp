// Copyright 2026 QuickDesk Authors

#include "HostManager.h"
#include "NativeMessaging.h"
#include <QDebug>
#include <QJsonArray>

namespace quickdesk {

HostManager::HostManager(QObject* parent)
    : QObject(parent)
{
}

void HostManager::setMessaging(NativeMessaging* messaging)
{
    if (m_messaging) {
        QObject::disconnect(m_messaging, nullptr, this, nullptr);
    }

    m_messaging = messaging;

    if (m_messaging) {
        connect(m_messaging, &NativeMessaging::messageReceived,
                this, &HostManager::onMessageReceived);
        connect(m_messaging, &NativeMessaging::errorOccurred,
                this, &HostManager::onMessagingError);
    }
}

void HostManager::connectToServer(const QString& serverUrl,
                                  const QString& deviceId,
                                  const QString& accessCode)
{
    if (!m_messaging || !m_messaging->isReady()) {
        emit errorOccurred("NOT_READY", "Host process is not ready");
        return;
    }

    QJsonObject message;
    message["type"] = "connect";
    // Use correct field names expected by C++ Host
    message["signalingServerUrl"] = serverUrl;
    
    if (!deviceId.isEmpty()) {
        message["deviceId"] = deviceId;
    }
    
    // accessCodeValue is required by C++ Host
    // If not provided, generate a temporary one or use empty string
    QString code = accessCode.isEmpty() ? QString() : accessCode;
    message["accessCodeValue"] = code;

    qInfo() << "Sending connect message to host:" << serverUrl 
            << "deviceId:" << deviceId;
    m_messaging->sendMessage(message);
}

void HostManager::disconnectFromServer()
{
    if (!m_messaging || !m_messaging->isReady()) {
        return;
    }

    QJsonObject message;
    message["type"] = "disconnect";
    m_messaging->sendMessage(message);

    m_isConnected = false;
    m_deviceId.clear();
    m_accessCode.clear();
    m_clients.clear();

    emit connectionStatusChanged();
    emit deviceIdChanged();
    emit accessCodeChanged();
    emit clientCountChanged();
    emit clientListChanged();
}

void HostManager::sendHello()
{
    if (!m_messaging || !m_messaging->isReady()) {
        emit errorOccurred("NOT_READY", "Host process is not ready");
        return;
    }

    QJsonObject message;
    message["type"] = "hello";
    m_messaging->sendMessage(message);
}

void HostManager::authorizeClient(const QString& connectionId, bool authorized)
{
    if (!m_messaging || !m_messaging->isReady()) {
        return;
    }

    QJsonObject message;
    message["type"] = "authorizationResponse";
    message["connectionId"] = connectionId;
    message["authorized"] = authorized;
    m_messaging->sendMessage(message);
}

void HostManager::kickClient(const QString& connectionId)
{
    if (!m_messaging || !m_messaging->isReady()) {
        return;
    }

    QJsonObject message;
    message["type"] = "kickClient";
    message["connectionId"] = connectionId;
    m_messaging->sendMessage(message);
}

QString HostManager::deviceId() const
{
    return m_deviceId;
}

QString HostManager::accessCode() const
{
    return m_accessCode;
}

bool HostManager::isConnected() const
{
    return m_isConnected;
}

int HostManager::clientCount() const
{
    return m_clients.size();
}

QList<SessionInfo> HostManager::connectedClients() const
{
    return m_clients.values();
}

void HostManager::onMessageReceived(const QJsonObject& message)
{
    QString type = message["type"].toString();
    
    qDebug() << "Host received message:" << type;

    if (type == "helloResponse") {
        handleHelloResponse(message);
    } else if (type == "hostReady") {
        handleHostReady(message);
    } else if (type == "clientConnected") {
        handleClientConnected(message);
    } else if (type == "clientDisconnected") {
        handleClientDisconnected(message);
    } else if (type == "authorizationRequest") {
        handleAuthorizationRequest(message);
    } else if (type == "clientListChanged") {
        handleClientListChanged(message);
    } else if (type == "error") {
        handleError(message);
    } else {
        qWarning() << "Unknown message type from host:" << type;
    }
}

void HostManager::onMessagingError(const QString& error)
{
    emit errorOccurred("MESSAGING_ERROR", error);
}

void HostManager::handleHelloResponse(const QJsonObject& message)
{
    QString version = message["version"].toString();
    qInfo() << "Host hello response, version:" << version;
    emit helloResponseReceived(version);
}

void HostManager::handleHostReady(const QJsonObject& message)
{
    m_deviceId = message["deviceId"].toString();
    m_accessCode = message["accessCode"].toString();
    m_isConnected = true;

    qInfo() << "Host ready - Device ID:" << m_deviceId 
            << "Access Code:" << m_accessCode;

    emit deviceIdChanged();
    emit accessCodeChanged();
    emit connectionStatusChanged();
    emit hostReady(m_deviceId, m_accessCode);
}

void HostManager::handleClientConnected(const QJsonObject& message)
{
    QString connectionId = message["connectionId"].toString();
    QJsonObject clientInfo = message["clientInfo"].toObject();

    SessionInfo session;
    session.connectionId = connectionId;
    session.username = clientInfo["username"].toString();
    session.ip = clientInfo["ip"].toString();
    session.deviceName = clientInfo["deviceName"].toString();
    session.state = "connected";

    m_clients[connectionId] = session;

    qInfo() << "Client connected:" << connectionId << session.username;

    emit clientCountChanged();
    emit clientListChanged();
    emit clientConnected(connectionId, clientInfo);
}

void HostManager::handleClientDisconnected(const QJsonObject& message)
{
    QString connectionId = message["connectionId"].toString();
    QString reason = message["reason"].toString();

    m_clients.remove(connectionId);

    qInfo() << "Client disconnected:" << connectionId << reason;

    emit clientCountChanged();
    emit clientListChanged();
    emit clientDisconnected(connectionId, reason);
}

void HostManager::handleAuthorizationRequest(const QJsonObject& message)
{
    QString connectionId = message["connectionId"].toString();
    QJsonObject clientInfo = message["clientInfo"].toObject();
    
    QString username = clientInfo["username"].toString();
    QString ip = clientInfo["ip"].toString();

    qInfo() << "Authorization request from:" << username << ip;

    emit authorizationRequested(connectionId, username, ip);
}

void HostManager::handleClientListChanged(const QJsonObject& message)
{
    m_clients.clear();
    
    QJsonArray clients = message["clients"].toArray();
    for (const QJsonValue& value : clients) {
        QJsonObject obj = value.toObject();
        SessionInfo session;
        session.connectionId = obj["connectionId"].toString();
        session.username = obj["username"].toString();
        session.ip = obj["ip"].toString();
        session.deviceName = obj["deviceName"].toString();
        session.connectedAt = obj["connectedAt"].toString();
        session.state = obj["state"].toString();
        
        m_clients[session.connectionId] = session;
    }

    emit clientCountChanged();
    emit clientListChanged();
}

void HostManager::handleError(const QJsonObject& message)
{
    QString code = message["code"].toString();
    QString errorMsg = message["message"].toString();
    
    qWarning() << "Host error:" << code << errorMsg;
    emit errorOccurred(code, errorMsg);
}

} // namespace quickdesk
