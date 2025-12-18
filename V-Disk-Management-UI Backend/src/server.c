// #define _POSIX_C_SOURCE 200809L
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <ctype.h>
// #ifdef _WIN32
// #include <io.h>
// #else
// #include <unistd.h>
// #endif
// #include <errno.h>
// #include <stdint.h>
// #ifdef _WIN32
// #include <winsock2.h>
// #include <ws2tcpip.h>
// #pragma comment(lib, "ws2_32.lib")
// #else
// #include <netinet/in.h>
// #include <sys/socket.h>
// #include <sys/types.h>
// #endif
// #include "disk.h"
// #include "utils.h"
// #include "system_disk.h"
// #include "server.h" // Include server.h so we can expose run_server()

// #define RECV_BUF 8192
// #define SEND_BUF 8192

// // cross platform block
// #ifdef _WIN32
// #include <string.h>
// #define strncasecmp _strnicmp
// #else
// #include <strings.h>
// #endif
// // end cross platform block

// typedef struct {
//     char method[8];
//     char path[256];
//     char protocol[16];
//     int content_length;
//     char body[RECV_BUF];
// } HttpRequest;

// static void send_json(int client_fd, int status, const char* json_data, const char* error_msg) {
//     char header[SEND_BUF];
//     StrBuf body;
//     sb_init(&body, 1024);
//     if (json_data) {
//         sb_appendf(&body, "{ \"success\": 1, \"data\": %s, \"error\": null }", json_data);
//     } else if (error_msg) {
//         sb_appendf(&body, "{ \"success\": 0, \"data\": null, \"error\": \"%s\" }", error_msg);
//     } else {
//         sb_append(&body, "{ \"success\": 1, \"data\": null, \"error\": null }");
//     }
//     char* b = sb_take(&body);
//     snprintf(header, sizeof(header),
//              "HTTP/1.1 %d OK\r\n"
//              "Content-Type: application/json\r\n"
//              "Content-Length: %zu\r\n"
//              "Connection: close\r\n\r\n",
//              status, strlen(b));
//     send(client_fd, header, strlen(header), 0);
//     send(client_fd, b, strlen(b), 0);
//     free(b);
// }

// static void send_json_kv(int client_fd, int status, const char* kv_pairs) {
//     char buf[1024];
//     snprintf(buf, sizeof(buf), "{ %s }", kv_pairs);
//     send_json(client_fd, status, buf, NULL);
// }

// static int parse_request(int fd, HttpRequest* req) {
//     char buf[RECV_BUF];
//     int n = recv(fd, buf, sizeof(buf)-1, 0);
//     if (n <= 0) return -1;
//     buf[n] = '\0';

//     char *line_end = strstr(buf, "\r\n");
//     if (!line_end) return -1;
//     *line_end = '\0';
//     sscanf(buf, "%7s %255s %15s", req->method, req->path, req->protocol);

//     char* p = line_end + 2;
//     req->content_length = 0;
//     while (1) {
//         char* next = strstr(p, "\r\n");
//         if (!next) return -1;
//         if (next == p) { 
//             p = next + 2;
//             break;
//         }
//         *next = '\0';
//         if (strncasecmp(p, "Content-Length:", 15) == 0) {
//             req->content_length = atoi(p + 15);
//         }
//         p = next + 2;
//     }

//     int remaining = n - (int)(p - buf);
//     if (remaining > 0) {
//         int m = remaining < (int)sizeof(req->body)-1 ? remaining : (int)sizeof(req->body)-1;
//         memcpy(req->body, p, m);
//         req->body[m] = '\0';
//     } else {
//         req->body[0] = '\0';
//     }
//     return 0;
// }

// static void handle_client(int client_fd) {
//     HttpRequest req;
//     if (parse_request(client_fd, &req) != 0) {
//         send_json(client_fd, 400, NULL, "Invalid request");
//         return;
//     }

//     const char* m = req.method;
//     const char* path = req.path;
//     const char* body = req.body;

//     char* response = NULL;

