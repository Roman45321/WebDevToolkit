#include "server_facade.h"
#include "../config/configuration_manager.h"
#include "../../gui/views/mainwindow.h"
#include <QDebug>
#include <QTcpSocket>
#include <QThread>
#include <QMessageBox>
#include <QMainWindow>

ServerFacade::ServerFacade(){
    serverStates.insert("apache", false);
    serverStates.insert("nginx", false);
    serverStates.insert("mysql", false);
    QMainWindow::connect(&apacheServer, &ApacheServer::updateState, this, &ServerFacade::setServerState);
    QMainWindow::connect(&nginxServer, &NginxServer::updateState, this, &ServerFacade::setServerState);
    QMainWindow::connect(&mysqlServer, &MySQLServer::updateState, this, &ServerFacade::setServerState);
    QMainWindow::connect(&apacheServer, &ApacheServer::errorOccurred, this, &ServerFacade::handleError);
    QMainWindow::connect(&nginxServer, &NginxServer::errorOccurred, this, &ServerFacade::handleError);
    QMainWindow::connect(&mysqlServer, &MySQLServer::errorOccurred, this, &ServerFacade::handleError);
    QMainWindow::connect(&apacheServer, &ApacheServer::displayServerWarning, this, [this](const QString &warningMessage) {
        onDisplayServerWarning("apache", warningMessage);
    });
    QMainWindow::connect(&nginxServer, &NginxServer::displayServerWarning, this, [this](const QString &warningMessage) {
        onDisplayServerWarning("nginx", warningMessage);
    });
    QMainWindow::connect(&mysqlServer, &MySQLServer::displayServerWarning, this, [this](const QString &warningMessage) {
        onDisplayServerWarning("mysql", warningMessage);
    });
}

