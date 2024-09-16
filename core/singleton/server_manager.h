#ifndef SERVER_MANAGER_H
#define SERVER_MANAGER_H

#include "../facade/server_facade.h"

class ServerManager {
public:
    static ServerManager& getInstance() {
        static ServerManager instance;
        return instance;
    }

    ServerFacade& getFacade() {
        return facade;
    }

private:
    ServerManager() {}
    ServerManager(const ServerManager&) = delete;
    ServerManager& operator=(const ServerManager&) = delete;

    ServerFacade facade;
};

#endif // SERVER_MANAGER_H
