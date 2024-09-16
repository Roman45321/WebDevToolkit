#ifndef ISERVER_H
#define ISERVER_H

#include <QString>
#include <QJsonObject>
#include <QDir>

class IServer {
public:
    virtual ~IServer() {}
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual QJsonObject getConfig() const = 0;
    virtual bool isRunning() const = 0;
    virtual bool setVersion(const QString& version) = 0;
    virtual QString getVersion() const = 0;
    virtual QDir getPath() const = 0;
    virtual bool setPort(int port, QStringList &validationErrors) = 0;
};

class IServerWithPHP {
public:
    virtual bool setPHPVersion(const QString& phpVersion) = 0;
    virtual QDir getPHPPath() const = 0;
};

#endif // ISERVER_H
