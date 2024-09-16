#include "mysql_server.h"
#include "../../utility/process_manager.h"
#include "../config/configuration_manager.h"
#include "../singleton/server_manager.h"
#include <QDebug>
#include <QFile>
#include <QCoreApplication>
#include <QtCore>
#include <QMainWindow>
#include <QTextStream>
#include <QMessageBox>

MySQLServer::MySQLServer() : process(new QProcess()), lastCrashed(true) {

}

MySQLServer::~MySQLServer() {
}

bool MySQLServer::start() {
    if(!ServerManager::getInstance().getFacade().getServerState("mysql")) {
        if (ServerManager::getInstance().getFacade().isPortFree(port)) {
            qDebug() << "Starting MySQL server version" << version << "on port" << port;
            QString command;
#ifdef Q_OS_WIN

            command = QDir::toNativeSeparators(path.filePath("bin/mysqld.exe"));

            if(!QFileInfo::exists(command)) {
                throw std::runtime_error("Cannot start MySQL server process: MySQL executable not found. Check you MySQL installation.");
            }
#elif defined(Q_OS_LINUX) || defined(Q_OS_MAC)

#endif
            process = new QProcess();
            lastCrashed = true;
            process->start(command);
            if (!process->waitForStarted(5000)) {
                QString errMsg = "Failed to start MySQL server process:" + process->errorString();
                qWarning() << errMsg;
                throw std::runtime_error(errMsg.toStdString());
            } else {
                qDebug() << "MySQL server started successfully.";
                QMainWindow::connect(process, &QProcess::finished, this, [this](){
                    if(lastCrashed){
                        emit errorOccurred("The server was stopped", "The MySQL server was stopped due to an internal server error. For more detailed information, please check the MySQL error log.");
                    }
                });
                emit updateState("mysql", true);
                mysqlProcessID = process->processId();
                return true;
            }
        } else{
            emit displayServerWarning("The port is already in use.");
            return false;
        }
    }
    else {
        qWarning() << "Failed to start MySQL server: the server is already running.";
        return false;
    }
}

bool MySQLServer::stop() {
    qDebug() << "Stopping MySQL server...";
    if (ServerManager::getInstance().getFacade().getServerState("mysql")) {
#ifdef Q_OS_WIN
        killChildProcesses();
#elif defined(Q_OS_LINUX) || defined(Q_OS_MAC)

#endif
        lastCrashed = false;
        process->kill();
        if (!process->waitForFinished(5000)) {
            QString errMsg = "Failed to stop MySQL server process:" + process->errorString();
            qWarning() << errMsg;
            throw std::runtime_error(errMsg.toStdString());
        } else {
            qDebug() << "MySQL server stopped successfully.";
            emit updateState("mysql", false);
            delete process;
            return true;
        }
    } else{
        qWarning() << "Failed to stop MySQL server: the server is not running.";
        return false;
    }
}

bool MySQLServer::killChildProcesses() const {
    Process_manager::killChildProcessesRecursively(mysqlProcessID);
    return true;
}

QJsonObject MySQLServer::getConfig() const {
    QJsonObject config;
    config["port"] = port;
    config["version"] = version;
    return config;
}

bool MySQLServer::setVersion(const QString& version) {
    ConfigurationManager& configManager = ConfigurationManager::getInstance();
    QJsonObject serversConfig = configManager.getConfiguration();
    QJsonObject serverConfig = serversConfig["servers"].toObject()["mysql"].toObject();
    QJsonObject versions = serverConfig["versions"].toObject();
    if (versions[version].isString() && versions.contains(version)) {
        QDir dir(versions[version].toString());
        if(dir.exists()){
            path.setPath(dir.absolutePath());
            this->version = version;
            return true;

        } else{
            throw std::runtime_error("The path set for this version of MySQL does not exist.\nPlease check MySQL installation.");
        }
    } else{
        QString errMsg = "Failed to set MySQL version: Installed MySQL version " + version + "not found or has invalid path.\nPlease check the MySQL installation.";
        throw std::runtime_error(errMsg.toStdString());
    }
}

QString MySQLServer::getVersion() const {
    return version;
}