void ServerFacade::loadConfigurations(const QJsonObject& config) {
    if(!(config.contains("servers") && config["servers"].isObject())) {
        QString errMsg = "Configuration is corrupted or has invalid values.";

        throw std::runtime_error(errMsg.toStdString());

    }
    if (config["servers"].toObject().contains("apache")) {
        QJsonObject apacheConfig = config["servers"].toObject()["apache"].toObject()["config"].toObject();
        QStringList validationErrors;
        if(!(apacheConfig.contains("version") && apacheConfig["version"].isString())) {
            QString errMsg = "Failed to set Apache version: configuration is corrupted or has invalid version value.";
            throw std::runtime_error(errMsg.toStdString());
        }
        setServerVersion("apache", apacheConfig["version"].toString());

        if(!(apacheConfig.contains("php_version") && apacheConfig["php_version"].isString())) {
            QString errMsg = "Failed to set PHP version for Apache: configuration is corrupted or has invalid PHP version value.";
            throw std::runtime_error(errMsg.toStdString());
        }
        setApachePHPVersion(apacheConfig["php_version"].toString());

        if (!(apacheConfig.contains("port") && apacheConfig["port"].isDouble())) {
            QString errMsg = "Failed to set port for Apache: configuration is corrupted or has invalid port value.";
            throw std::runtime_error(errMsg.toStdString());
        }
        setServerPort("apache", apacheConfig["port"].toInt(), validationErrors);

        if(!(apacheConfig.contains("document_root") && apacheConfig["document_root"].isString())) {
            QString errMsg = "Failed to set document root for Apache: configuration is corrupted or has invalid document root value.";
            throw std::runtime_error(errMsg.toStdString());
        }
        if(!setApacheDocumentRoot(apacheConfig["document_root"].toString())){
            QString errMsg = "Failed to configure Apache: DocumentRoot path does not exist.";
            throw std::runtime_error(errMsg.toStdString());
        }
        if(!validationErrors.isEmpty()){
            QString errMsg = "Failed to configure Apache: configuration has invalid values.";
            throw std::runtime_error(errMsg.toStdString());
        }
    } else{
        QString errMsg = "Failed to configure Apache: server configuration was not found or corrupted. ";
        throw std::runtime_error(errMsg.toStdString());
    }

    if (config["servers"].toObject().contains("nginx")) {
        QJsonObject nginxConfig = config["servers"].toObject()["nginx"].toObject()["config"].toObject();
        QStringList validationErrors;
        if(!(nginxConfig.contains("version") && nginxConfig["version"].isString())) {
            QString errMsg = "Failed to set Nginx version: configuration is corrupted or has invalid version value.";
            throw std::runtime_error(errMsg.toStdString());
        }
        setServerVersion("nginx", nginxConfig["version"].toString());

        if(!(nginxConfig.contains("php_version") && nginxConfig["php_version"].isString())) {
            QString errMsg = "Failed to set PHP version for Nginx: configuration is corrupted or has invalid PHP version value.";
            throw std::runtime_error(errMsg.toStdString());
        }
        setNginxPHPVersion(nginxConfig["php_version"].toString());

        if (!(nginxConfig.contains("port") && nginxConfig["port"].isDouble())) {
            QString errMsg = "Failed to set port for Nginx: configuration is corrupted or has invalid port value.";
            throw std::runtime_error(errMsg.toStdString());
        }
        setServerPort("nginx", nginxConfig["port"].toInt(), validationErrors);

        if(!(nginxConfig.contains("document_root") && nginxConfig["document_root"].isString())) {
            QString errMsg = "Failed to set document root for Nginx: configuration is corrupted or has invalid document root value.";
            throw std::runtime_error(errMsg.toStdString());
        }
        if(!setNginxDocumentRoot(nginxConfig["document_root"].toString())){
            QString errMsg = "Failed to configure Nginx: DocumentRoot path does not exist.";
            throw std::runtime_error(errMsg.toStdString());
        }
        if(!(nginxConfig.contains("php_cgi_port") && nginxConfig["php_cgi_port"].isDouble())) {
            QString errMsg = "Failed to set PHP-CGI port for Nginx: configuration is corrupted or has invalid port value.";
            throw std::runtime_error(errMsg.toStdString());
        }
        setNginxPHPCGIport(nginxConfig["php_cgi_port"].toInt(), validationErrors);
        if(!validationErrors.isEmpty()){
            QString errMsg = "Failed to configure Nginx: configuration has invalid values.";
            throw std::runtime_error(errMsg.toStdString());
        }
    } else{
        QString errMsg = "Failed to configure Nginx: server configuration was not found or corrupted.";
        throw std::runtime_error(errMsg.toStdString());
    }

    if (config["servers"].toObject().contains("mysql")) {
        QJsonObject mysqlConfig = config["servers"].toObject()["mysql"].toObject()["config"].toObject();
        QStringList validationErrors;

        if(!(mysqlConfig.contains("version") && mysqlConfig["version"].isString())) {
            QString errMsg = "Failed to set Mysql version: configuration is corrupted or has invalid version value.";
            throw std::runtime_error(errMsg.toStdString());
        }

        setServerVersion("mysql", mysqlConfig["version"].toString());

        if (!(mysqlConfig.contains("port") && mysqlConfig["port"].isDouble())) {
            QString errMsg = "Failed to set port for Mysql: configuration is corrupted or has invalid port value.";
            throw std::runtime_error(errMsg.toStdString());
        }
        setServerPort("mysql", mysqlConfig["port"].toInt(), validationErrors);

        if(!validationErrors.isEmpty()){

            QString errMsg = "Failed to configure Mysql: configuration has invalid values.";
            throw std::runtime_error(errMsg.toStdString());
        }

    } else{
        QString errMsg = "Failed to configure Mysql: server configuration not found or corrupted. ";
        throw std::runtime_error(errMsg.toStdString());
    }
    updateAbsolutePaths();
}

QJsonObject ServerFacade::getConfigurations() const {
    QJsonObject servers, serversObj, apacheConfig, nginxConfig;
    apacheConfig["config"] = apacheServer.getConfig();
    nginxConfig["config"] = nginxServer.getConfig();
    serversObj["apache"] = apacheConfig;
    serversObj["nginx"] = nginxConfig;
    servers["servers"] = serversObj;
    return servers;
}

QJsonObject ServerFacade::getServerConfiguration(const QString& serverName) {
    IServer* server = getServerByName(serverName);
    return server->getConfig();
}