//     if (strcmp(m, "POST") == 0 && strcmp(path, "/allocate/contiguous") == 0) {
//         int size = 0; parse_json_int(req.body, "size", &size);
//         if (size <= 0) { send_json(client_fd, 400, NULL, "size must be positive"); return; }
//         int fid = 0;
//         int r = disk_allocate_contiguous(size, &fid);
//         if (r == 0) {
//             char tmp[64]; snprintf(tmp, sizeof(tmp), "\"fileId\": %d", fid);
//             send_json_kv(client_fd, 200, tmp);
//         } else {
//             send_json(client_fd, 409, NULL, "No contiguous space available");
//         }
//         return;
//     }

//     if (strcmp(m, "POST") == 0 && strcmp(path, "/allocate/fragmented") == 0) {
//         int size = 0; parse_json_int(req.body, "size", &size);
//         if (size <= 0) { send_json(client_fd, 400, NULL, "size must be positive"); return; }
//         int fid = 0;
//         int r = disk_allocate_fragmented(size, &fid);
//         if (r == 0) {
//             char tmp[64]; snprintf(tmp, sizeof(tmp), "\"fileId\": %d", fid);
//             send_json_kv(client_fd, 200, tmp);
//         } else {
//             send_json(client_fd, 409, NULL, "Not enough free blocks");
//         }
//         return;
//     }

//     if (strcmp(m, "POST") == 0 && strcmp(path, "/allocate/custom") == 0) {
//         int size = 0; parse_json_int(req.body, "size", &size);
//         char strategy[32] = {0};
//         if (parse_json_string(req.body, "strategy", strategy, sizeof(strategy)) != 0) {
//             strncpy(strategy, "first-fit", sizeof(strategy)-1);
//         }
//         if (size <= 0) { send_json(client_fd, 400, NULL, "size must be positive"); return; }
//         int fid = 0;
//         int r = disk_allocate_custom(size, strategy, &fid);
//         if (r == 0) {
//             char tmp[128]; snprintf(tmp, sizeof(tmp), "\"fileId\": %d, \"strategy\": \"%s\"", fid, strategy);
//             send_json_kv(client_fd, 200, tmp);
//         } else {
//             send_json(client_fd, 409, NULL, "Allocation failed");
//         }
//         return;
//     }

//     if (strcmp(m, "DELETE") == 0 && strncmp(path, "/file/", 6) == 0) {
//         int id = atoi(path + 6);
//         if (id <= 0) { send_json(client_fd, 400, NULL, "Invalid file id"); return; }
//         int r = disk_logical_delete(id);
//         if (r == 0) send_json(client_fd, 200, "{ \"deleted\": 1 }", NULL);
//         else send_json(client_fd, 404, NULL, "File not found");
//         return;
//     }

//     if (strcmp(m, "POST") == 0 && strcmp(path, "/undelete/last") == 0) {
//         int r = disk_undelete_last();
//         if (r == 0) send_json(client_fd, 200, "{ \"undeleted\": 1 }", NULL);
//         else send_json(client_fd, 409, NULL, "No deletions to restore or space unavailable");
//         return;
//     }

//     if (strcmp(m, "POST") == 0 && strcmp(path, "/defragment") == 0) {
//         int r = disk_defragment();
//         if (r == 0) send_json(client_fd, 200, "{ \"defragmented\": 1 }", NULL);
//         else send_json(client_fd, 500, NULL, "Defragmentation failed");
//         return;
//     }

//     if (strcmp(m, "POST") == 0 && strcmp(path, "/mark-bad") == 0) {
//         int count = 0; parse_json_int(req.body, "count", &count);
//         if (count <= 0) { send_json(client_fd, 400, NULL, "count must be positive"); return; }
//         int r = disk_mark_random_bad(count);
//         if (r == 0) send_json(client_fd, 200, "{ \"marked\": 1 }", NULL);
//         else send_json(client_fd, 409, NULL, "Unable to mark requested number as bad");
//         return;
//     }

//     if (strcmp(m, "GET") == 0 && strcmp(path, "/fragmentation") == 0) {
//         double p = disk_fragmentation_percent();
//         char tmp[128]; snprintf(tmp, sizeof(tmp), "{ \"fragmentationPercent\": %.2f }", p);
//         send_json(client_fd, 200, tmp, NULL);
//         return;
//     }

