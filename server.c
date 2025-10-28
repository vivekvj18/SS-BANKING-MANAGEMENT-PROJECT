// server.c

#include <sys/socket.h> // For socket, bind, listen, accept
#include <netinet/in.h> // For sockaddr_in, INADDR_ANY, htons
#include <arpa/inet.h>  // For inet_ntoa
#include <errno.h>      // For errno, EINTR
#include <signal.h>     // Defines struct sigaction, sigaction()
#include <sys/wait.h>   // Defines waitpid()
#include <stdlib.h>     // For exit, EXIT_FAILURE
#include <stdio.h>      // For printf, perror
#include <unistd.h>     // For fork, close, sys_close

#include "utils.h"
#include "structs.h"

#define PORT 8080
#define BACKLOG 10 // Max pending connections

// CRITICAL FIX: The definition of current_user is moved to utils.c.
// We keep the definition specific to this file to hold the session state in the CHILD process.
// NOTE: If your g++ continues to complain about multiple definition, you must change this line to:
// extern struct User current_user; // And remove the current_user definition in utils.c as well.
extern struct User current_user;

// Function declarations for functions defined in utils.c
extern int authenticate_and_set_user(char *username, char *password, int expected_role);
extern void serve_view_balance(int client_sd, struct Message *request);
extern void serve_deposit(int client_sd, struct Message *request);
extern void serve_withdraw(int client_sd, struct Message *request);
extern void serve_transfer(int client_sd, struct Message *request); 

// --- Signal Handler to Prevent Zombie Processes ---
void sigchld_handler(int s) {
    (void)s; // Silence unused parameter warning
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0); 
    errno = saved_errno;
}

// --- Child Process Handler (Concurrency) ---
void handle_client(int client_sd) {
    struct Message request, response;
    ssize_t bytes_read;
    int logged_in = 0;

    sys_write_string("\n[SERVER] Child process started. Waiting for login...\n");

    while ((bytes_read = sys_read(client_sd, &request, sizeof(struct Message))) > 0) {
        
        // --- Command Dispatch ---
        response.success_status = 0; 
        response.command = request.command; 

        switch (request.command) {
            case CMD_LOGIN:
                if (authenticate_and_set_user(request.data, request.data + MAX_NAME_LEN, request.source_id)) {
                    logged_in = 1;
                    response.success_status = 1;
                    response.source_id = current_user.id;
                    sys_write_string("[SERVER] Login successful.\n");
                } else {
                    sys_write_string("[SERVER] Login failed.\n");
                }
                break;
                
            case CMD_VIEW_BALANCE:
                if (logged_in && current_user.role == CUSTOMER) {
                    serve_view_balance(client_sd, &request);
                    continue; 
                } else {
                    sys_write_string("[SERVER] Unauthorized attempt to view balance.\n");
                }
                break;
            case CMD_APPLY_LOAN:
            case CMD_VIEW_LOAN_STATUS:
                if (logged_in && current_user.role == CUSTOMER) {
                    if (request.command == CMD_APPLY_LOAN) {
                        serve_apply_loan(client_sd, &request);
                    } else {
                        serve_view_loan_status(client_sd, &request);
                    }
                    continue; 
                } else {
                    sys_write_string("[SERVER] Unauthorized loan request.\n");
                }
                break;

            // --- NEW: Employee Loan Commands ---
            case CMD_PROCESS_LOAN:
            case CMD_VIEW_ASSIGNED_LOANS:
                if (logged_in && current_user.role == EMPLOYEE) {
                    if (request.command == CMD_PROCESS_LOAN) {
                        serve_process_loan(client_sd, &request);
                    } else {
                        serve_view_assigned_loans(client_sd, &request);
                    }
                    continue; 
                } else {
                    sys_write_string("[SERVER] Unauthorized loan processing request.\n");
                }
            case CMD_DEPOSIT:
            case CMD_WITHDRAW:
                if (logged_in && current_user.role == CUSTOMER) {
                    if (request.command == CMD_DEPOSIT) {
                        serve_deposit(client_sd, &request);
                    } else {
                        serve_withdraw(client_sd, &request);
                    }
                    continue; 
                } else {
                    sys_write_string("[SERVER] Unauthorized attempt to perform transaction.\n");
                }
                break;
            case CMD_ADD_CUSTOMER: // New Employee command
            case CMD_MODIFY_CUSTOMER: // New Employee command
                if (logged_in && current_user.role == EMPLOYEE) {
                    if (request.command == CMD_ADD_CUSTOMER) {
                        serve_add_customer(client_sd, &request);
                    } else {
                        serve_modify_customer(client_sd, &request);
                    }
                    continue; 
                } else {
                    sys_write_string("[SERVER] Unauthorized attempt to modify customer data.\n");
                }
                break;
            
            case CMD_TRANSFER: // Transfer Logic
                if (logged_in && current_user.role == CUSTOMER) {
                    serve_transfer(client_sd, &request);
                    continue;
                } else {
                    sys_write_string("[SERVER] Unauthorized attempt to transfer funds.\n");
                }
                break;

            case CMD_LOGOUT:
                logged_in = 0;
                current_user.id = 0;
                response.success_status = 1;
                sys_write_string("[SERVER] User logged out.\n");
                break;
                
            default:
                sys_write_string("[SERVER] Unknown command received.\n");
                break;
        }

        // Send generic response back to client (for commands like LOGIN, LOGOUT)
        sys_write(client_sd, &response, sizeof(struct Message));
    }

    sys_write_string("[SERVER] Client disconnected. Child process exiting.\n");
    sys_close(client_sd);
    exit(0); 
}

// --- Main Server Setup ---
int main() {
    int listen_sd, client_sd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    // Set up signal handler for zombie processes
    struct sigaction sa;
    sa.sa_handler = sigchld_handler; 
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    
    // 1. Create Socket 
    listen_sd = sys_socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sd < 0) {
        perror("[SERVER] Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // 2. Bind Socket
    if (sys_bind(listen_sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("[SERVER] Bind failed");
        sys_close(listen_sd);
        exit(EXIT_FAILURE);
    }
    
    // 3. Listen 
    if (sys_listen(listen_sd, BACKLOG) < 0) {
        perror("[SERVER] Listen failed");
        sys_close(listen_sd);
        exit(EXIT_FAILURE);
    }

    printf("[SERVER] Banking Server listening on port %d...\n", PORT);

    // Main loop to accept new clients
    while (1) {
        // 4. Accept connection 
        client_sd = sys_accept(listen_sd, (struct sockaddr *)&client_addr, &client_len);
        if (client_sd < 0) {
            if (errno == EINTR) continue; 
            perror("[SERVER] Accept failed");
            continue;
        }
        
        printf("[SERVER] Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // 5. Fork a new process (Concurrency)
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("[SERVER] Fork failed");
            sys_close(client_sd);
        } else if (pid == 0) {
            // Child Process: Handle the client connection
            sys_close(listen_sd); 
            handle_client(client_sd);
        } else {
            // Parent Process: Close the client socket and wait for the next connection
            sys_close(client_sd); 
        }
    }

    sys_close(listen_sd);
    return 0;
}