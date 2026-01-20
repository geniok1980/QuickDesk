// Copyright 2026 QuickDesk Authors

#include "ProcessManager.h"
#include "NativeMessaging.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

namespace quickdesk {

ProcessManager::ProcessManager(QObject* parent)
    : QObject(parent)
{
}

ProcessManager::~ProcessManager()
{
    stopAllProcesses();
}

bool ProcessManager::startHostProcess()
{
    if (isHostRunning()) {
        qWarning() << "Host process is already running";
        return true;
    }

    if (m_hostExePath.isEmpty()) {
        emit hostProcessError("Host executable path not set");
        return false;
    }

    m_hostProcess = std::make_unique<QProcess>(this);
    
    connect(m_hostProcess.get(), 
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ProcessManager::onHostProcessFinished);

    if (!startProcess(m_hostProcess.get(), m_hostExePath, "Host")) {
        m_hostProcess.reset();
        return false;
    }

    // Create Native Messaging handler
    m_hostMessaging = std::make_unique<NativeMessaging>(m_hostProcess.get(), this);
    
    emit hostProcessStarted();
    qInfo() << "Host process started, PID:" << m_hostProcess->processId();
    
    return true;
}

bool ProcessManager::startClientProcess()
{
    if (isClientRunning()) {
        qWarning() << "Client process is already running";
        return true;
    }

    if (m_clientExePath.isEmpty()) {
        emit clientProcessError("Client executable path not set");
        return false;
    }

    m_clientProcess = std::make_unique<QProcess>(this);
    
    connect(m_clientProcess.get(), 
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ProcessManager::onClientProcessFinished);

    if (!startProcess(m_clientProcess.get(), m_clientExePath, "Client")) {
        m_clientProcess.reset();
        return false;
    }

    // Create Native Messaging handler
    m_clientMessaging = std::make_unique<NativeMessaging>(m_clientProcess.get(), this);
    
    emit clientProcessStarted();
    qInfo() << "Client process started, PID:" << m_clientProcess->processId();
    
    return true;
}

void ProcessManager::stopHostProcess()
{
    if (m_hostProcess && m_hostProcess->state() != QProcess::NotRunning) {
        qInfo() << "Stopping host process...";
        m_hostProcess->closeWriteChannel(); // Close stdin to trigger graceful exit
        if (!m_hostProcess->waitForFinished(5000)) {
            qWarning() << "Host process did not exit gracefully, terminating...";
            m_hostProcess->terminate();
            if (!m_hostProcess->waitForFinished(3000)) {
                m_hostProcess->kill();
            }
        }
    }
    m_hostMessaging.reset();
    m_hostProcess.reset();
}

void ProcessManager::stopClientProcess()
{
    if (m_clientProcess && m_clientProcess->state() != QProcess::NotRunning) {
        qInfo() << "Stopping client process...";
        m_clientProcess->closeWriteChannel();
        if (!m_clientProcess->waitForFinished(5000)) {
            qWarning() << "Client process did not exit gracefully, terminating...";
            m_clientProcess->terminate();
            if (!m_clientProcess->waitForFinished(3000)) {
                m_clientProcess->kill();
            }
        }
    }
    m_clientMessaging.reset();
    m_clientProcess.reset();
}

void ProcessManager::stopAllProcesses()
{
    stopHostProcess();
    stopClientProcess();
}

bool ProcessManager::isHostRunning() const
{
    return m_hostProcess && m_hostProcess->state() == QProcess::Running;
}

bool ProcessManager::isClientRunning() const
{
    return m_clientProcess && m_clientProcess->state() == QProcess::Running;
}

NativeMessaging* ProcessManager::hostMessaging() const
{
    return m_hostMessaging.get();
}

NativeMessaging* ProcessManager::clientMessaging() const
{
    return m_clientMessaging.get();
}

void ProcessManager::setHostExePath(const QString& path)
{
    m_hostExePath = path;
}

void ProcessManager::setClientExePath(const QString& path)
{
    m_clientExePath = path;
}

QString ProcessManager::hostExePath() const
{
    return m_hostExePath;
}

QString ProcessManager::clientExePath() const
{
    return m_clientExePath;
}

bool ProcessManager::autoDetectPaths()
{
    QString hostPath = findExecutable("quickdesk_host");
    QString clientPath = findExecutable("quickdesk_client");

    if (!hostPath.isEmpty()) {
        m_hostExePath = hostPath;
        qInfo() << "Auto-detected host executable:" << hostPath;
    }

    if (!clientPath.isEmpty()) {
        m_clientExePath = clientPath;
        qInfo() << "Auto-detected client executable:" << clientPath;
    }

    return !hostPath.isEmpty() && !clientPath.isEmpty();
}

void ProcessManager::onHostProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(status);
    qInfo() << "Host process finished with exit code:" << exitCode;
    m_hostMessaging.reset();
    emit hostProcessStopped(exitCode);
}

void ProcessManager::onClientProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(status);
    qInfo() << "Client process finished with exit code:" << exitCode;
    m_clientMessaging.reset();
    emit clientProcessStopped(exitCode);
}

bool ProcessManager::startProcess(QProcess* process, const QString& exePath, 
                                  const QString& processName)
{
    QFileInfo fileInfo(exePath);
    if (!fileInfo.exists() || !fileInfo.isExecutable()) {
        QString error = QString("%1 executable not found: %2").arg(processName, exePath);
        qWarning() << error;
        if (processName == "Host") {
            emit hostProcessError(error);
        } else {
            emit clientProcessError(error);
        }
        return false;
    }

    process->setProgram(exePath);
    process->setWorkingDirectory(fileInfo.absolutePath());
    
    // Native Messaging uses stdin/stdout
    process->setProcessChannelMode(QProcess::SeparateChannels);
    
    process->start();
    
    if (!process->waitForStarted(5000)) {
        QString error = QString("Failed to start %1 process: %2")
                        .arg(processName, process->errorString());
        qWarning() << error;
        if (processName == "Host") {
            emit hostProcessError(error);
        } else {
            emit clientProcessError(error);
        }
        return false;
    }

    return true;
}

QString ProcessManager::findExecutable(const QString& name)
{
    // Search paths in order of priority
    QStringList searchPaths;
    
    // 1. Same directory as Qt app
    searchPaths << QCoreApplication::applicationDirPath();
    
    // 2. Relative to workspace (for development)
    QString appDir = QCoreApplication::applicationDirPath();
    searchPaths << QDir(appDir).filePath("../../src/out/Debug");
    searchPaths << QDir(appDir).filePath("../../../src/out/Debug");
    searchPaths << QDir(appDir).filePath("../../../../src/out/Debug");
    
    // 3. Absolute development paths
    searchPaths << "D:/mycode/remoting/src/out/Debug";
    
#ifdef Q_OS_WIN
    QString exeName = name + ".exe";
#else
    QString exeName = name;
#endif

    for (const QString& path : searchPaths) {
        QString fullPath = QDir(path).filePath(exeName);
        QFileInfo fileInfo(fullPath);
        if (fileInfo.exists() && fileInfo.isExecutable()) {
            return fileInfo.absoluteFilePath();
        }
    }

    return QString();
}

} // namespace quickdesk
