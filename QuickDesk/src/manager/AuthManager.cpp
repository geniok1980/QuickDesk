// Copyright 2026 QuickDesk Authors
#include "AuthManager.h"
#include "ServerManager.h"
#include "infra/http/httprequest.h"
#include "infra/log/log.h"
#include "core/localconfigcenter.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

namespace quickdesk {

namespace {
constexpr int kRequestTimeoutMs = 10000;
}

AuthManager::AuthManager(ServerManager* serverManager, QObject* parent)
    : QObject(parent)
    , m_serverManager(serverManager)
{
}

void AuthManager::restoreSession()
{
    auto& lcc = core::LocalConfigCenter::instance();
    m_token = lcc.userToken();
    if (m_token.isEmpty()) {
        LOG_INFO("[AuthManager] No saved token, not logged in");
        return;
    }

    LOG_INFO("[AuthManager] Restoring session from saved token");
    // Validate token by fetching user info
    fetchUserInfo();
}

QString AuthManager::httpBaseUrl() const
{
    QString wsUrl = m_serverManager->serverUrl();
    QString httpUrl = wsUrl;
    httpUrl.replace("ws://", "http://");
    httpUrl.replace("wss://", "https://");
    if (!httpUrl.endsWith("/")) httpUrl += "/";
    return httpUrl;
}

void AuthManager::login(const QString& username, const QString& password)
{
    QUrl url(httpBaseUrl() + "api/v1/user/login");
    QList<QPair<QString, QString>> headers;
    headers.append(qMakePair(QStringLiteral("Content-Type"), QStringLiteral("application/json")));

    QJsonObject body;
    body["username"] = username;
    body["password"] = password;
    QByteArray bodyData = QJsonDocument(body).toJson(QJsonDocument::Compact);

    LOG_INFO("[AuthManager] Logging in as: {}", username.toStdString());

    infra::HttpRequest::instance().sendPostRequest(
        url, headers, QString::fromUtf8(bodyData), kRequestTimeoutMs,
        [this](int statusCode, const std::string& errorMsg, const std::string& data) {
            QMetaObject::invokeMethod(this, [this, statusCode, errorMsg, data]() {
                if (statusCode != 200 || !errorMsg.empty()) {
                    // Try to parse error message from response
                    QString errStr;
                    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(data));
                    if (doc.isObject()) {
                        errStr = doc.object()["error"].toString();
                    }
                    if (errStr.isEmpty()) {
                        errStr = QString::fromStdString(errorMsg);
                    }
                    LOG_WARN("[AuthManager] Login failed: {}", errStr.toStdString());
                    emit loginFailed(errStr);
                    return;
                }

                QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(data));
                QJsonObject root = doc.object();
                m_token = root["token"].toString();
                QJsonObject userObj = root["user"].toObject();
                m_userId = userObj["id"].toInt();
                m_username = userObj["username"].toString();
                m_isLoggedIn = true;

                // Persist token
                auto& lcc = core::LocalConfigCenter::instance();
                lcc.setUserToken(m_token);
                lcc.setUserId(QString::number(m_userId));
                lcc.setUsername(m_username);

                LOG_INFO("[AuthManager] Login successful: {} (id={})", m_username.toStdString(), m_userId);
                emit loginStateChanged();
                emit loginSuccess();
            });
        });
}

void AuthManager::registerUser(const QString& username, const QString& password,
                                const QString& phone, const QString& email)
{
    QUrl url(httpBaseUrl() + "api/v1/user/register");
    QList<QPair<QString, QString>> headers;
    headers.append(qMakePair(QStringLiteral("Content-Type"), QStringLiteral("application/json")));

    QJsonObject body;
    body["username"] = username;
    body["password"] = password;
    if (!phone.isEmpty()) body["phone"] = phone;
    if (!email.isEmpty()) body["email"] = email;
    QByteArray bodyData = QJsonDocument(body).toJson(QJsonDocument::Compact);

    LOG_INFO("[AuthManager] Registering user: {}", username.toStdString());

    infra::HttpRequest::instance().sendPostRequest(
        url, headers, QString::fromUtf8(bodyData), kRequestTimeoutMs,
        [this](int statusCode, const std::string& errorMsg, const std::string& data) {
            QMetaObject::invokeMethod(this, [this, statusCode, errorMsg, data]() {
                if (statusCode != 200 || !errorMsg.empty()) {
                    QString errStr;
                    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(data));
                    if (doc.isObject()) {
                        errStr = doc.object()["error"].toString();
                    }
                    if (errStr.isEmpty()) {
                        errStr = QString::fromStdString(errorMsg);
                    }
                    LOG_WARN("[AuthManager] Registration failed: {}", errStr.toStdString());
                    emit registerFailed(errStr);
                    return;
                }

                LOG_INFO("[AuthManager] Registration successful");
                emit registerSuccess();
            });
        });
}

void AuthManager::logout()
{
    if (m_token.isEmpty()) {
        clearSession();
        return;
    }

    QUrl url(httpBaseUrl() + "api/v1/user/logout");
    QList<QPair<QString, QString>> headers;
    headers.append(qMakePair(QStringLiteral("Authorization"), QStringLiteral("Bearer ") + m_token));

    LOG_INFO("[AuthManager] Logging out user: {}", m_username.toStdString());

    infra::HttpRequest::instance().sendPostRequest(
        url, headers, QString(), kRequestTimeoutMs,
        [this](int statusCode, const std::string& errorMsg, const std::string& data) {
            Q_UNUSED(statusCode); Q_UNUSED(errorMsg); Q_UNUSED(data);
            QMetaObject::invokeMethod(this, [this]() {
                clearSession();
            });
        });
}

void AuthManager::fetchUserInfo()
{
    if (m_token.isEmpty()) return;

    QUrl url(httpBaseUrl() + "api/v1/user/me");
    QList<QPair<QString, QString>> headers;
    headers.append(qMakePair(QStringLiteral("Authorization"), QStringLiteral("Bearer ") + m_token));

    infra::HttpRequest::instance().sendGetRequest(
        url, headers, kRequestTimeoutMs,
        [this](int statusCode, const std::string& errorMsg, const std::string& data) {
            QMetaObject::invokeMethod(this, [this, statusCode, errorMsg, data]() {
                if (statusCode != 200 || !errorMsg.empty()) {
                    LOG_WARN("[AuthManager] Token validation failed, clearing session");
                    clearSession();
                    return;
                }

                QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(data));
                QJsonObject userObj = doc.object()["user"].toObject();
                m_userId = userObj["id"].toInt();
                m_username = userObj["username"].toString();
                m_isLoggedIn = true;

                LOG_INFO("[AuthManager] Session restored: {} (id={})", m_username.toStdString(), m_userId);
                emit loginStateChanged();
                emit loginSuccess();
            });
        });
}

void AuthManager::clearSession()
{
    m_isLoggedIn = false;
    m_username.clear();
    m_userId = 0;
    m_token.clear();

    auto& lcc = core::LocalConfigCenter::instance();
    lcc.setUserToken("");
    lcc.setUserId("");
    lcc.setUsername("");

    LOG_INFO("[AuthManager] Session cleared");
    emit loginStateChanged();
    emit loggedOut();
}

bool AuthManager::isLoggedIn() const { return m_isLoggedIn; }
QString AuthManager::username() const { return m_username; }
uint AuthManager::userId() const { return m_userId; }
QString AuthManager::token() const { return m_token; }

} // namespace quickdesk
