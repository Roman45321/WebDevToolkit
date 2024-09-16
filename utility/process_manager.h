#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include "qglobal.h"
#include <QString>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#endif

class Process_manager {
public:
#ifdef Q_OS_WIN
    static QList<DWORD> getChildPIDs(DWORD parentPID);
    static void killChildProcessesRecursively(qint64 processid);
#endif
};

#endif // PROCESS_MANAGER_H