QDir MySQLServer::getPath() const {
    return path;
}

bool MySQLServer::setPort(int newPort, QStringList &validationErrors) {
    if(!(newPort > 0 && newPort <= 65535)){
        validationErrors.append("InvalidPortValue");
        return false;
    }
    if(this->port == newPort) {
        return false;
    }
    if(!ServerManager::getInstance().getFacade().isPortFreeInApp(newPort)) {
        validationErrors.append("PortOccupied");
        return false;
    }

    this->port = newPort;
    QString mysqlConfPath = path.filePath("my.ini");
    if (!QFile::exists(mysqlConfPath)) {
        QString errMsg = "MySQL configuration file not found: " + mysqlConfPath;
        qWarning() << errMsg;
        throw std::runtime_error(errMsg.toStdString());
    }
    QFile mysqlConfFile(mysqlConfPath);
    if (!mysqlConfFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        QString errMsg = "Failed to open MySQL configuration file: " + mysqlConfPath;
        qWarning() << errMsg;
        throw std::runtime_error(errMsg.toStdString());
    }

    QTextStream confStream(&mysqlConfFile);
    QString config = confStream.readAll();

    QRegularExpression portRegex("port=\\d+");
    QRegularExpressionMatch portMatch = portRegex.match(config);
    if (!portMatch.hasMatch()) {
        mysqlConfFile.close();
        QString errMsg = "Port configuration not found in the MySQL configuration file: " + mysqlConfPath;
        qWarning() << errMsg;
        throw std::runtime_error(errMsg.toStdString());
    }
    config.replace(portRegex, "port=" + QString::number(newPort));
    mysqlConfFile.resize(0);
    confStream << config;
    mysqlConfFile.close();
    try {
        if(!ServerManager::getInstance().getFacade().setPHPMyAdminPort(port, validationErrors)){
            QString errMsg = "Failed to configure PHPMyAdmin: configuration has invalid values.";
            throw std::runtime_error(errMsg.toStdString());
        }
        return true;

    } catch (const std::runtime_error &e) {
        QMessageBox::critical(nullptr, "Configuration failed", e.what());
    }
    qDebug() << "port completed here!";
}

bool MySQLServer::isRunning() const {
    if (process && process->state() == QProcess::Running) {
        return true;
    } else{
        return false;
    }
}

bool MySQLServer::setPHPMyAdminPort(int newPort, QStringList &validationErrors) {
    if(!(newPort > 0 && newPort <= 65535)){
        validationErrors.append("InvalidPortValue");
        return false;
    }
    this->phpMyAdminPort = newPort;
    QDir currentPath(QDir::currentPath());
    QString phpmyadminConfPath = currentPath.filePath("phpMyAdmin/config.inc.php");
    if (!QFile::exists(phpmyadminConfPath)) {
        QString errMsg = "PHPMyAdmin configuration file not found: " + phpmyadminConfPath;
        qWarning() << errMsg;
        throw std::runtime_error(errMsg.toStdString());
    }
    QFile phpmyadminConfFile(phpmyadminConfPath);
    if (!phpmyadminConfFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        QString errMsg = "Failed to open PHPMyAdmin configuration file: " + phpmyadminConfPath;
        qWarning() << errMsg;
        throw std::runtime_error(errMsg.toStdString());
    }

    QTextStream confStream(&phpmyadminConfFile);
    QString config = confStream.readAll();

    QRegularExpression portRegex("\\$cfg\\['Servers'\\]\\[\\$i\\]\\['port'\\] * = * '\\d+';");
    QRegularExpressionMatch portMatch = portRegex.match(config);

    if (portMatch.hasMatch()) {

        config.replace(portRegex, QString("$cfg['Servers'][$i]['port'] = '%1';").arg(newPort));

        phpmyadminConfFile.resize(0);
        confStream << config;

        phpmyadminConfFile.close();

        return true;
    } else {

        phpmyadminConfFile.close();
        QString errMsg = "Port configuration not found in the PHPMyAdmin configuration file: " + phpmyadminConfPath;
        qWarning() << errMsg;
        throw std::runtime_error(errMsg.toStdString());
    }
}
