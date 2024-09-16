#ifndef MYSQL_SERVER_H
#define MYSQL_SERVER_H

#include "qglobal.h"
#include "../interfaces/iserver.h"
#include <QProcess>
#include <QMap>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#endif

class MySQLServer : public QObject, public IServer {
    Q_OBJECT
public:
    MySQLServer();
    ~MySQLServer();
    bool start() override;
    bool stop() override;
    bool killChildProcesses() const;
    QJsonObject getConfig() const override;
    bool setVersion(const QString& version) override;
    QString getVersion() const override;
    QDir getPath() const override;
    bool isRunning() const override;
    bool setPort(int port, QStringList &validationErrors) override;
    bool setPHPMyAdminPort(int newPort, QStringList &validationErrors);

signals:
    void updateState(const QString& serverName, bool isRunning);
    void errorOccurred(const QString& errorTitle, const QString& errorMessage);
    void displayServerWarning(const QString& errorMessage);

private:
    int port;
    QString version;
    QDir path;
    QProcess* process;
    bool lastCrashed;
    int mysqlProcessID;
    int phpMyAdminPort;
};

#endif // MYSQL_SERVER_H
