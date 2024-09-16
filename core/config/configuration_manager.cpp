#include "configuration_manager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>

bool ConfigurationManager::loadConfiguration(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(fileData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return false;
    }

    configuration = jsonDoc.object();
    return true;
}

bool ConfigurationManager::saveConfiguration(const QString& filePath) const {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QJsonDocument jsonDoc(configuration);
    file.write(jsonDoc.toJson());
    file.close();
    return true;
}

QJsonObject ConfigurationManager::getConfiguration() const {
    return configuration;
}

void ConfigurationManager::setConfiguration(const QJsonObject& config) {
    configuration = config;
}

void ConfigurationManager::setServerConfiguration(const QString &serverName, const QJsonObject &config)
{
    QJsonObject servers = configuration["servers"].toObject();
    QJsonObject server = servers[serverName].toObject();
    server["config"] = config;
    servers[serverName] = server;
    configuration["servers"] = servers;
}
