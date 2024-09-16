#include "nginx_server.h"
#include "../../utility/process_manager.h"
#include "../config/configuration_manager.h"
#include "../singleton/server_manager.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QMessageBox>
#include <QMainWindow>

NginxServer::NginxServer() : nginxProcess(new QProcess()), phpCGIProcess(new QProcess()), phpFPMProcess(new QProcess()), lastCrashed(true) {

}

NginxServer::~NginxServer() {
}

bool NginxServer::start() {
    if(!ServerManager::getInstance().getFacade().getServerState("nginx")) {
        if (ServerManager::getInstance().getFacade().isPortFree(port)) {
            qDebug() << "Starting Nginx server version" << version << "with PHP version" << phpVersion << "on port" << port;
            QString command;
#ifdef Q_OS_WIN
            command = QDir::toNativeSeparators(path.filePath("nginx.exe"));
            if(!QFileInfo::exists(command)) {
                throw std::runtime_error("Failed to start Nginx server process: Nginx executable not found. Check you Nginx installation.");
            }
#elif defined(Q_OS_LINUX) || defined(Q_OS_MAC)

#endif
            nginxProcess = new QProcess();
            nginxProcess->setWorkingDirectory(QDir::toNativeSeparators(path.absolutePath()));
            lastCrashed = true;
            nginxProcess->start(command);
            if (!nginxProcess->waitForStarted(5000)) {
                QString errMsg = "Failed to start Nginx server process:" + nginxProcess->errorString();
                qWarning() << errMsg;
                throw std::runtime_error(errMsg.toStdString());
            } else {
                qDebug() << "Nginx server started successfully.";
                if(startPHPCGI()){
                    emit updateState("nginx", true);
                }
                QMainWindow::connect(nginxProcess, &QProcess::finished, this, [this](){
                    if(lastCrashed){
                        emit errorOccurred("The server was stopped", "The Nginx server was stopped due to an internal server error. For more detailed information, please check the Nginx error log.");
                    }
                });
                return true;
            }
        } else{
            emit displayServerWarning("The port is already in use.");
            return false;
        }
    }else {
        qWarning() << "Failed to start Nginx server: the server is already running.";
        return false;
    }
}

bool NginxServer::stop() {
    qDebug() << "Stopping Nginx server...";
    if (ServerManager::getInstance().getFacade().getServerState("nginx")) {
#ifdef Q_OS_WIN
        Process_manager::killChildProcessesRecursively(nginxProcess->processId());
#endif
        lastCrashed = false;
        nginxProcess->kill();
        if (!nginxProcess->waitForFinished(5000)) {
            QString errMsg = "Failed to stop Nginx server process:" + nginxProcess->errorString();
            qWarning() << errMsg;
            throw std::runtime_error(errMsg.toStdString());
        }
        qDebug() << "Nginx server stopped successfully.";
        if(stopPHPCGI()){
            emit updateState("nginx", false);
        }
        return true;
    } else{
        qWarning() << "Failed to stop Nginx server: the server is not running.";
        return false;
    }
}

bool NginxServer::stopPHPCGI(){
    if (!(phpCGIProcess && phpCGIProcess->state() == QProcess::Running)) {
        QString errMsg = "Failed to stop PHP-CGI process: PHP-CGI is not running.";
        qWarning() << errMsg;
        throw std::runtime_error(errMsg.toStdString());
    }
    phpCGIProcess->kill();
    if (!phpCGIProcess->waitForFinished(5000)) {
        QString errMsg = "Failed to stop PHP-CGI process:" + phpCGIProcess->errorString();
        qWarning() << errMsg;
        throw std::runtime_error(errMsg.toStdString());
    }
    qDebug() << "PHP-CGI stopped successfully.";
    return true;
}

QJsonObject NginxServer::getConfig() const {
    QJsonObject config;
    config["port"] = port;
    config["version"] = version;
    config["php_version"] = phpVersion;
    config["php_fpm_port"] = phpFPMPort;
    config["php_cgi_port"] = phpCGIport;
    config["document_root"] = documentRoot.absolutePath();
    return config;
}

