#include "tasks_controller.h"
#include "../views/mainwindow.h"
#include "../../core/singleton/server_manager.h"
#include "qapplication.h"
#include <QMessageBox>

TasksController::TasksController(QObject *parent) :
    QObject(parent)
    , progressDialog(nullptr)
    , apacheThread(new QThread(this))
    , apacheTask(new ServerTask())
    , nginxThread(new QThread(this))
    , nginxTask(new ServerTask())
    , mysqlThread(new QThread(this))
    , mysqlTask(new ServerTask()){
    apacheTask->moveToThread(apacheThread);
    nginxTask->moveToThread(nginxThread);

    connect(apacheTask, &ServerTask::errorOccurred, (MainWindow*)parent, &MainWindow::handleError);
    connect(nginxTask, &ServerTask::errorOccurred, (MainWindow*)parent, &MainWindow::handleError);
    connect(apacheThread, &QThread::finished, apacheThread, &QObject::deleteLater);
    connect(nginxThread, &QThread::finished, nginxThread, &QObject::deleteLater);

}

TasksController::~TasksController() {
}

void TasksController::startServer(const QString& serverName) {

    if(serverName == "apache") {
        if(!apacheThread->isRunning()){
            apacheThread = new QThread(this);
            apacheTask = new ServerTask();
            apacheTask->moveToThread(apacheThread);
            apacheThread->start();
        }
        QMetaObject::invokeMethod(apacheTask, "startServer", Qt::QueuedConnection, Q_ARG(QString, "apache"));
    }
    else if(serverName == "nginx"){
        if(!nginxThread->isRunning()){
            nginxThread = new QThread(this);
            nginxTask = new ServerTask();
            nginxTask->moveToThread(nginxThread);
            nginxThread->start();
        }
        QMetaObject::invokeMethod(nginxTask, "startServer", Qt::QueuedConnection, Q_ARG(QString, "nginx"));
    }
    else if(serverName == "mysql"){
        if(!mysqlThread->isRunning()){
            mysqlThread = new QThread(this);
            mysqlTask = new ServerTask();
            mysqlTask->moveToThread(mysqlThread);
            mysqlThread->start();
        }
        QMetaObject::invokeMethod(mysqlTask, "startServer", Qt::QueuedConnection, Q_ARG(QString, "mysql"));
    }
    else{
        QString errMsg = "Cannot start the server: server " + serverName + " not found.";
        throw std::runtime_error(errMsg.toStdString());
    }
}

void TasksController::stopServer(const QString& serverName) {
    if(serverName == "apache") {
        if(!apacheThread->isRunning()){
            qWarning() << "Cannot stop Apache server: Apache is not running.";
            return;
        }
        QMetaObject::invokeMethod(apacheTask, "stopServer", Qt::QueuedConnection, Q_ARG(QString, "apache"));
    }
    else if(serverName == "nginx"){
        if(!nginxThread->isRunning()){
            qWarning() << "Cannot stop Nginx server: Nginx is not running.";
            return;
        }
        QMetaObject::invokeMethod(nginxTask, "stopServer", Qt::QueuedConnection, Q_ARG(QString, "nginx"));
    }
    else if(serverName == "mysql") {
        if(!mysqlThread->isRunning()){
            qWarning() << "Cannot stop Apache server: MySQL is not running.";
            return;
        }
        QMetaObject::invokeMethod(mysqlTask, "stopServer", Qt::QueuedConnection, Q_ARG(QString, "mysql"));
    }
    else {
        QString errMsg = "Cannot start the server: server " + serverName + " not found.";
        throw std::runtime_error(errMsg.toStdString());
    }
}

void TasksController::stopAllServers() {
    stopServer("apache");
    stopServer("nginx");
    stopServer("mysql");
}

void TasksController::startAllServers() {
    startServer("apache");
    startServer("nginx");
    startServer("mysql");
}

void TasksController::stopAllTasks() {
    if (apacheThread->isRunning()) {
        apacheThread->quit();
        apacheThread->wait();
    }
    if (nginxThread->isRunning()) {
        nginxThread->quit();
        nginxThread->wait();
    }
    if (mysqlThread->isRunning()) {
        mysqlThread->quit();
        mysqlThread->wait();
    }
}

void TasksController::setProgressDialog(QProgressDialog* dialog) {
    this->progressDialog = dialog;
}

bool allFalse(std::initializer_list<bool> bools) {
    for (bool b : bools) {
        if (b) {
            return false;
        }
    }
    return true;
}

void TasksController::exitApplication() {
    progressDialog->show();
    bool apacheState = ServerManager::getInstance().getFacade().getServerState("apache");
    bool nginxState = ServerManager::getInstance().getFacade().getServerState("nginx");
    bool mysqlState = ServerManager::getInstance().getFacade().getServerState("mysql");

    if (allFalse({apacheState, nginxState, mysqlState})) {
        this->stopAllTasks();
        progressDialog->hide();
        QApplication::exit();
    } else {
        QMainWindow::connect(&(ServerManager::getInstance().getFacade()), &ServerFacade::allServersStopped, this, [this](){
            qDebug() << "All servers stopped.";
            this->stopAllTasks();
            progressDialog->hide();
            QApplication::exit();
        } );
        this->stopAllServers();
    }
}