QJsonObject ServerFacade::getAvailableVersions(const QString& serverName) {
    getServerByName(serverName);
    const QJsonObject& serverConfig = ConfigurationManager::getInstance().getConfiguration()["servers"].toObject()[serverName].toObject();
    QJsonObject versions = serverConfig["versions"].toObject();
    return versions;
}

QJsonObject ServerFacade::getAvailablePHPVersions(const QString& serverName) const {
    if(serverName == "apache" || serverName == "nginx") {
        const QJsonObject& serverConfig = ConfigurationManager::getInstance().getConfiguration()["servers"].toObject()[serverName].toObject();
        QJsonObject versions = serverConfig["php_versions"].toObject();
        return versions;
    } else{
        QString errMsg = "Failed to get available PHP version for server: server " + serverName + "does not support PHP.";
        throw std::runtime_error(errMsg.toStdString());
    }
}


QDir ServerFacade::getPHPPath(const QString& serverName) const {
    if(serverName == "apache") {
        return apacheServer.getPHPPath();
    } else if(serverName == "nginx") {
        return nginxServer.getPHPPath();
    }
    else{
        QString errMsg = "Failed to get PHP version for server: server " + serverName + " does not support PHP.";
        throw std::runtime_error(errMsg.toStdString());
    }
}

void ServerFacade::startServer(const QString& serverName) {
    IServer* server = getServerByName(serverName);
    server->start();
}

void ServerFacade::stopServer(const QString& serverName) {
    IServer* server = getServerByName(serverName);
    server->stop();
}

void ServerFacade::setServerVersion(const QString& serverName, const QString& version) {
    IServer* server = getServerByName(serverName);

    const QJsonObject& serverConfig = ConfigurationManager::getInstance().getConfiguration()["servers"].toObject()[serverName].toObject();
    const QJsonObject& versions = serverConfig["versions"].toObject();

    if (versions.contains(version) && versions[version].isString()) {
        QString path = versions[version].toString();
        server->setVersion(version);
        serverConfig["config"].toObject()["version"] = version;
        ConfigurationManager::getInstance().saveConfiguration("config.json");
    } else {
        QString errMsg = "Failed to set version for " + serverName + ": the version " + version + " not found for this server.";
        throw std::runtime_error(errMsg.toStdString());
    }

}

bool ServerFacade::setServerPort(const QString& serverName, int port, QStringList &validationErrors) {
    IServer* server = getServerByName(serverName);
    return server->setPort(port, validationErrors);
}

bool ServerFacade::setNginxPHPVersion(const QString& phpVersion) {
    return nginxServer.setPHPVersion(phpVersion);
}

bool ServerFacade::setApachePHPVersion(const QString& phpVersion) {
    return apacheServer.setPHPVersion(phpVersion);
}

bool ServerFacade::setApacheDocumentRoot(const QString &newRoot) {
    return apacheServer.setDocumentRoot(newRoot);
}

bool ServerFacade::setNginxDocumentRoot(const QString &newRoot) {
    return nginxServer.setDocumentRoot(newRoot);
}

bool ServerFacade::setNginxPHPFPMport(int port, QStringList &validationErrors) {
    return nginxServer.setPHPFPMport(port, validationErrors);
}

bool ServerFacade::setNginxPHPCGIport(int port, QStringList &validationErrors){
    return nginxServer.setPHPCGIPort(port, validationErrors);
}

bool ServerFacade::setPHPMyAdminPort(int port, QStringList &validationErrors){
    mysqlServer.setPHPMyAdminPort(port, validationErrors);
    return true;
}

bool ServerFacade::isPortFree(int port) const{
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, port);
    bool isFree = !socket.waitForConnected(100);
    socket.disconnectFromHost();
    return isFree;
}

bool ServerFacade::isPortFreeInApp(int port) const {
    if(apacheServer.getConfig()["port"].toInt() == port){
        return false;
    }
    if(nginxServer.getConfig()["port"].toInt() == port){
        return false;
    }
    if(nginxServer.getConfig()["php_cgi_port"].toInt() == port){
        return false;
    }
    if(mysqlServer.getConfig()["port"].toInt() == port){
        return false;
    }
    return true;
}

