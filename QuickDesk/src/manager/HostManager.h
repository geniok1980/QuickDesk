// Copyright 2026 QuickDesk Authors
// Host process communication manager

#ifndef QUICKDESK_MANAGER_HOSTMANAGER_H
#define QUICKDESK_MANAGER_HOSTMANAGER_H

#include <QObject>
#include <QJsonObject>
#include <QList>
#include <QMap>

namespace quickdesk {

class NativeMessaging;

/**
 * @brief Session information for connected clients
 */
struct SessionInfo {
    QString connectionId;
    QString username;
    QString ip;
    QString deviceName;
    QString connectedAt;
    QString state;
};

/**
 * @brief Manages communication with the Host process
 */
class HostManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString deviceId READ deviceId NOTIFY deviceIdChanged)
    Q_PROPERTY(QString accessCode READ accessCode NOTIFY accessCodeChanged)
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionStatusChanged)
    Q_PROPERTY(int clientCount READ clientCount NOTIFY clientCountChanged)

public:
    explicit HostManager(QObject* parent = nullptr);
    ~HostManager() override = default;

    // Set the Native Messaging handler
    void setMessaging(NativeMessaging* messaging);

    // Host operations
    Q_INVOKABLE void connectToServer(const QString& serverUrl,
                                     const QString& deviceId = QString(),
                                     const QString& accessCode = QString());
    Q_INVOKABLE void disconnectFromServer();
    Q_INVOKABLE void sendHello();

    // Client management
    Q_INVOKABLE void authorizeClient(const QString& connectionId, bool authorized);
    Q_INVOKABLE void kickClient(const QString& connectionId);

    // State getters
    QString deviceId() const;
    QString accessCode() const;
    bool isConnected() const;
    int clientCount() const;
    QList<SessionInfo> connectedClients() const;

signals:
    void deviceIdChanged();
    void accessCodeChanged();
    void connectionStatusChanged();
    void clientCountChanged();
    
    void helloResponseReceived(const QString& version);
    void hostReady(const QString& deviceId, const QString& accessCode);
    void clientConnected(const QString& connectionId, const QJsonObject& clientInfo);
    void clientDisconnected(const QString& connectionId, const QString& reason);
    void authorizationRequested(const QString& connectionId, 
                                const QString& username, 
                                const QString& ip);
    void clientListChanged();
    void errorOccurred(const QString& code, const QString& message);

private slots:
    void onMessageReceived(const QJsonObject& message);
    void onMessagingError(const QString& error);

private:
    NativeMessaging* m_messaging = nullptr;
    QString m_deviceId;
    QString m_accessCode;
    bool m_isConnected = false;
    QMap<QString, SessionInfo> m_clients;

    void handleHelloResponse(const QJsonObject& message);
    void handleHostReady(const QJsonObject& message);
    void handleClientConnected(const QJsonObject& message);
    void handleClientDisconnected(const QJsonObject& message);
    void handleAuthorizationRequest(const QJsonObject& message);
    void handleClientListChanged(const QJsonObject& message);
    void handleError(const QJsonObject& message);
};

} // namespace quickdesk

#endif // QUICKDESK_MANAGER_HOSTMANAGER_H
