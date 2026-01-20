// Copyright 2026 QuickDesk Authors
// QuickDesk Qt Application Entry Point

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QDebug>

#include "controller/MainController.h"
#include "manager/ServerManager.h"
#include "manager/HostManager.h"
#include "manager/ClientManager.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    
    app.setApplicationName("QuickDesk");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("QuickDesk");
    
    qInfo() << "QuickDesk starting...";
    qInfo() << "Qt version:" << qVersion();

    // Register C++ types for QML
    qmlRegisterType<quickdesk::MainController>("QuickDesk", 1, 0, "MainController");
    qmlRegisterUncreatableType<quickdesk::ServerManager>("QuickDesk", 1, 0, "ServerManager",
        "ServerManager is accessed through MainController");
    qmlRegisterUncreatableType<quickdesk::HostManager>("QuickDesk", 1, 0, "HostManager",
        "HostManager is accessed through MainController");
    qmlRegisterUncreatableType<quickdesk::ClientManager>("QuickDesk", 1, 0, "ClientManager",
        "ClientManager is accessed through MainController");

    QQmlApplicationEngine engine;
    
    // Handle QML creation failures
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { 
            qCritical() << "QML object creation failed!";
            QCoreApplication::exit(-1); 
        },
        Qt::QueuedConnection);
    
    // Load main QML
    engine.loadFromModule("QuickDesk", "Main");

    qInfo() << "QuickDesk started successfully";
    
    return app.exec();
}
