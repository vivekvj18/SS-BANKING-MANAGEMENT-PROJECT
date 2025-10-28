// utils.h

#ifndef UTILS_H
#define UTILS_H

#include <unistd.h>
#include <sys/socket.h> // For socketlen_t and sockaddr structures
#include "structs.h"

// --- File I/O System Call Wrappers ---
int sys_open(const char *pathname, int flags);
ssize_t sys_read(int fd, void *buf, size_t count);
ssize_t sys_write(int fd, const void *buf, size_t count);
off_t sys_lseek(int fd, off_t offset, int whence);
int sys_close(int fd);

// --- Socket System Call Wrappers ---
int sys_socket(int domain, int type, int protocol);
int sys_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int sys_listen(int sockfd, int backlog);
int sys_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int sys_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

// --- Synchronization: File Locking Wrappers ---
int sys_lock_record(int fd, int record_index, int type); // Type: F_RDLCK or F_WRLCK
int sys_unlock_record(int fd, int record_index);

// --- Server Service Functions (Defined in utils.c, Called from server.c) ---
int authenticate_and_set_user(char *username, char *password, int expected_role);
void serve_view_balance(int client_sd, struct Message *request);
void serve_deposit(int client_sd, struct Message *request);
void serve_withdraw(int client_sd, struct Message *request);
void serve_transfer(int client_sd, struct Message *request); 
void serve_add_customer(int client_sd, struct Message *request); // <-- NEW
void serve_modify_customer(int client_sd, struct Message *request);

// --- General Utilities ---
ssize_t sys_write_string(const char *s);
int get_input(char *buffer, size_t size);
void change_password_flow();
void print_menu(int role);

#endif