//     if (strcmp(m, "GET") == 0 && strcmp(path, "/api/system-disk") == 0) {
//         response = handle_get_system_disk_info();
//     } else if (strcmp(m, "POST") == 0 && strcmp(path, "/api/create-file") == 0) {
//         response = handle_create_file(body);
//     } else if (strcmp(m, "POST") == 0 && strcmp(path, "/api/delete-file") == 0) {
//         response = handle_delete_file(body);
//     } else if (strcmp(m, "GET") == 0 && strcmp(path, "/api/disk/state") == 0) {
//         char* s = disk_get_state();
//         if (s) { send_json(client_fd, 200, s, NULL); free(s); }
//         else send_json(client_fd, 500, NULL, "Unable to build state");
//         return;
//     }

//     if (strcmp(m, "GET") == 0 && strcmp(path, "/api/disk/files") == 0) {
//         char* s = disk_get_files();
//         if (s) { send_json(client_fd, 200, s, NULL); free(s); }
//         else send_json(client_fd, 500, NULL, "Unable to build files");
//         return;
//     }

//     if (strcmp(m, "GET") == 0 && strcmp(path, "/api/disk/stats") == 0) {
//         char* s = disk_get_stats();
//         if (s) { send_json(client_fd, 200, s, NULL); free(s); }
//         else send_json(client_fd, 500, NULL, "Unable to build stats");
//         return;
//     }

//     if (strcmp(m, "GET") == 0 && strcmp(path, "/api/disk/logs") == 0) {
//         char* s = disk_get_logs();
//         if (s) { send_json(client_fd, 200, s, NULL); free(s); }
//         else send_json(client_fd, 500, NULL, "Unable to build logs");
//         return;
//     }

//     if (strcmp(m, "POST") == 0 && strcmp(path, "/api/disk/reset") == 0) {
//         int r = disk_reset();
//         if (r == 0) send_json(client_fd, 200, "{ \"reset\": 1 }", NULL);
//         else send_json(client_fd, 500, NULL, "Reset failed");
//         return;
//     }

//     if (strcmp(m, "POST") == 0 && strcmp(path, "/api/repair") == 0) {
//         int r = disk_repair();
//         if (r == 0) send_json(client_fd, 200, "{ \"repaired\": 1 }", NULL);
//         else send_json(client_fd, 500, NULL, "Repair failed");
//         return;
//     }

//     // Not found
//     send_json(client_fd, 404, NULL, "Endpoint not found");
// }

// char* handle_get_system_disk_info() {
//     SystemDiskInfo info;
//     if (get_system_disk_info(&info) != 0) {
//         return utils_json_error("Failed to get system disk information");
//     }
    
//     StrBuf sb;
//     if (sb_init(&sb, 512) != 0) return utils_json_error("Out of memory");
    
//     sb_appendf(&sb, "{"
//         "\"total\": %.0f,"
//         "\"free\": %.0f,"
//         "\"used\": %.0f,"
//         "\"usedPercentage\": %.2f,"
//         "\"freePercentage\": %.2f,"
//         "\"badSectors\": %d,"
//         "\"path\": \"%s\""
//         "}",
//         info.total_gb,
//         info.free_gb,
//         info.used_gb,
//         info.used_percentage,
//         100.0 - info.used_percentage,
//         info.bad_sectors,
//         info.path
//     );
    
//     return sb_take(&sb);
// }

// char* handle_create_file(const char* body) {
//     // Parse JSON to get filename and size
//     char filename[256] = {0};
//     int size = 0;
    
//     // Simple JSON parsing (in a real implementation, you'd use a proper JSON library)
//     const char* name_ptr = strstr(body, "\"filename\"");
//     if (name_ptr) {
//         name_ptr = strchr(name_ptr, ':');
//         if (name_ptr) {
//             name_ptr = strchr(name_ptr, '"');
//             if (name_ptr) {
//                 name_ptr++;
//                 const char* end_quote = strchr(name_ptr, '"');
//                 if (end_quote && end_quote - name_ptr < sizeof(filename) - 1) {
//                     strncpy(filename, name_ptr, end_quote - name_ptr);
//                     filename[end_quote - name_ptr] = '\0';
//                 }
//             }
//         }
//     }
    
