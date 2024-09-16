#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "../../core/singleton/server_manager.h"
#include "../../core/config/configuration_manager.h"
#include <QtConcurrent/QtConcurrent>
#include <QMessageBox>
#include <QTreeWidget>
#include <QScrollBar>
#include <QPalette>
#include <QApplication>
#include <QVBoxLayout>
#include <QIntValidator>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , tasksController(new TasksController(this))
    , progressDialog(new QProgressDialog(this))
{

    ui->setupUi(this);
    progressDialog->setWindowTitle(tr("Stopping Servers"));
    progressDialog->setLabelText(tr("Please wait while stopping servers..."));
    progressDialog->setCancelButton(nullptr);
    progressDialog->setRange(0, 0);
    progressDialog->setModal(true);
    progressDialog->setVisible(true);
    progressDialog->setWindowFlags(progressDialog->windowFlags() & ~Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint);
    progressDialog->hide();
    tasksController->setProgressDialog(progressDialog);
    connect(ui->startApache, &QPushButton::clicked, this, &MainWindow::onStartApacheButtonClicked);
    connect(ui->startNginx, &QPushButton::clicked, this, &MainWindow::onStartNginxButtonClicked);
    connect(ui->stopNginx, &QPushButton::clicked, this, &MainWindow::onStopNginxButtonClicked);
    connect(ui->stopApache, &QPushButton::clicked, this, &MainWindow::onStopApacheButtonClicked);
    connect(ui->startMySQL, &QPushButton::clicked, this, &MainWindow::onStartMySQLButtonClicked);
    connect(ui->stopMySQL, &QPushButton::clicked, this, &MainWindow::onStopMySQLButtonClicked);
    connect(ui->stopAllServersButton, &QPushButton::clicked, this, &MainWindow::onStopAllServersButtonClicked);
    connect(ui->startAllServersButton, &QPushButton::clicked, this, &MainWindow::onStartAllServersButtonClicked);
    connect(ui->saveApacheConfigurationBtn, &QPushButton::clicked, this, &MainWindow::onSaveApacheConfigurationButtonClicked);
    connect(ui->saveNginxConfigurationBtn, &QPushButton::clicked, this, &MainWindow::onSaveNginxConfigurationButtonClicked);
    connect(ui->saveMySQLConfigurationBtn, &QPushButton::clicked, this, &MainWindow::onSaveMySQLConfigurationButtonClicked);
    connect(&ServerManager::getInstance().getFacade(), &ServerManager::getInstance().getFacade().errorOccurred, this, &MainWindow::handleError);
    connect(&ServerManager::getInstance().getFacade(), &ServerManager::getInstance().getFacade().updateState, this, &MainWindow::setServerIndicator);
    connect(&ServerManager::getInstance().getFacade(), &ServerManager::getInstance().getFacade().displayServerWarning, this, &MainWindow::onDisplayServerWarning);
    int pageIndex = 0;
    traverseTree(ui->treeWidget->invisibleRootItem(), pageIndex);
    connect(ui->treeWidget, &QTreeWidget::itemSelectionChanged, this, &MainWindow::onItemSelectionChanged);
    ui->stackedWidget->setCurrentIndex(0);
    setupApacheConfigurationPage();
    setupNginxConfigurationPage();
    setupMySQLConfigurationPage();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::handleError(const QString& errorTitle, const QString& errorMessage)
{
    QMessageBox::critical(this, errorTitle, errorMessage);
}

void MainWindow::onStartApacheButtonClicked()
{
    ui->apacheWarning->setText("");
    ui->nginxWarning->setText("");
    ui->mysqlWarning->setText("");
    tasksController->startServer("apache");
}


void MainWindow::onStopAllServersButtonClicked()
{
    ui->apacheWarning->setText("");
    ui->nginxWarning->setText("");
    ui->mysqlWarning->setText("");
    tasksController->stopAllServers();
}

void MainWindow::onExitButtonClicked()
{

    tasksController->exitApplication();

}

void MainWindow::onStopApacheButtonClicked()
{
    tasksController->stopServer("apache");
}


void MainWindow::onStartNginxButtonClicked()
{
    tasksController->startServer("nginx");
}


void MainWindow::onStopNginxButtonClicked()
{
    tasksController->stopServer("nginx");
}

void MainWindow::onStartMySQLButtonClicked(){
    tasksController->startServer("mysql");
}

void MainWindow::onStopMySQLButtonClicked(){
    tasksController->stopServer("mysql");
}

void MainWindow::traverseTree(QTreeWidgetItem *parentItem, int &pageIndex) {
    for (int i = 0; i < parentItem->childCount(); ++i) {
        QTreeWidgetItem *child = parentItem->child(i);
        child->setData(0, Qt::UserRole, pageIndex++);
        traverseTree(child, pageIndex);
    }
}

void MainWindow::onItemSelectionChanged() {
    QTreeWidgetItem *selectedItem = ui->treeWidget->currentItem();
    if (selectedItem) {
        int pageIndex = selectedItem->data(0, Qt::UserRole).toInt();
        ui->stackedWidget->setCurrentIndex(pageIndex);
    }
}

void MainWindow::setServerIndicator(const QString& serverName, bool isRunning) {
    try {
        QPixmap pixmap(isRunning ? ":/icons/icons/on1.png" : ":/icons/icons/off1.png");
        if(serverName == "apache") {
            ui->apacheIndicator->setPixmap(pixmap);
            ui->apacheIndicator_2->setPixmap(pixmap);
        }
        else if(serverName == "mysql") {
            ui->mysqlIndicator->setPixmap(pixmap);
            ui->mysqlIndicator_2->setPixmap(pixmap);
        }
        else if(serverName == "nginx") {
            ui->nginxIndicator->setPixmap(pixmap);
            ui->nginxIndicator_2->setPixmap(pixmap);
        } else{
            QString errMsg = "Failed to set server indicator state: server " + serverName + " not found.";
            throw std::runtime_error(errMsg.toStdString());
        }
    } catch (const std::runtime_error &e) {
        QMessageBox::critical(nullptr, "Runtime error", e.what());


    }

}

void MainWindow::onDisplayServerWarning(const QString &serverName, const QString &errorMessage)
{
    if(serverName == "apache") {
        ui->apacheWarning->setText(errorMessage);
    } else if(serverName == "nginx") {
        ui->nginxWarning->setText(errorMessage);

    } else if(serverName == "mysql") {
        ui->mysqlWarning->setText(errorMessage);
    }
    else{
        QString errMsg = "Failed to display server warning: server " + serverName + " not found.";
        throw std::runtime_error(errMsg.toStdString());
    }
}



void MainWindow::onStartAllServersButtonClicked()
{
    ui->apacheWarning->setText("");
    ui->nginxWarning->setText("");
    ui->mysqlWarning->setText("");
    tasksController->startAllServers();
}

void MainWindow::onSaveApacheConfigurationButtonClicked(){
    if(ServerManager::getInstance().getFacade().getServerState("apache")){
        QMessageBox::StandardButton reply;
        QMessageBox::information(this, "Configuration", "Configuration cannot be applied because the service is currently running. Stop the service and save the configuration.",
                                 QMessageBox::Ok);
        return;
    }
    QStringList validationErrors;
    try {
        ServerManager::getInstance().getFacade().setServerVersion("apache", ui->apacheVersionSelect->currentText());
    } catch (const std::runtime_error &e) {
        QMessageBox::critical(nullptr, "Failed to set configuration", e.what());
    }
    try {
        ServerManager::getInstance().getFacade().setServerPort("apache", ui->apachePortLineEdit->text().toInt(), validationErrors);
        if(validationErrors.contains("PortOccupied")){
            ui->apachePortWarning->setText("This port is already in use in application");
        }
    } catch (const std::runtime_error &e) {
        QMessageBox::critical(nullptr, "Failed to set configuration", e.what());
    }
    try {
        ServerManager::getInstance().getFacade().setApachePHPVersion(ui->apachePHPVersionSelect->currentText());
    } catch (const std::runtime_error &e) {
        QMessageBox::critical(nullptr, "Failed to set configuration", e.what());
    }
    try {
        bool pathExists = ServerManager::getInstance().getFacade().setApacheDocumentRoot(ui->apacheDocumentRootLineEdit->text());
        if(!pathExists){
            ui->apacheDocumentRootWarning->setText("The specified path does not exist");
        }
    } catch (const std::runtime_error &e) {
        QMessageBox::critical(nullptr, "Failed to set configuration", e.what());
    }
    QJsonObject apacheConfig = ServerManager::getInstance().getFacade().getServerConfiguration("apache");
    ConfigurationManager& configManager = ConfigurationManager::getInstance();
    try {
        configManager.setServerConfiguration("apache", apacheConfig);
        configManager.saveConfiguration("config.json");
    } catch (const std::runtime_error &e) {
        QMessageBox::critical(nullptr, "Failed to save configuration", e.what());
    }
    if(validationErrors.empty()){
        QMessageBox::information(this, "Configuration", "Configuration saved",
                                 QMessageBox::Ok);
    }
}

void MainWindow::onSaveNginxConfigurationButtonClicked(){
    if(ServerManager::getInstance().getFacade().getServerState("nginx")){
        QMessageBox::StandardButton reply;
        QMessageBox::question(this, "Configuration", "Configuration cannot be applied because the service is currently running. Stop the service and save the configuration.",
                              QMessageBox::Ok);
        return;
    }
    QStringList validationErrors;
    try {
        ServerManager::getInstance().getFacade().setServerVersion("nginx", ui->nginxVersionSelect->currentText());
    } catch (const std::runtime_error &e) {
        QMessageBox::critical(nullptr, "Failed to set configuration", e.what());
    }
    try {
        ServerManager::getInstance().getFacade().setServerPort("nginx", ui->nginxPortLineEdit->text().toInt(), validationErrors);
        if(validationErrors.contains("PortOccupied")){
            ui->nginxPortWarning->setText("This port is already in use in application");
        }
    } catch (const std::runtime_error &e) {
        QMessageBox::critical(nullptr, "Failed to set configuration", e.what());
    }
    try {
        ServerManager::getInstance().getFacade().setNginxPHPCGIport(ui->nginxPHPCGIPortLineEdit->text().toInt(), validationErrors);
        if(validationErrors.contains("PHPCGIPortOccupied")){
            ui->nginxPHPCGIPortWarning->setText("This port is already in use in application");
        }
    } catch (const std::runtime_error &e) {
        QMessageBox::critical(nullptr, "Failed to set configuration", e.what());
    }
    try {
        ServerManager::getInstance().getFacade().setNginxPHPVersion(ui->nginxPHPVersionSelect->currentText());
    } catch (const std::runtime_error &e) {
        QMessageBox::critical(nullptr, "Failed to set configuration", e.what());
    }
    try {
        bool pathExists = ServerManager::getInstance().getFacade().setNginxDocumentRoot(ui->nginxDocumentRootLineEdit->text());
        if(!pathExists){
            ui->nginxDocumentRootWarning->setText("The specified path does not exist");
        }
    } catch (const std::runtime_error &e) {
        QMessageBox::critical(nullptr, "Failed to set configuration", e.what());
    }
    QJsonObject nginxConfig = ServerManager::getInstance().getFacade().getServerConfiguration("nginx");
    ConfigurationManager& configManager = ConfigurationManager::getInstance();
    try {
        configManager.setServerConfiguration("nginx", nginxConfig);
        configManager.saveConfiguration("config.json");
    } catch (const std::runtime_error &e) {
        QMessageBox::critical(nullptr, "Failed to save configuration", e.what());
    }
    if(validationErrors.empty()){
        QMessageBox::information(this, "Configuration", "Configuration saved",
                                 QMessageBox::Ok);
    }
}

void MainWindow::onSaveMySQLConfigurationButtonClicked(){
    if(ServerManager::getInstance().getFacade().getServerState("mysql")){
        QMessageBox::StandardButton reply;
        QMessageBox::question(this, "Configuration", "Configuration cannot be applied because the service is currently running. Stop the service and save the configuration.",
                              QMessageBox::Ok);
        return;
    }
    QStringList validationErrors;
    try {
        ServerManager::getInstance().getFacade().setServerVersion("mysql", ui->mysqlVersionSelect->currentText());
    } catch (const std::runtime_error &e) {
        QMessageBox::critical(nullptr, "Failed to set configuration", e.what());
    }
    try {
        ServerManager::getInstance().getFacade().setServerPort("mysql", ui->mysqlPortLineEdit->text().toInt(), validationErrors);
        if(validationErrors.contains("PortOccupied")){
            ui->mysqlPortWarning->setText("This port is already in use in application");
        }
    } catch (const std::runtime_error &e) {
        QMessageBox::critical(nullptr, "Failed to set configuration", e.what());
    }
    QJsonObject mysqlConfig = ServerManager::getInstance().getFacade().getServerConfiguration("mysql");
    ConfigurationManager& configManager = ConfigurationManager::getInstance();
    try {
        configManager.setServerConfiguration("mysql", mysqlConfig);
        configManager.saveConfiguration("config.json");
    } catch (const std::runtime_error &e) {
        QMessageBox::critical(nullptr, "Failed to save configuration", e.what());
    }
    if(validationErrors.empty()){
        QMessageBox::information(this, "Configuration", "Configuration saved",
                                 QMessageBox::Ok);
    }
}

void MainWindow::setupApacheConfigurationPage() {
    QObject::connect(ui->apachePortLineEdit, &QLineEdit::textChanged, [this]() {
        ui->apachePortWarning->setText("");
        QString text = ui->apachePortLineEdit->text();
        bool ok;
        int number = text.toInt(&ok);
        if (ok && (number < 1 || number > 65535)) {
            ui->apachePortLineEdit->setText(text.left(text.length() - 1));
        }
    });
    QObject::connect(ui->apacheDocumentRootLineEdit, &QLineEdit::textChanged, [this]() {
        ui->apacheDocumentRootWarning->setText("");
    });
    QJsonObject apacheVesions = ServerManager::getInstance().getFacade().getAvailableVersions("apache");
    for (auto it = apacheVesions.begin(); it != apacheVesions.end(); ++it) {
        ui->apacheVersionSelect->addItem(it.key());
    }
    QJsonObject apacheConfig = ServerManager::getInstance().getFacade().getServerConfiguration("apache");
    int vIndex = ui->apacheVersionSelect->findText(apacheConfig["version"].toString());
    if (vIndex != -1) {
        ui->apacheVersionSelect->setCurrentIndex(vIndex);
    } else{
        QString errMsg = "Failed to set active Apache version: version " + apacheConfig["version"].toString() + " not found. ";
        throw std::runtime_error(errMsg.toStdString());
    }

    ui->apachePortLineEdit->setText(QString::number(apacheConfig["port"].toDouble()));

    QJsonObject apachePHPVesions = ServerManager::getInstance().getFacade().getAvailablePHPVersions("apache");
    for (auto it = apachePHPVesions.begin(); it != apachePHPVesions.end(); ++it) {
        ui->apachePHPVersionSelect->addItem(it.key());
    }
    int phpIndex = ui->apachePHPVersionSelect->findText(apacheConfig["php_version"].toString());
    if (phpIndex != -1) {
        ui->apachePHPVersionSelect->setCurrentIndex(phpIndex);
    } else{
        QString errMsg = "Failed to set active PHP version for Apache: version " + apacheConfig["php_version"].toString() + " not found. ";
        throw std::runtime_error(errMsg.toStdString());
    }

    ui->apacheDocumentRootLineEdit->setText(apacheConfig["document_root"].toString());
    connect(ui->apacheDocumentRootChooseBtn, &QPushButton::clicked, this, [this](){
        QString dir = QFileDialog::getExistingDirectory(nullptr, tr("Выберите папку"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!dir.isEmpty()) {
            ui->apacheDocumentRootLineEdit->setText(dir);
        }
    });
    connect(ui->apacheDocumentRootOpenBtn, &QPushButton::clicked, this, [this, apacheConfig](){
#ifdef Q_OS_WIN
        QString command = "explorer";
        QProcess::startDetached(command, QStringList() << QDir::toNativeSeparators(apacheConfig["document_root"].toString()));
#endif
    });
}

void MainWindow::setupNginxConfigurationPage() {
    QObject::connect(ui->nginxPortLineEdit, &QLineEdit::textChanged, [this]() {
        ui->nginxPortWarning->setText("");
        QString text = ui->nginxPortLineEdit->text();
        bool ok;
        int number = text.toInt(&ok);
        if (ok && (number < 1 || number > 65535)) {
            ui->nginxPortLineEdit->setText(text.left(text.length() - 1));
        }
    });
    QObject::connect(ui->nginxPHPCGIPortLineEdit, &QLineEdit::textChanged, [this]() {
        ui->nginxPHPCGIPortWarning->setText("");
        QString text = ui->nginxPHPCGIPortLineEdit->text();
        bool ok;
        int number = text.toInt(&ok);
        if (ok && (number < 1 || number > 65535)) {
            ui->nginxPHPCGIPortLineEdit->setText(text.left(text.length() - 1));
        }
    });
    QObject::connect(ui->nginxDocumentRootLineEdit, &QLineEdit::textChanged, [this]() {
        ui->nginxDocumentRootWarning->setText("");
    });
    QJsonObject nginxVesions = ServerManager::getInstance().getFacade().getAvailableVersions("nginx");
    for (auto it = nginxVesions.begin(); it != nginxVesions.end(); ++it) {
        ui->nginxVersionSelect->addItem(it.key());
    }
    QJsonObject nginxConfig = ServerManager::getInstance().getFacade().getServerConfiguration("nginx");
    int vIndex = ui->nginxVersionSelect->findText(nginxConfig["version"].toString());
    if (vIndex != -1) {
        ui->nginxVersionSelect->setCurrentIndex(vIndex);
    } else{
        QString errMsg = "Failed to set active Nginx version: version " + nginxConfig["version"].toString() + " not found. ";
        throw std::runtime_error(errMsg.toStdString());
    }

    ui->nginxPortLineEdit->setText(QString::number(nginxConfig["port"].toDouble()));
    ui->nginxPHPCGIPortLineEdit->setText(QString::number(nginxConfig["php_cgi_port"].toDouble()));

    QJsonObject nginxPHPVesions = ServerManager::getInstance().getFacade().getAvailablePHPVersions("nginx");
    for (auto it = nginxPHPVesions.begin(); it != nginxPHPVesions.end(); ++it) {
        ui->nginxPHPVersionSelect->addItem(it.key());
    }
    int phpIndex = ui->nginxPHPVersionSelect->findText(nginxConfig["php_version"].toString());
    if (phpIndex != -1) {
        ui->nginxPHPVersionSelect->setCurrentIndex(phpIndex);
    } else{
        QString errMsg = "Failed to set active PHP version for Nginx: version " + nginxConfig["php_version"].toString() + " not found. ";
        throw std::runtime_error(errMsg.toStdString());
    }

    ui->nginxDocumentRootLineEdit->setText(nginxConfig["document_root"].toString());
    connect(ui->nginxDocumentRootChooseBtn, &QPushButton::clicked, this, [this](){
        QString dir = QFileDialog::getExistingDirectory(nullptr, tr("Выберите папку"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!dir.isEmpty()) {
            ui->nginxDocumentRootLineEdit->setText(dir);
        }
    });
    connect(ui->nginxDocumentRootOpenBtn, &QPushButton::clicked, this, [this, nginxConfig](){
#ifdef Q_OS_WIN
        QString command = "explorer";
        QProcess::startDetached(command, QStringList() << QDir::toNativeSeparators(nginxConfig["document_root"].toString()));
#endif
    });
}

void MainWindow::setupMySQLConfigurationPage()
{
    QObject::connect(ui->mysqlPortLineEdit, &QLineEdit::textChanged, [this]() {
        ui->mysqlPortWarning->setText("");
        QString text = ui->mysqlPortLineEdit->text();
        bool ok;
        int number = text.toInt(&ok);
        if (ok && (number < 1 || number > 65535)) {
            ui->mysqlPortLineEdit->setText(text.left(text.length() - 1));
        }
    });

    QJsonObject mysqlVesions = ServerManager::getInstance().getFacade().getAvailableVersions("mysql");
    for (auto it = mysqlVesions.begin(); it != mysqlVesions.end(); ++it) {
        ui->mysqlVersionSelect->addItem(it.key());
    }
    QJsonObject mysqlConfig = ServerManager::getInstance().getFacade().getServerConfiguration("mysql");
    int vIndex = ui->mysqlVersionSelect->findText(mysqlConfig["version"].toString());
    if (vIndex != -1) {
        ui->mysqlVersionSelect->setCurrentIndex(vIndex);
    } else{
        QString errMsg = "Failed to set active mysql version: version " + mysqlConfig["version"].toString() + " not found. ";
        throw std::runtime_error(errMsg.toStdString());
    }

    ui->mysqlPortLineEdit->setText(QString::number(mysqlConfig["port"].toDouble()));
}

void MainWindow::closeEvent(QCloseEvent *event)
{

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Exit", "Exit the application and terminate all services?",
                                  QMessageBox::Yes | QMessageBox::No);
    if(reply == QMessageBox::Yes){
        event->ignore();
        tasksController->exitApplication();
    }
    else{
        event->ignore();
    }

}
