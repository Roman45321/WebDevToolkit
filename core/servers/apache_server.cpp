#include "apache_server.h"
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

ApacheServer::ApacheServer() : process(new QProcess()), lastCrashed(true) {

}

ApacheServer::~ApacheServer() {
}

bool ApacheServer::start() {
    if(!ServerManager::getInstance().getFacade().getServerState("apache")) {
        if (ServerManager::getInstance().getFacade().isPortFree(port)) {
            qDebug() << "Starting Apache server version" << version << "with PHP version" << phpVersion << "on port" << port;
            QString command;
#ifdef Q_OS_WIN
            command = QDir::toNativeSeparators(path.filePath("bin/httpd.exe"));
            if(!QFileInfo::exists(command)) {
                throw std::runtime_error("Cannot start Apache server process: Apache executable not found. Check you Apache installation.");
            }
#elif defined(Q_OS_LINUX) || defined(Q_OS_MAC)
            qputenv("PATH", QByteArray(phpPath.toUtf8() + ":") + qgetenv("PATH"));
            command = path + "/bin/apachectl start";
#endif
            process = new QProcess();
            lastCrashed = true;
            process->start(command);
            if (!process->waitForStarted(5000)) {
                QString errMsg = "Failed to start Apache server process:" + process->errorString();
                qWarning() << errMsg;
                throw std::runtime_error(errMsg.toStdString());
            } else {
                qDebug() << "Apache server started successfully.";
                QMainWindow::connect(process, &QProcess::finished, this, [this](){
                    if(lastCrashed){
                        emit errorOccurred("The server was stopped", "The Apache server was stopped due to an internal server error. For more detailed information, please check the Apache error log.");
                    }
                });
                emit updateState("apache", true);
                apacheProcessID = process->processId();
                return true;
            }
        } else{
            emit displayServerWarning("The port is already in use.");
            return false;
        }
    }
    else {
        qWarning() << "Failed to start Apache server: the server is already running.";
        return false;
    }
}

bool ApacheServer::stop() {
    qDebug() << "Stopping Apache server...";
    if (ServerManager::getInstance().getFacade().getServerState("apache")) {
#ifdef Q_OS_WIN
        killChildProcesses();
#elif defined(Q_OS_LINUX) || defined(Q_OS_MAC)
        QString command = QString("%1/bin/apachectl stop").arg(path);
#endif
        lastCrashed = false;
        process->kill();
        if (!process->waitForFinished(5000)) {
            QString errMsg = "Failed to stop Apache server process:" + process->errorString();
            qWarning() << errMsg;
            throw std::runtime_error(errMsg.toStdString());
        } else {
            qDebug() << "Apache server stopped successfully.";
            emit updateState("apache", false);
            return true;
        }
    } else{
        qWarning() << "Failed to stop Apache server: the server is not running.";
        return false;
    }
}

bool ApacheServer::killChildProcesses() const {
    Process_manager::killChildProcessesRecursively(apacheProcessID);
    return true;
}

QJsonObject ApacheServer::getConfig() const {
    QJsonObject config;
    config["port"] = port;
    config["version"] = version;
    config["php_version"] = phpVersion;
    config["document_root"] = documentRoot.absolutePath();
    return config;
}

bool ApacheServer::setVersion(const QString& version) {
    ConfigurationManager& configManager = ConfigurationManager::getInstance();
    QJsonObject serversConfig = configManager.getConfiguration();
    QJsonObject serverConfig = serversConfig["servers"].toObject()["apache"].toObject();
    QJsonObject versions = serverConfig["versions"].toObject();
    if (versions[version].isString() && versions.contains(version)) {
        QDir dir(versions[version].toString());
        if(dir.exists()){
            path.setPath(dir.absolutePath());
            this->version = version;
            return true;
        } else{
            throw std::runtime_error("The path set for this version of Apache does not exist.\nPlease check Apache installation.");
        }
    } else{
        QString errMsg = "Failed to set Apache version: Installed Apache version " + version + "not  found or has invalid path.\nPlease check the Apache installation.";
        throw std::runtime_error(errMsg.toStdString());
    }
}

QString ApacheServer::getVersion() const {
    return version;
}