bool ServerFacade::updateAbsolutePaths()
{

    ConfigurationManager& configManager = ConfigurationManager::getInstance();
    QJsonObject serversConfig = configManager.getConfiguration()["servers"].toObject();
    QJsonObject apacheVersionsObj = serversConfig["apache"].toObject()["versions"].toObject();
    QString currentExecPath = QDir::currentPath();
    QString phpMyAdminAlias = currentExecPath + "/phpMyAdmin";

    for (auto it = apacheVersionsObj.begin(); it != apacheVersionsObj.end(); ++it) {
        QString version = it.key();
        QString basePath = it.value().toString();
        QString confPath = QDir(basePath).absoluteFilePath("conf/httpd.conf");

        QFile confFile(confPath);
        if (!confFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
            QString errMsg = "Could not open conf file:" + confPath;
            throw std::runtime_error(errMsg.toStdString());
        }

        QString confContent = confFile.readAll();
        confFile.close();


        QString absoluteServerRoot = QDir(basePath).absolutePath();
        QRegularExpression serverRootRegex(R"(ServerRoot\s+\"[^\"]+\")");
        confContent.replace(serverRootRegex, "ServerRoot \"" + absoluteServerRoot + "\"");


        QRegularExpression aliasRegex(R"(Alias\s+/phpMyAdmin\s+[^\s]+)");
        QRegularExpression directoryRegex(R"(<Directory\s+\"[^\"]+/phpMyAdmin\">)");

        confContent.replace(aliasRegex, "Alias /phpMyAdmin \"" + phpMyAdminAlias + "\"");
        confContent.replace(directoryRegex, "<Directory \"" + phpMyAdminAlias + "\">");


        if (!confFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            QString errMsg = "Could not open conf file for writing:" + confPath;
            throw std::runtime_error(errMsg.toStdString());
        }

        QTextStream outStream(&confFile);
        outStream << confContent;
        confFile.close();
    }


    QJsonObject nginxVersionsObj = serversConfig["nginx"].toObject()["versions"].toObject();
    QString currentExecPathNginx = QDir::currentPath();
    QString phpCgiInclude = currentExecPathNginx + "/conf/nginx/php_cgi.conf";

    for (auto it = nginxVersionsObj.begin(); it != nginxVersionsObj.end(); ++it) {
        QString version = it.key();
        QString basePath = it.value().toString();
        QString confPath = QDir(basePath).absoluteFilePath("conf/nginx.conf");

        QFile confFile(confPath);
        if (!confFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
            QString errMsg = "Could not open conf file:" + confPath;
            throw std::runtime_error(errMsg.toStdString());
        }

        QString confContent = confFile.readAll();
        confFile.close();

        QRegularExpression includeRegex(R"(include\s+\S+/conf/nginx/php_cgi.conf;)");
        confContent.replace(includeRegex, "include " + phpCgiInclude + ";");

        if (!confFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            QString errMsg = "Could not open conf file for writing:" + confPath;
            throw std::runtime_error(errMsg.toStdString());
        }

        QTextStream outStream(&confFile);
        outStream << confContent;
        confFile.close();
    }


    QJsonObject mysqlVersionsObj = serversConfig["mysql"].toObject()["versions"].toObject();

    for (auto it = mysqlVersionsObj.begin(); it != mysqlVersionsObj.end(); ++it) {
        QString version = it.key();
        QString basePath = it.value().toString();
        QString confPath = QDir(basePath).absoluteFilePath("my.ini");
        QString dataDirPath = QDir(basePath).absolutePath() + "/data";

        QFile confFile(confPath);
        if (!confFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
            QString errMsg = "Could not open conf file:" + confPath;
            throw std::runtime_error(errMsg.toStdString());
        }

        QString confContent = confFile.readAll();
        confFile.close();

        QRegularExpression datadirRegex(R"(datadir\s*=\s*[^\s]+)");
        confContent.replace(datadirRegex, "datadir=" + dataDirPath);

        if (!confFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            QString errMsg = "Could not open conf file for writing:" + confPath;
            throw std::runtime_error(errMsg.toStdString());
        }

        QTextStream outStream(&confFile);
        outStream << confContent;
        confFile.close();
    }

    QJsonObject phpVersionsObj = serversConfig["apache"].toObject()["php_versions"].toObject();

    for (auto it = phpVersionsObj.begin(); it != phpVersionsObj.end(); ++it) {
        QString version = it.key();
        QString basePath = it.value().toString();
        QString confPath =  QDir::currentPath() + "/conf/apache/php" + version + "_fcgid.conf";
        QString phpCgiPath = QDir(basePath).absolutePath() + "/php-cgi.exe";

        QFile confFile(confPath);
        if (!confFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
            QString errMsg = "Could not open conf file:" + confPath;
            throw std::runtime_error(errMsg.toStdString());
        }

        QString confContent = confFile.readAll();
        confFile.close();


        QRegularExpression fcgidWrapperRegex1(R"(FcgidWrapper\s+\"[^"]+\"\s+\.php)");
        QRegularExpression fcgidWrapperRegex2(R"(FcgidWrapper\s+\"[^"]+\"\s+\.html)");
        confContent.replace(fcgidWrapperRegex1, "FcgidWrapper \"" + phpCgiPath + "\" .php");
        confContent.replace(fcgidWrapperRegex2, "FcgidWrapper \"" + phpCgiPath + "\" .html");


        if (!confFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            QString errMsg = "Could not open conf file for writing:" + confPath;
            throw std::runtime_error(errMsg.toStdString());
        }

        QTextStream outStream(&confFile);
        outStream << confContent;
        confFile.close();
    }

    QString phpCGIconfPath = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/conf/nginx/php_cgi.conf");
    if (!QFile::exists(phpCGIconfPath)) {
        QString errMsg = "Configuration file not found:" + phpCGIconfPath;
        throw std::runtime_error(errMsg.toStdString());
    }

    QFile phpCGIconfFile(phpCGIconfPath);
    if (!phpCGIconfFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        QString errMsg = "Cannot open configuration file: " + phpCGIconfPath;
        throw std::runtime_error(errMsg.toStdString());
    }

    QTextStream confStream(&phpCGIconfFile);
    QString config = confStream.readAll();
    QString currentPath = QCoreApplication::applicationDirPath();

    QRegularExpression rootRegex(R"(\broot\s+[^;]+;)");
    config.replace(rootRegex, "root " + currentPath + ";");

    phpCGIconfFile.resize(0);
    confStream << config;
    phpCGIconfFile.close();
    return true;
}

