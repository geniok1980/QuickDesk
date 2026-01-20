// Copyright 2026 QuickDesk Authors
// Native Messaging protocol implementation for Qt

#ifndef QUICKDESK_MANAGER_NATIVEMESSAGING_H
#define QUICKDESK_MANAGER_NATIVEMESSAGING_H

#include <QObject>
#include <QProcess>
#include <QJsonObject>
#include <QByteArray>

namespace quickdesk {

/**
 * @brief Native Messaging protocol handler
 * 
 * Handles communication with Chrome Native Messaging protocol:
 * - 4-byte length prefix (little-endian)
 * - JSON message body
 */
class NativeMessaging : public QObject {
    Q_OBJECT
public:
    explicit NativeMessaging(QProcess* process, QObject* parent = nullptr);
    ~NativeMessaging() override = default;

    /**
     * @brief Send a JSON message to the process
     */
    void sendMessage(const QJsonObject& message);

    /**
     * @brief Check if the process is ready for communication
     */
    bool isReady() const;

signals:
    /**
     * @brief Emitted when a complete message is received
     */
    void messageReceived(const QJsonObject& message);

    /**
     * @brief Emitted when an error occurs
     */
    void errorOccurred(const QString& error);

private slots:
    void onReadyRead();
    void onProcessError(QProcess::ProcessError error);

private:
    QProcess* m_process;
    QByteArray m_buffer;

    void parseBuffer();
    QByteArray encodeMessage(const QJsonObject& message);
};

} // namespace quickdesk

#endif // QUICKDESK_MANAGER_NATIVEMESSAGING_H
