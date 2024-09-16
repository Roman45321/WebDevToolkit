#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../controllers/tasks_controller.h"
#include <QMainWindow>
#include <QProgressDialog>
#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QCloseEvent>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
public slots:
    void handleError(const QString& errorTitle, const QString& errorMessage);
    void onItemSelectionChanged();
    void setServerIndicator(const QString& serverName, bool isRunning);
    void onDisplayServerWarning(const QString& serverName, const QString& errorMessage);
private slots:
    void onStartApacheButtonClicked();
    void onStartMySQLButtonClicked();
    void onStopAllServersButtonClicked();
    void onStartAllServersButtonClicked();
    void onExitButtonClicked();
    void onStopApacheButtonClicked();
    void onStopMySQLButtonClicked();
    void onSaveNginxConfigurationButtonClicked();
    void onSaveApacheConfigurationButtonClicked();
    void onSaveMySQLConfigurationButtonClicked();
    void onStartNginxButtonClicked();
    void onStopNginxButtonClicked();

private:
    Ui::MainWindow *ui;
    TasksController *tasksController;
    QProgressDialog *progressDialog;

    void traverseTree(QTreeWidgetItem *parentItem, int &pageIndex);
    void setupApacheConfigurationPage();
    void setupNginxConfigurationPage();
    void setupMySQLConfigurationPage();

protected:
    void closeEvent(QCloseEvent *event) override;
};
#endif // MAINWINDOW_H
