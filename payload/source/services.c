#include "ps4.h"
#include "services.h"

#define PLUTANA_ROOT "/data/Plutana"
#define PLUTANA_LOG  "/data/Plutana/plutana.log"
#define FTP_PORT 2121
#define KLOG_PORT 3232
#define FTP_USER "plutana"
#define FTP_PASS "Plutana14.02!"

static int g_net_ready = 0;

static void log_write(const char *level, const char *message) {
    char line[512];
    int n = snprintf(line, sizeof(line), "[PLUTANA] [%s] %s\n", level, message);
    if (n <= 0) return;
    int fd = open(PLUTANA_LOG, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd >= 0) {
        (void)write(fd, line, (size_t)n);
        close(fd);
    }
}

void plutana_log(const char *level, const char *message) {
    if (level && message) log_write(level, message);
}

static int send_all(int socket, const char *data, int length) {
    int sent = 0;
    while (sent < length) {
        int n = sceNetSend(socket, data + sent, (size_t)(length - sent), 0);
        if (n <= 0) return -1;
        sent += n;
    }
    return sent;
}

static int send_text(int socket, const char *text) {
    return send_all(socket, text, (int)strlen(text));
}

static int recv_line(int socket, char *out, int capacity) {
    int used = 0;
    while (used < capacity - 1) {
        char ch;
        int n = sceNetRecv(socket, &ch, 1, 0);
        if (n <= 0) return -1;
        if (ch == '\n') break;
        if (ch != '\r') out[used++] = ch;
    }
    out[used] = 0;
    return used;
}

static int make_listener(int port) {
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_len = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_port = sceNetHtons((uint16_t)port);
    address.sin_addr.s_addr = IN_ADDR_ANY;

    int socket = sceNetSocket("plutana-listener", AF_INET, SOCK_STREAM, 0);
    if (socket < 0) return -1;

    int yes = 1;
    (void)sceNetSetsockopt(socket, SOL_SOCKET, SCE_NET_SO_REUSEADDR, &yes, sizeof(yes));
    if (sceNetBind(socket, (struct sockaddr *)&address, sizeof(address)) < 0 ||
        sceNetListen(socket, 4) < 0) {
        sceNetSocketClose(socket);
        return -1;
    }
    return socket;
}

static int open_passive_listener(int *port_out) {
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_len = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_port = 0;
    address.sin_addr.s_addr = IN_ADDR_ANY;

    int socket = sceNetSocket("plutana-passive", AF_INET, SOCK_STREAM, 0);
    if (socket < 0) return -1;
    if (sceNetBind(socket, (struct sockaddr *)&address, sizeof(address)) < 0 ||
        sceNetListen(socket, 1) < 0) {
        sceNetSocketClose(socket);
        return -1;
    }

    unsigned int length = sizeof(address);
    memset(&address, 0, sizeof(address));
    if (sceNetGetsockname(socket, (struct sockaddr *)&address, &length) < 0) {
        sceNetSocketClose(socket);
        return -1;
    }
    *port_out = sceNetNtohs(address.sin_port);
    return socket;
}

/* Resolve paths only beneath /data/Plutana. Parent traversal is rejected. */
static int ftp_path(const char *cwd, const char *arg, char *out, size_t out_size) {
    char relative[256];
    size_t used = 0;
    if (!arg || !arg[0]) arg = ".";

    if (strstr(arg, "..") || strchr(arg, '\\')) return -1;
    if (arg[0] == '/') {
        arg++;
    } else if (cwd && cwd[0]) {
        int n = snprintf(relative, sizeof(relative), "%s/%s", cwd, arg);
        if (n < 0 || (size_t)n >= sizeof(relative)) return -1;
        arg = relative;
    }

    while (*arg == '/') arg++;
    if (snprintf(out, out_size, "%s/%s", PLUTANA_ROOT, arg) >= (int)out_size)
        return -1;
    used = strlen(out);
    (void)used;
    return 0;
}

static int ftp_data_accept(int passive_socket) {
    struct sockaddr peer;
    unsigned int length = sizeof(peer);
    int data = sceNetAccept(passive_socket, &peer, &length);
    sceNetSocketClose(passive_socket);
    return data;
}

static void ftp_list(int control, int passive, const char *path) {
    send_text(control, "150 Opening data connection.\r\n");
    int data = ftp_data_accept(passive);
    if (data < 0) {
        send_text(control, "425 Cannot open data connection.\r\n");
        return;
    }
    DIR *dir = opendir(path);
    if (!dir) {
        send_text(control, "550 Directory unavailable.\r\n");
        sceNetSocketClose(data);
        return;
    }
    struct dirent *entry;
    char line[320];
    while ((entry = readdir(dir)) != NULL) {
        int n = snprintf(line, sizeof(line), "%s\r\n", entry->d_name);
        if (n > 0 && send_all(data, line, n) < 0) break;
    }
    closedir(dir);
    sceNetSocketClose(data);
    send_text(control, "226 Transfer complete.\r\n");
}

