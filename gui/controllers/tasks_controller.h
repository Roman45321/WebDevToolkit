#ifndef TASKS_CONTROLLER_H
#define TASKS_CONTROLLER_H

#include <QObject>
#include <QHash>
#include <QThread>
#include <QProgressDialog>
#include <QHash>

#include "../../utility/server_task.h"

class TasksController : public QObject {
    Q_OBJECT

public:
    explicit TasksController(QObject *parent = nullptr);
    ~TasksController();

    void startServer(const QString& serverName);
    void stopServer(const QString& serverName);
    void stopAllServers();
    void startAllServers();
    void stopAllTasks();
    void setProgressDialog(QProgressDialog* dialog);
    void exitApplication();

private:
    QProgressDialog* progressDialog;
    QThread* apacheThread;
    ServerTask* apacheTask;
    QThread* nginxThread;
    ServerTask* nginxTask;
    QThread* mysqlThread;
    ServerTask* mysqlTask;

};

#endif // TASKSCONTROLLER_H
