// server_task.h
#ifndef SERVER_TASK_H
#define SERVER_TASK_H

#include <QObject>
#include <QString>
#include <QHash>

class ServerTask : public QObject {
    Q_OBJECT

public:
    ServerTask(QObject *parent = nullptr);

public slots:
    void startServer(const QString& serverName);
    void stopServer(const QString& serverName);

signals:
    void started();
    void stopped();
    void errorOccurred(const QString& errorTitle, const QString& errorMessage);


};

#endif // SERVER_TASK_H
