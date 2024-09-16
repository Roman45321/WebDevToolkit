// server_task.cpp
#include "server_task.h"
#include "../core/singleton/server_manager.h"
#include "../gui/controllers/tasks_controller.h"
#include "../core/facade/server_facade.h"
#include <QDebug>
#include <QMessageBox>
#include <QThread>
#include <QMainWindow>

ServerTask::ServerTask(QObject *parent) : QObject(parent){

}

void ServerTask::startServer(const QString& serverName) {
    try {
        ServerManager::getInstance().getFacade().startServer(serverName);
        emit started();
    } catch (const std::exception& e) {
        emit errorOccurred("Failed to start the server", QString::fromUtf8(e.what()));
    }
}

void ServerTask::stopServer(const QString& serverName) {
    try {
        ServerManager::getInstance().getFacade().stopServer(serverName);
    } catch (const std::exception& e) {
        emit errorOccurred("Failed to stop the server", QString::fromUtf8(e.what()));
    }
}