bool NginxServer::setVersion(const QString& version) {
    ConfigurationManager& configManager = ConfigurationManager::getInstance();
    QJsonObject serversConfig = configManager.getConfiguration();
    QJsonObject serverConfig = serversConfig["servers"].toObject()["nginx"].toObject();
    QJsonObject versions = serverConfig["versions"].toObject();
    if (versions.contains(version)) {
        QDir dir(versions[version].toString());
        if(dir.exists()){
            path.setPath(dir.absolutePath());
            this->version = version;
            return true;

        } else{
            throw std::runtime_error("The path set for this version of Nginx does not exist.\nPlease check Apache installation.");
        }
    } else{
        QString errMsg = "Failed to set Nginx version: Installed Nginx version " + version + "not found.\nPlease check the Nginx installation.";
        throw std::runtime_error(errMsg.toStdString());
    }
}

QString NginxServer::getVersion() const {
    return version;
}

bool NginxServer::setPHPVersion(const QString& phpVersion) {
    ConfigurationManager& configManager = ConfigurationManager::getInstance();
    QJsonObject serversConfig = configManager.getConfiguration();
    QJsonObject serverConfig = serversConfig["servers"].toObject()["nginx"].toObject();
    QJsonObject phpVersions = serverConfig["php_versions"].toObject();
    if (!(phpVersions[phpVersion].isString() && phpVersions.contains(phpVersion))) {
        QString errMsg = "Failed to set PHP version for Nginx: PHP version " + phpVersion + " not found or has invalid path.";
        throw std::runtime_error(errMsg.toStdString());
    }
    QDir dir(phpVersions[phpVersion].toString());
    if(dir.exists()) {
        phpPath.setPath(dir.absolutePath());
        this->phpVersion = phpVersion;
    }
    else{
        QString errMsg = "Failed to set PHP version for Nginx: the path set for PHP version " + phpVersion + "does not exist.";
        throw std::runtime_error(errMsg.toStdString());
    }
    return true;
}

bool NginxServer::setPHPFPMport(int port, QStringList &validationErrors){
    //in process
}

QDir NginxServer::getPHPPath() const {
    return phpPath;
}

bool NginxServer::setPHPCGIPort(int port, QStringList &validationErrors) {
    if (this->phpCGIport == port) {
        return false;
    }
    if(!(port > 0 && port <= 65535)){
        validationErrors.append("InvalidPHPCGIPortValue");
        return false;
    }
    if(!ServerManager::getInstance().getFacade().isPortFreeInApp(port)) {
        validationErrors.append("PHPCGIPortOccupied");
        return false;
    }
    this->phpCGIport = port;
    QString phpCGIconfPath = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/conf/nginx/php_cgi.conf");
    if (!QFile::exists(phpCGIconfPath)) {
        QString errMsg = "Failed to set Nginx PHP-CGI port: PHP-CGI configuration file not found: " + phpCGIconfPath;
        qWarning() << errMsg;
        throw std::runtime_error(errMsg.toStdString());
    }
    QFile phpCGIconfFile(phpCGIconfPath);
    if (!phpCGIconfFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        QString errMsg = "Failed to set Nginx PHP-CGI port: Cannot open Nginx PHP-CGI configuration file: " + phpCGIconfPath;
        qWarning() << errMsg;
        throw std::runtime_error(errMsg.toStdString());
    }

    QTextStream confStream(&phpCGIconfFile);
    QString config = confStream.readAll();
    config.replace(QRegularExpression("fastcgi_pass\\s+127.0.0.1:\\d+;"), "fastcgi_pass 127.0.0.1:" + QString::number(phpCGIport) + ";");
    phpCGIconfFile.resize(0);
    confStream << config;
    phpCGIconfFile.close();
    return true;
}


QDir NginxServer::getPath() const {
    return path;
}

