// Copyright 2026 QuickDesk Authors
#ifndef QUICKDESK_MANAGER_CLOUDDEVICEMANAGER_H
#define QUICKDESK_MANAGER_CLOUDDEVICEMANAGER_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QWebSocket>

namespace quickdesk {

class ServerManager;
class AuthManager;

class CloudDeviceManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList myDevices READ myDevices NOTIFY myDevicesChanged)
    Q_PROPERTY(QVariantList myFavorites READ myFavorites NOTIFY myFavoritesChanged)

public:
    explicit CloudDeviceManager(ServerManager* serverManager, AuthManager* authManager, QObject* parent = nullptr);
    ~CloudDeviceManager() override;

    // My Devices
    Q_INVOKABLE void fetchMyDevices();
    Q_INVOKABLE void autoBindDevice(const QString& deviceId);
    Q_INVOKABLE void unbindDevice(const QString& deviceId);
    Q_INVOKABLE void setDeviceRemark(const QString& deviceId, const QString& remark);
    Q_INVOKABLE void syncAccessCode(const QString& deviceId, const QString& accessCode);
    Q_INVOKABLE QString getDeviceAccessCode(const QString& deviceId) const;

    // My Favorites
    Q_INVOKABLE void fetchFavorites();
    Q_INVOKABLE void addFavorite(const QString& deviceId, const QString& name, const QString& password);
    Q_INVOKABLE void updateFavorite(const QString& deviceId, const QString& name, const QString& password);
    Q_INVOKABLE void removeFavorite(const QString& deviceId);

    // Sync WebSocket
    void startSync();
    void stopSync();

    QVariantList myDevices() const;
    QVariantList myFavorites() const;

signals:
    void myDevicesChanged();
    void myFavoritesChanged();
    void syncMessage(const QJsonObject& msg);

private slots:
    void onSyncTextMessageReceived(const QString& message);
    void onSyncDisconnected();

private:
    QString httpBaseUrl() const;
    QList<QPair<QString, QString>> authHeaders() const;
    void handleSyncMessage(const QJsonObject& msg);

    ServerManager* m_serverManager;
    AuthManager* m_authManager;
    QWebSocket* m_syncSocket = nullptr;
    QVariantList m_myDevices;
    QVariantList m_myFavorites;
};

} // namespace quickdesk

#endif // QUICKDESK_MANAGER_CLOUDDEVICEMANAGER_H
