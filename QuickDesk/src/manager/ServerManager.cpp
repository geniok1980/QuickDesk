// Copyright 2026 QuickDesk Authors

#include "ServerManager.h"
#include <QCoreApplication>
#include <QDebug>

namespace quickdesk {

ServerManager::ServerManager(QObject* parent)
    : QObject(parent)
{
    loadSettings();
}

QString ServerManager::serverUrl() const
{
    return m_serverUrl;
}

void ServerManager::setServerUrl(const QString& url)
{
    if (m_serverUrl != url) {
        m_serverUrl = url;
        saveSettings();
        emit serverUrlChanged();
        qInfo() << "Server URL changed to:" << url;
    }
}

QString ServerManager::status() const
{
    return m_status;
}

void ServerManager::loadSettings()
{
    QSettings settings(QCoreApplication::organizationName(),
                       QCoreApplication::applicationName());
    
    m_serverUrl = settings.value("server/url", "ws://localhost:8000").toString();
    
    qInfo() << "Loaded server URL:" << m_serverUrl;
}

void ServerManager::saveSettings()
{
    QSettings settings(QCoreApplication::organizationName(),
                       QCoreApplication::applicationName());
    
    settings.setValue("server/url", m_serverUrl);
    settings.sync();
}

void ServerManager::updateStatus(const QString& status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged();
    }
}

} // namespace quickdesk
