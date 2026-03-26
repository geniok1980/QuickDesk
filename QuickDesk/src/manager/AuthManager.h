// Copyright 2026 QuickDesk Authors
#ifndef QUICKDESK_MANAGER_AUTHMANAGER_H
#define QUICKDESK_MANAGER_AUTHMANAGER_H

#include <QObject>
#include <QString>

namespace quickdesk {

class ServerManager;

class AuthManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY loginStateChanged)
    Q_PROPERTY(QString username READ username NOTIFY loginStateChanged)
    Q_PROPERTY(uint userId READ userId NOTIFY loginStateChanged)
    Q_PROPERTY(QString token READ token NOTIFY loginStateChanged)

public:
    explicit AuthManager(ServerManager* serverManager, QObject* parent = nullptr);
    ~AuthManager() override = default;

    // Restore login state from persisted token
    void restoreSession();

    Q_INVOKABLE void login(const QString& username, const QString& password);
    Q_INVOKABLE void registerUser(const QString& username, const QString& password,
                                   const QString& phone, const QString& email);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void fetchUserInfo();

    bool isLoggedIn() const;
    QString username() const;
    uint userId() const;
    QString token() const;

signals:
    void loginStateChanged();
    void loginSuccess();
    void loginFailed(const QString& errorMsg);
    void registerSuccess();
    void registerFailed(const QString& errorMsg);
    void loggedOut();

private:
    void clearSession();
    QString httpBaseUrl() const;

    ServerManager* m_serverManager;
    bool m_isLoggedIn = false;
    QString m_username;
    uint m_userId = 0;
    QString m_token;
};

} // namespace quickdesk

#endif // QUICKDESK_MANAGER_AUTHMANAGER_H
