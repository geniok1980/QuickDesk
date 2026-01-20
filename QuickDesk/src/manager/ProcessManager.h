// Copyright 2026 QuickDesk Authors
// Process lifecycle management

#ifndef QUICKDESK_MANAGER_PROCESSMANAGER_H
#define QUICKDESK_MANAGER_PROCESSMANAGER_H

#include <QObject>
#include <QProcess>
#include <memory>

namespace quickdesk {

class NativeMessaging;

/**
 * @brief Manages the lifecycle of Host and Client processes
 */
class ProcessManager : public QObject {
    Q_OBJECT
public:
    explicit ProcessManager(QObject* parent = nullptr);
    ~ProcessManager() override;

    // Process management
    bool startHostProcess();
    bool startClientProcess();
    
    void stopHostProcess();
    void stopClientProcess();
    void stopAllProcesses();

    bool isHostRunning() const;
    bool isClientRunning() const;

    // Get Native Messaging handlers
    NativeMessaging* hostMessaging() const;
    NativeMessaging* clientMessaging() const;

    // Executable paths
    void setHostExePath(const QString& path);
    void setClientExePath(const QString& path);
    QString hostExePath() const;
    QString clientExePath() const;

    // Auto-detect executable paths
    bool autoDetectPaths();

signals:
    void hostProcessStarted();
    void hostProcessStopped(int exitCode);
    void hostProcessError(const QString& error);
    
    void clientProcessStarted();
    void clientProcessStopped(int exitCode);
    void clientProcessError(const QString& error);

private slots:
    void onHostProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onClientProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    std::unique_ptr<QProcess> m_hostProcess;
    std::unique_ptr<QProcess> m_clientProcess;
    
    std::unique_ptr<NativeMessaging> m_hostMessaging;
    std::unique_ptr<NativeMessaging> m_clientMessaging;

    QString m_hostExePath;
    QString m_clientExePath;

    bool startProcess(QProcess* process, const QString& exePath, 
                      const QString& processName);
    QString findExecutable(const QString& name);
};

} // namespace quickdesk

#endif // QUICKDESK_MANAGER_PROCESSMANAGER_H
