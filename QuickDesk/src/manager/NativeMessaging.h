// Copyright 2026 QuickDesk Authors
// Native Messaging protocol implementation for Qt.
// Supports two transports:
//   1. QProcess stdin/stdout  (child-process / development mode)
//   2. Dual QLocalSocket      (service mode — separate read/write pipes)

#ifndef QUICKDESK_MANAGER_NATIVEMESSAGING_H
#define QUICKDESK_MANAGER_NATIVEMESSAGING_H

#include <QObject>
#include <QProcess>
#include <QLocalSocket>
#include <QJsonObject>
#include <QByteArray>

namespace quickdesk {

class NativeMessaging : public QObject {
    Q_OBJECT
public:
    // Transport 1: QProcess (stdin/stdout)
    explicit NativeMessaging(QProcess* process, QObject* parent = nullptr);

    // Transport 2: dual QLocalSocket (separate read/write Named Pipes)
    NativeMessaging(QLocalSocket* readSocket, QLocalSocket* writeSocket,
                    QObject* parent = nullptr);

    ~NativeMessaging() override = default;

    void sendMessage(const QJsonObject& message);
    bool isReady() const;

signals:
    void messageReceived(const QJsonObject& message);
    void errorOccurred(const QString& error);

private slots:
    void onReadyRead();
    void onProcessError(QProcess::ProcessError error);
    void onSocketError(QLocalSocket::LocalSocketError error);

private:
    QProcess* m_process = nullptr;
    QLocalSocket* m_readSocket = nullptr;
    QLocalSocket* m_writeSocket = nullptr;
    QByteArray m_buffer;

    void parseBuffer();
    QByteArray encodeMessage(const QJsonObject& message);
};

} // namespace quickdesk

#endif // QUICKDESK_MANAGER_NATIVEMESSAGING_H