static void ftp_retr(int control, int passive, const char *path) {
    int fd = open(path, O_RDONLY, 0);
    if (fd < 0) {
        send_text(control, "550 File unavailable.\r\n");
        sceNetSocketClose(passive);
        return;
    }
    send_text(control, "150 Opening binary data connection.\r\n");
    int data = ftp_data_accept(passive);
    if (data < 0) {
        close(fd);
        send_text(control, "425 Cannot open data connection.\r\n");
        return;
    }
    char buffer[8192];
    ssize_t n;
    while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
        if (send_all(data, buffer, (int)n) < 0) break;
    }
    close(fd);
    sceNetSocketClose(data);
    send_text(control, "226 Transfer complete.\r\n");
}

static void ftp_stor(int control, int passive, const char *path) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        send_text(control, "550 Cannot create file.\r\n");
        sceNetSocketClose(passive);
        return;
    }
    send_text(control, "150 Ready to receive file.\r\n");
    int data = ftp_data_accept(passive);
    if (data < 0) {
        close(fd);
        send_text(control, "425 Cannot open data connection.\r\n");
        return;
    }
    char buffer[8192];
    int n;
    while ((n = sceNetRecv(data, buffer, sizeof(buffer), 0)) > 0) {
        if (write(fd, buffer, (size_t)n) != n) break;
    }
    close(fd);
    sceNetSocketClose(data);
    send_text(control, "226 Transfer complete.\r\n");
}

static void ftp_client(int control) {
    char line[512], command[16], argument[384], cwd[192] = "";
    int authenticated = 0;
    int user_ok = 0;
    int passive = -1;
    send_text(control, "220 Plutana-HEN FTP service ready.\r\n");

    while (recv_line(control, line, sizeof(line)) >= 0) {
        command[0] = argument[0] = 0;
        (void)sscanf(line, "%15s %383[^\r\n]", command, argument);

        if (!strcmp(command, "USER")) {
            user_ok = !strcmp(argument, FTP_USER);
            authenticated = 0;
            send_text(control, "331 Password required.\r\n");
        } else if (!strcmp(command, "PASS")) {
            authenticated = user_ok && !strcmp(argument, FTP_PASS);
            if (authenticated) send_text(control, "230 Login successful.\r\n");
            else send_text(control, "530 Login incorrect.\r\n");
        } else if (!strcmp(command, "QUIT")) {
            send_text(control, "221 Goodbye.\r\n");
            break;
        } else if (!authenticated) {
            send_text(control, "530 Please login first.\r\n");
        } else if (!strcmp(command, "SYST")) {
            send_text(control, "215 UNIX Type: L8.\r\n");
        } else if (!strcmp(command, "FEAT")) {
            send_text(control, "211-Features\r\n PASV\r\n SIZE\r\n211 End\r\n");
        } else if (!strcmp(command, "TYPE")) {
            send_text(control, "200 Type set.\r\n");
        } else if (!strcmp(command, "NOOP")) {
            send_text(control, "200 OK.\r\n");
        } else if (!strcmp(command, "PWD") || !strcmp(command, "XPWD")) {
            char response[256];
            (void)snprintf(response, sizeof(response), "257 \"/%s\" is current directory.\r\n", cwd);
            send_text(control, response);
        } else if (!strcmp(command, "CWD")) {
            char path[512];
            if (ftp_path(cwd, argument, path, sizeof(path)) == 0) {
                DIR *dir = opendir(path);
                if (dir) {
                    closedir(dir);
                    if (argument[0] == '/') {
                        strncpy(cwd, argument + 1, sizeof(cwd) - 1);
                    } else {
                        strncpy(cwd, argument, sizeof(cwd) - 1);
                    }
                    cwd[sizeof(cwd) - 1] = 0;
                    send_text(control, "250 Directory changed.\r\n");
                } else send_text(control, "550 Directory unavailable.\r\n");
            } else send_text(control, "550 Invalid path.\r\n");
        } else if (!strcmp(command, "PASV")) {
            if (passive >= 0) sceNetSocketClose(passive);
            int port = 0;
            passive = open_passive_listener(&port);
            if (passive < 0) {
                send_text(control, "425 Cannot enter passive mode.\r\n");
            } else {
                SceNetCtlInfo info;
                memset(&info, 0, sizeof(info));
                char ip[16] = "127.0.0.1";
                if (sceNetCtlGetInfo(SCE_NET_CTL_INFO_IP_ADDRESS, &info) == 0 &&
                    info.ip_address[0]) {
                    strncpy(ip, info.ip_address, sizeof(ip) - 1);
                    ip[sizeof(ip) - 1] = 0;
                }
                int a=127,b=0,c=0,d=1;
                (void)sscanf(ip, "%d.%d.%d.%d", &a,&b,&c,&d);
                char response[128];
                (void)snprintf(response, sizeof(response),
                    "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).\r\n",
                    a,b,c,d,port / 256,port % 256);
                send_text(control, response);
            }
        } else if (!strcmp(command, "LIST") || !strcmp(command, "NLST")) {
            char path[512];
            if (passive < 0) send_text(control, "425 Use PASV first.\r\n");
            else if (ftp_path(cwd, argument, path, sizeof(path)) < 0) {
                sceNetSocketClose(passive); passive = -1;
                send_text(control, "550 Invalid path.\r\n");
            } else {
                int data_listener = passive; passive = -1;
                ftp_list(control, data_listener, path);
            }
        } else if (!strcmp(command, "RETR") || !strcmp(command, "STOR")) {
            char path[512];
            if (passive < 0) send_text(control, "425 Use PASV first.\r\n");
            else if (ftp_path(cwd, argument, path, sizeof(path)) < 0) {
                sceNetSocketClose(passive); passive = -1;
                send_text(control, "550 Invalid path.\r\n");
            } else {
                int data_listener = passive; passive = -1;
                if (!strcmp(command, "RETR")) ftp_retr(control, data_listener, path);
                else ftp_stor(control, data_listener, path);
            }
        } else if (!strcmp(command, "SIZE")) {
            char path[512];
            struct stat st;
            if (ftp_path(cwd, argument, path, sizeof(path)) == 0 &&
                stat(path, &st) == 0) {
                char response[96];
                (void)snprintf(response, sizeof(response), "213 %lld\r\n", (long long)st.st_size);
                send_text(control, response);
            } else send_text(control, "550 File unavailable.\r\n");
        } else {
            send_text(control, "502 Command not implemented.\r\n");
        }
    }

    if (passive >= 0) sceNetSocketClose(passive);
    sceNetSocketClose(control);
}

