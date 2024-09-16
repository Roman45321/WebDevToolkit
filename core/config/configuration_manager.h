#ifndef CONFIGURATION_MANAGER_H
#define CONFIGURATION_MANAGER_H

#include <QString>
#include <QJsonObject>

class ConfigurationManager {
public:

    static ConfigurationManager& getInstance() {
        static ConfigurationManager instance;
        return instance;
    }


    bool loadConfiguration(const QString& filePath);


    bool saveConfiguration(const QString& filePath) const;


    QJsonObject getConfiguration() const;


    void setConfiguration(const QJsonObject& config);
    void setServerConfiguration(const QString& serverName, const QJsonObject& config);

private:
    ConfigurationManager() {}
    QJsonObject configuration;
};

#endif // CONFIGURATION_MANAGER_H
