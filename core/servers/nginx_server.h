#ifndef NGINX_SERVER_H
#define NGINX_SERVER_H

#include "qglobal.h"
#include "../interfaces/iserver.h"
#include <QProcess>
#include <QMap>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#endif
class NginxServer : public QObject, public IServer, public IServerWithPHP {
    Q_OBJECT
public:
    NginxServer();
    ~NginxServer();
    bool start() override;
    bool stop() override;
    bool stopPHPCGI();
    QJsonObject getConfig() const override;

    bool setVersion(const QString& version) override;
    QString getVersion() const override;
    bool isRunning() const override;

    bool setPHPVersion(const QString& phpVersion) override;
    QDir getPHPPath() const override;
    QDir getPath() const override;

    bool setPHPCGIPort(int port, QStringList &validationErrors);
    bool setPHPFPMport(int port, QStringList &validationErrors);
    bool setPort(int port, QStringList &validationErrors) override;
    bool setDocumentRoot(const QString &newPath);
    bool startPHPCGI();

signals:
    void updateState(const QString& serverName, bool isRunning);
    void errorOccurred(const QString& errorTitle, const QString& errorMessage);
    void displayServerWarning(const QString& errorMessage);

private:
    int port;
    int phpFPMPort;
    int phpCGIport;
    int phpMyAdminPort;
    QString version;
    QString phpVersion;
    QDir path;
    QDir phpPath;
    QDir documentRoot;
    QProcess* nginxProcess;
    QProcess* phpFPMProcess;
    QProcess* phpCGIProcess;
    bool lastCrashed = true;
};

#endif // NGINX_SERVER_H