bool ApacheServer::setPHPVersion(const QString& phpVersion) {
    this->phpVersion = phpVersion;
    const QJsonObject& serverConfig = ConfigurationManager::getInstance().getConfiguration()["servers"].toObject()["apache"].toObject();
    QJsonObject phpVersions = serverConfig["php_versions"].toObject();

    if (phpVersions.contains(phpVersion) && phpVersions[phpVersion].isString()) {
        QDir dir(phpVersions[phpVersion].toString());
        if(dir.exists()) {
            this->phpPath.setPath(dir.absolutePath());
        }
        else{
            QString errMsg = "The path set for PHP version " + phpVersion + "does not exist.";
            throw std::runtime_error(errMsg.toStdString());
        }

        QString apacheConfigPath = getPath().filePath("conf/httpd.conf");
        QFile apacheConfigFile(apacheConfigPath);
        if (!apacheConfigFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
            QString errMsg = "Failed to open Apache config file: " + apacheConfigPath;
            qWarning() << errMsg;
            throw std::runtime_error(errMsg.toStdString());
        }

        QTextStream in(&apacheConfigFile);
        QString content = in.readAll();

        apacheConfigFile.close();

        QRegularExpression phpIncludeRegex(R"(Include\s+../../../conf/apache/php\d+\.\d+\.\d+_fcgid\.conf)");
        QRegularExpressionMatch match = phpIncludeRegex.match(content);

        if (!match.hasMatch()) {
            QString errMsg = "PHP include configuration not found in Apache config file.";
            qWarning() << errMsg;
            throw std::runtime_error(errMsg.toStdString());
        }

        QString oldPHPConfig = match.captured();
        QString newPHPConfig = QString("Include ../../../conf/apache/php%1.conf").arg(phpVersion + "_fcgid");

        if (oldPHPConfig.trimmed() == newPHPConfig.trimmed()) {
            return true;
        }

        content.replace(phpIncludeRegex, newPHPConfig.trimmed());

        if (!apacheConfigFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QString errMsg = "Failed to open Apache config file for writing: " + apacheConfigPath;
            qWarning() << errMsg;
            throw std::runtime_error(errMsg.toStdString());
        }

        QTextStream out(&apacheConfigFile);
        out << content;
        apacheConfigFile.close();
    } else {
        QString errMsg = "PHP Version " + phpVersion + " not found for Apache server.";
        qWarning() << errMsg;
        throw std::runtime_error(errMsg.toStdString());
    }

    return true;
}

QDir ApacheServer::getPHPPath() const {
    return phpPath;
}

QDir ApacheServer::getPath() const {
    return path;
}

bool ApacheServer::setPort(int port, QStringList &validationErrors) {
    if(!(port > 0 && port <= 65535)){
        validationErrors.append("InvalidPortValue");
        return false;
    }
    if(this->port == port) {
        return false;
    }
    if(!ServerManager::getInstance().getFacade().isPortFreeInApp(port)) {
        validationErrors.append("PortOccupied");
        return false;
    }

    this->port = port;
    QString apacheConfPath = path.filePath("conf/httpd.conf");
    if (!QFile::exists(apacheConfPath)) {
        QString errMsg = "Apache configuration file not found: " + apacheConfPath;
        qWarning() << errMsg;
        throw std::runtime_error(errMsg.toStdString());
    }

    QFile apacheConfFile(apacheConfPath);
    if (!apacheConfFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        QString errMsg = "Failed to open Apache configuration file:" + apacheConfPath;
        qWarning() << errMsg;
        throw std::runtime_error(errMsg.toStdString());
    }

    QTextStream confStream(&apacheConfFile);
    QString config = confStream.readAll();
    config.replace(QRegularExpression("Listen \\d+"), "Listen " + QString::number(port));
    apacheConfFile.resize(0);
    confStream << config;
    apacheConfFile.close();

}

bool ApacheServer::setDocumentRoot(const QString &newPath) {
    QDir dir(newPath);
    if (!dir.exists()) {
        return false;
    }

    QString oldPath = documentRoot.absolutePath();
    this->documentRoot.setPath(dir.absolutePath());
    QFile file(path.filePath("conf/httpd.conf"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString errMsg = "Unable to open the file for reading";
        qWarning() << errMsg;
        throw std::runtime_error(errMsg.toStdString());
    }

    QString content;
    QTextStream in(&file);
    QRegularExpression reDocRoot(R"(^DocumentRoot\s+"[^"]+")");
    QRegularExpression reDirectory(R"(<Directory\s+"[^"]+">)");
    QRegularExpressionMatch match;
    bool docRootLine = false;

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (docRootLine) {
            match = reDirectory.match(line);
            if (match.hasMatch()) {
                line = QString("<Directory \"%1\">").arg(documentRoot.absolutePath());
            }
            docRootLine = false;
        } else {
            match = reDocRoot.match(line);
            if (match.hasMatch()) {
                line = QString("DocumentRoot \"%1\"").arg(documentRoot.absolutePath());
                docRootLine = true;
            }
        }
        content += line + '\n';
    }
    file.close();

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QString errMsg = "Unable to open the file for writing";
        qWarning() << errMsg;
        throw std::runtime_error(errMsg.toStdString());
    }
    QTextStream out(&file);
    out << content;
    file.close();
    return true;
}



bool ApacheServer::isRunning() const {
    if (process && process->state() == QProcess::Running) {
        return true;
    } else{
        return false;
    }
}

