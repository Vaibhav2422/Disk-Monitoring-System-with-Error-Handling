#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include "disk.h"
#include "server.h"
#include "system_disk.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

int main(int argc, char** argv) {
    // Get port from environment, default to 8080
    const char* port_env = getenv("PORT");
    int port = port_env ? atoi(port_env) : 8080;
    if (port <= 0) port = 8080;

#ifdef _WIN32
    // Initialize Windows sockets
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        return 1;
    }
#endif

    // Initialize disk persistence
    const char* persist_env = getenv("DATA_FILE");
    disk_init(persist_env && persist_env[0] ? persist_env : "disk_state.json");

    // Test system disk info
    SystemDiskInfo sys_info;
    if (get_system_disk_info(&sys_info) == 0) {
        printf("System Disk Info:\n");
        printf("  Path: %s\n", sys_info.path);
        printf("  Total: %.2f GB\n", sys_info.total_gb);
        printf("  Used: %.2f GB (%.2f%%)\n", sys_info.used_gb, sys_info.used_percentage);
        printf("  Free: %.2f GB\n", sys_info.free_gb);
        printf("  Bad sectors: %d\n", sys_info.bad_sectors);
    } else {
        printf("Failed to get system disk info\n");
    }

    // Run the server
    int rc = run_server(port);

    // Shutdown disk
    disk_shutdown();

#ifdef _WIN32
    // Cleanup Windows sockets
    WSACleanup();
#endif

    return rc;
}