static void *ftp_worker(void *unused) {
    UNUSED(unused);
    for (;;) {
        int listener = make_listener(FTP_PORT);
        if (listener < 0) {
            log_write("ERROR", "FTP listener could not bind port 2121; retrying.");
            sceKernelUsleep(3000000);
            continue;
        }
        log_write("INFO", "FTP service listening on TCP port 2121.");
        for (;;) {
            struct sockaddr peer;
            unsigned int length = sizeof(peer);
            int client = sceNetAccept(listener, &peer, &length);
            if (client < 0) break;
            ftp_client(client);
        }
        sceNetSocketClose(listener);
        sceKernelUsleep(1000000);
    }
    return NULL;
}

static void *klog_worker(void *unused) {
    UNUSED(unused);
    for (;;) {
        int listener = make_listener(KLOG_PORT);
        if (listener < 0) {
            log_write("ERROR", "Klog listener could not bind port 3232; retrying.");
            sceKernelUsleep(3000000);
            continue;
        }
        log_write("INFO", "Klog stream listening on TCP port 3232.");
        for (;;) {
            struct sockaddr peer;
            unsigned int length = sizeof(peer);
            int client = sceNetAccept(listener, &peer, &length);
            if (client < 0) break;
            send_text(client, "Plutana-HEN userland log stream\r\n");
            int fd = open(PLUTANA_LOG, O_RDONLY, 0);
            if (fd >= 0) {
                char buffer[2048];
                ssize_t n;
                while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
                    if (send_all(client, buffer, (int)n) < 0) break;
                }
                close(fd);
            }
            sceNetSocketClose(client);
        }
        sceNetSocketClose(listener);
        sceKernelUsleep(1000000);
    }
    return NULL;
}

int plutana_services_start(void) {
    if (!g_net_ready) {
        initNetwork();
        if (!sceNetSocket || !sceNetBind || !sceNetListen ||
            !sceNetAccept || !sceNetSend || !sceNetRecv) {
            log_write("ERROR", "libSceNet exports unavailable; services not started.");
            return -1;
        }
        (void)sceNetCtlInit();
        g_net_ready = 1;
    }

    (void)mkdir("/data/Plutana", 0777);
    log_write("INFO", "Plutana-HEN 14.02 userland services starting.");

    ScePthread ftp_thread = NULL;
    ScePthread klog_thread = NULL;
    if (scePthreadCreate(&ftp_thread, NULL, ftp_worker, NULL, "plutana_ftp") != 0) {
        log_write("ERROR", "FTP worker thread creation failed.");
        return -1;
    }
    (void)scePthreadDetach(ftp_thread);

    if (scePthreadCreate(&klog_thread, NULL, klog_worker, NULL, "plutana_klog") != 0) {
        log_write("ERROR", "Klog worker thread creation failed.");
        return -1;
    }
    (void)scePthreadDetach(klog_thread);

    log_write("INFO", "Services launched: FTP 2121, Klog 3232.");
    return 0;
}
