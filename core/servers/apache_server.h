#ifndef APACHE_SERVER_H
#define APACHE_SERVER_H

#include "qglobal.h"
#include "../interfaces/iserver.h"
#include <QProcess>
#include <QMap>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#endif

class ApacheServer : public QObject, public IServer, public IServerWithPHP {
    Q_OBJECT
public:
    ApacheServer();
    ~ApacheServer();
    bool start() override;
    bool stop() override;
    bool killChildProcesses() const;
    QJsonObject getConfig() const override;

    bool setVersion(const QString& version) override;
    QString getVersion() const override;

    QDir getPHPPath() const override;
    QDir getPath() const override;
    bool isRunning() const override;

    bool setPHPVersion(const QString& phpVersion) override;
    bool setPort(int port, QStringList &validationErrors) override;
    bool setDocumentRoot(const QString& newPath);

signals:
    void updateState(const QString& serverName, bool isRunning);
    void errorOccurred(const QString& errorTitle, const QString& errorMessage);
    void displayServerWarning(const QString& errorMessage);

private:
    int port;
    int phpMyAdminPort;
    int apacheProcessID;
    QString version;
    QString phpVersion;
    QDir path;
    QDir phpPath;
    QDir documentRoot;
    QProcess* process;
    bool lastCrashed;
};

#endif // APACHE_SERVER_H