IServer* ServerFacade::getServerByName(const QString& serverName) {
    if (serverName == "apache") {
        return &apacheServer;
    } else if (serverName == "nginx") {
        return &nginxServer;
    } else if (serverName == "mysql") {
        return &mysqlServer;
    }
    else{
        QString errMsg = "Failed to get server instance: server " + serverName + " not found.";
        throw std::runtime_error(errMsg.toStdString());
    }
}

void ServerFacade::setServerState(const QString& serverName, bool isRunning){
    if(!serverStates.contains(serverName)){
        QString errMsg = "Failed to set server status: server " + serverName + " not found.";
        throw std::runtime_error(errMsg.toStdString());
    }
    serverStates[serverName] = isRunning;
    bool allStopped = std::all_of(serverStates.begin(), serverStates.end(), [](bool value) {
        return value == false;
    });
    emit updateState(serverName, isRunning);
    if(allStopped) {
        emit allServersStopped();
    }
}

bool ServerFacade::getServerState(const QString& serverName){
    if(!serverStates.contains(serverName)){
        QString errMsg = "Failed to set server status: server " + serverName + " not found.";
        throw std::runtime_error(errMsg.toStdString());
    }
    return serverStates[serverName];
}

bool ServerFacade::isRunning(const QString& serverName) {
    IServer* server = getServerByName(serverName);
    return server->isRunning();
}

void ServerFacade::handleError(const QString& errorTitle, const QString& errorMessage) {
    emit errorOccurred(errorTitle, errorMessage);
}

void ServerFacade::onDisplayServerWarning(const QString &serverName, const QString &errorMessage)
{
    emit displayServerWarning(serverName, errorMessage);
}

