// Copyright 2026 QuickDesk Authors
// TURN Server configuration manager

#ifndef QUICKDESK_MANAGER_TURNSERVERMANAGER_H
#define QUICKDESK_MANAGER_TURNSERVERMANAGER_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>

namespace quickdesk {

/**
 * @brief Manages user-configured STUN/TURN servers (persisted locally).
 * 
 * STUN/TURN with credentials are fetched from the signaling server by
 * QuickDeskIceConfigFetcher on the Chromium side. This class only provides
 * optional user overrides to pass via native messaging.
 */
class TurnServerManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QJsonArray servers READ servers NOTIFY serversChanged)

public:
    explicit TurnServerManager(QObject* parent = nullptr);
    ~TurnServerManager() override = default;

    QJsonArray servers() const;
    void setServers(const QJsonArray& servers);
    
    /**
     * @brief Get the ICE config object for native messaging
     * 
     * Returns QJsonObject with "iceServers" containing user-configured servers only.
     * STUN/TURN with credentials are fetched by Chromium from the signaling server.
     */
    Q_INVOKABLE QJsonObject getEffectiveIceConfig() const;
    
    Q_INVOKABLE bool addTurnServer(const QString& url,
                                     const QString& username,
                                     const QString& credential,
                                     int maxRateKbps = 8000);
    Q_INVOKABLE bool addStunServer(const QString& url);
    Q_INVOKABLE void removeServer(int index);
    Q_INVOKABLE void clearServers();
    Q_INVOKABLE static bool validateServerUrl(const QString& url);

    void loadSettings();
    void saveSettings();

signals:
    void serversChanged();

private:
    QJsonArray m_userServers;
};

} // namespace quickdesk

#endif // QUICKDESK_MANAGER_TURNSERVERMANAGER_H
