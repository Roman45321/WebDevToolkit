#ifndef SERVER_FACADE_H
#define SERVER_FACADE_H

#include "../servers/apache_server.h"
#include "../servers/nginx_server.h"
#include "../servers/mysql_server.h"


class ServerFacade : public QObject{
    Q_OBJECT
public:
    ServerFacade();
    void loadConfigurations(const QJsonObject& config);
    QJsonObject getConfigurations() const;
    QJsonObject getServerConfiguration(const QString& serverName);
    QJsonObject getAvailableVersions(const QString& serverName);
    QJsonObject getAvailablePHPVersions(const QString& serverName) const;
    bool getServerState(const QString& serverName);
    QDir getPHPPath(const QString& serverName) const;
    void startServer(const QString& serverName);
    void stopServer(const QString& serverName);
    void setServerVersion(const QString& serverName, const QString& version);
    bool setApachePHPVersion(const QString& phpVersion);
    bool setNginxPHPVersion(const QString& phpVersion);
    bool setServerPort(const QString& serverName, int port, QStringList &validationErrors);
    bool setApacheDocumentRoot(const QString& newRoot);
    bool setNginxDocumentRoot(const QString& newRoot);
    bool setNginxPHPFPMport(int port, QStringList &validationErrors);
    bool setNginxPHPCGIport(int port, QStringList &validationErrors);
    bool setPHPMyAdminPort(int port, QStringList &validationErrors);
    bool updateAbsolutePaths();
    bool isPortFree(int port) const;
    bool isPortFreeInApp(int port) const;
    bool isRunning(const QString& serverName);

private:
    ApacheServer apacheServer;
    NginxServer nginxServer;
    MySQLServer mysqlServer;
    QHash<QString, bool> serverStates;

    IServer* getServerByName(const QString& serverName);

public slots:
    void setServerState(const QString& serverName, bool isRunning);
    void handleError(const QString& errorTitle, const QString& errorMessage);
    void onDisplayServerWarning(const QString& serverName, const QString& errorMessage);

signals:
     void allServersStopped();
     void errorOccurred(const QString& errorTitle, const QString& errorMessage);
     void updateState(const QString& serverName, bool isRunning);
     void displayServerWarning(const QString& serverName, const QString& errorMessage);
};

#endif // SERVER_FACADE_H
