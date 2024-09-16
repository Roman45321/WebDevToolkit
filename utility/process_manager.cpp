#include "process_manager.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QCoreApplication>
#include <QtCore>

#ifdef Q_OS_WIN
QList<DWORD> Process_manager::getChildPIDs(DWORD parentPID) {
    QList<DWORD> childPIDs;
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        QString errMsg = "Failed to get child process id's: an error when taking process snapshot.";
        throw std::runtime_error(errMsg.toStdString());
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        QString errMsg = "Failed to get child process id's: an error when getting the first process.";
        throw std::runtime_error(errMsg.toStdString());
    }

    do {
        if (pe32.th32ParentProcessID == parentPID) {
            childPIDs.append(pe32.th32ProcessID);
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return childPIDs;
}

void Process_manager::killChildProcessesRecursively(qint64 processid) {
    DWORD pid = DWORD(processid);
    if (pid == 0) {
        QString errMsg = "Failed to kill child processes: parent process has no PID.";
        throw std::runtime_error(errMsg.toStdString());
    }

    QList<DWORD> childPIDs = getChildPIDs(pid);
    foreach (DWORD childPID, childPIDs) {

        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, childPID);
        if (hProcess != NULL) {
            killChildProcessesRecursively(childPID);
            TerminateProcess(hProcess, 1);
            CloseHandle(hProcess);
        }
    }
}
#endif