//     const char* size_ptr = strstr(body, "\"size\"");
//     if (size_ptr) {
//         size_ptr = strchr(size_ptr, ':');
//         if (size_ptr) {
//             size = atoi(size_ptr + 1);
//         }
//     }
    
//     if (strlen(filename) == 0) {
//         return utils_json_error("Filename is required");
//     }
    
//     // Create the file on the actual disk
//     if (create_file_on_disk(filename, size) != 0) {
//         return utils_json_error("Failed to create file");
//     }
    
//     StrBuf sb;
//     if (sb_init(&sb, 256) != 0) return utils_json_error("Out of memory");
    
//     sb_appendf(&sb, "{\"success\": true, \"message\": \"File %s created successfully\"}", filename);
//     return sb_take(&sb);
// }

// char* handle_delete_file(const char* body) {
//     // Parse JSON to get filename
//     char filename[256] = {0};
    
//     // Simple JSON parsing
//     const char* name_ptr = strstr(body, "\"filename\"");
//     if (name_ptr) {
//         name_ptr = strchr(name_ptr, ':');
//         if (name_ptr) {
//             name_ptr = strchr(name_ptr, '"');
//             if (name_ptr) {
//                 name_ptr++;
//                 const char* end_quote = strchr(name_ptr, '"');
//                 if (end_quote && end_quote - name_ptr < sizeof(filename) - 1) {
//                     strncpy(filename, name_ptr, end_quote - name_ptr);
//                     filename[end_quote - name_ptr] = '\0';
//                 }
//             }
//         }
//     }
    
//     if (strlen(filename) == 0) {
//         return utils_json_error("Filename is required");
//     }
    
//     // Delete the file from the actual disk
//     if (delete_file_from_disk(filename) != 0) {
//         return utils_json_error("Failed to delete file");
//     }
    
//     StrBuf sb;
//     if (sb_init(&sb, 256) != 0) return utils_json_error("Out of memory");
    
//     sb_appendf(&sb, "{\"success\": true, \"message\": \"File %s deleted successfully\"}", filename);
//     return sb_take(&sb);
// }

// int run_server(int port) {
// #ifdef _WIN32
//     WSADATA wsa;
//     if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
//         fprintf(stderr, "WSAStartup failed.\n");
//         return 1;
//     }
// #endif

//     const char* persist_env = getenv("DATA_FILE");
//     disk_init(persist_env && persist_env[0] ? persist_env : "disk_state.json");

//     int server_fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (server_fd < 0) { 
//         perror("socket");
// #ifdef _WIN32
//         WSACleanup();
// #endif
//         return 1;
//     }

//     int opt = 1;
//     setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

//     struct sockaddr_in addr;
//     memset(&addr, 0, sizeof(addr));
//     addr.sin_family = AF_INET;
//     addr.sin_addr.s_addr = htonl(INADDR_ANY);
//     addr.sin_port = htons((uint16_t)port);

//     if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
//         perror("bind");
//         close(server_fd);
// #ifdef _WIN32
//         WSACleanup();
// #endif
//         return 1;
//     }

//     if (listen(server_fd, 16) < 0) {
//         perror("listen");
//         close(server_fd);
// #ifdef _WIN32
//         WSACleanup();
// #endif
//         return 1;
//     }

//     printf("Disk Management Simulator server listening on port %d\n", port);

//     while (1) {
//         struct sockaddr_in cli;
//         socklen_t clilen = sizeof(cli);
//         int client_fd = accept(server_fd, (struct sockaddr*)&cli, &clilen);
//         if (client_fd < 0) {
//             perror("accept");
//             continue;
//         }
//         handle_client(client_fd);
//         close(client_fd);
//     }

//     close(server_fd);
//     disk_shutdown();
// #ifdef _WIN32
//     WSACleanup();
// #endif
//     return 0;
// }
