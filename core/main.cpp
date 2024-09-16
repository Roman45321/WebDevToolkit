#include "../gui/views/mainwindow.h"
#include "config/configuration_manager.h"
#include "singleton/server_manager.h"
#include <QJsonDocument>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QEventLoop>
#include <QTimer>
#include <QPushButton>
#include <QTimer>
#include <QMessageBox>
#include <QThread>
#include <QStyleFactory>
#include <QWidget>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Fusion"));
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(42, 42, 42));
    darkPalette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    a.setPalette(darkPalette);

    ConfigurationManager& configManager = ConfigurationManager::getInstance();
    configManager.loadConfiguration("config.json");
    try {
        ServerManager::getInstance().getFacade().loadConfigurations(configManager.getConfiguration());

    } catch (const std::runtime_error &e) {
        QMessageBox::critical(nullptr, "Failed to load configuration from file", e.what());

    }
    MainWindow w;
    w.show();
    return a.exec();
}