bool NginxServer::setPort(int port, QStringList &validationErrors) {
    if (this->port == port) {
        return false;
    }
    if(!(port > 0 && port <= 65535)){
        validationErrors.append("InvalidPortValue");
        return false;
    }
    if(!ServerManager::getInstance().getFacade().isPortFreeInApp(port)) {
        validationErrors.append("PortOccupied");
        return false;
    }
    this->port = port;
    QString nginxConfPath = path.filePath("conf/nginx.conf");
    if (!QFile::exists(nginxConfPath)) {
        QString errorMsg = "Failed to set Nginx port: Nginx configuration file not found: " + nginxConfPath;
        qWarning() << errorMsg;
        throw std::runtime_error(errorMsg.toStdString());
    }

    QFile nginxConfFile(nginxConfPath);
    if (!nginxConfFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        QString errorMsg = "Failed to set Nginx port: Cannot open Nginx configuration file: " + nginxConfPath;
        qWarning() << errorMsg;
        throw std::runtime_error(errorMsg.toStdString());
    }

    QTextStream confStream(&nginxConfFile);
    QString config = confStream.readAll();
    config.replace(QRegularExpression("listen\\s+\\d+;"), "listen " + QString::number(port) + ";");
    nginxConfFile.resize(0);
    confStream << config;
    nginxConfFile.close();
    return true;
}

bool NginxServer::startPHPCGI() {
    if(phpCGIProcess->state() != QProcess::Running) {
        if(ServerManager::getInstance().getFacade().isPortFree(phpCGIport)) {
            phpCGIProcess = new QProcess();
            QStringList arguments;
            arguments << "-b" << "127.0.0.1:" + QString::number(phpCGIport);
            QString command = QDir::toNativeSeparators(phpPath.filePath("php-cgi.exe"));
            if(!QFileInfo::exists(command)) {
                throw std::runtime_error("Failed to start PHP CGI process: PHP CGI executable not found. Check you PHP CGI installation.");
            }
            phpCGIProcess->start(command, arguments);
            if (!phpCGIProcess->waitForStarted(5000)) {
                QString errMsg = "Failed to start PHP CGI process:" + phpCGIProcess->errorString();
                qWarning() << errMsg;
                throw std::runtime_error(errMsg.toStdString());
            } else {
                qDebug() << "PHP CGI process started successfully.";
                return true;
            }
        } else{
            emit displayServerWarning("PHP-CGI port is already in use.");
            return false;
        }
    }else {
        qWarning() << "Failed to start PHP CGI: this port is already in use";
        return false;
    }
}


bool NginxServer::isRunning() const {
    if (nginxProcess && nginxProcess->state() == QProcess::Running) {
        return true;
    }
    else{
        return false;
    }
}

bool NginxServer::setDocumentRoot(const QString& newPath) {
    QDir dir(newPath);
    if (!dir.exists()) {
        return false;
    }
    this->documentRoot.setPath(dir.absolutePath());
    QFile file(path.filePath("conf/nginx.conf"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString errMsg = "Failed to set document root for Nginx: Unable to open Nginx configuration file for reading.";
        qWarning() << errMsg;
        throw std::runtime_error(errMsg.toStdString());
    }

    QString content;
    QTextStream in(&file);
    QRegularExpression re(R"(^(\s*)root\s+[^;]+;)");
    QRegularExpressionMatch match;

    while (!in.atEnd()) {
        QString line = in.readLine();
            match = re.match(line);
            if (match.hasMatch()) {
                QString indent = match.captured(1);
                line = QString("%1root %2;").arg(indent).arg(documentRoot.absolutePath());
            }
        content += line + '\n';
    }
    file.close();

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QString errMsg = "Failed to set document root for Nginx: Unable to open Nginx configuration file for writing.";
        qWarning() << errMsg;
        throw std::runtime_error(errMsg.toStdString());
    }
    QTextStream out(&file);
    out << content;
    file.close();
    return true;
}

