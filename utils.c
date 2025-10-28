// utils.c

#include <fcntl.h>      // For open flags, fcntl, F_RDLCK, F_WRLCK, F_UNLCK
#include <string.h>     // For string operations (strcmp, strcpy, etc.)
#include <unistd.h>     // For read, write, lseek, close, fork, etc.
#include <stdlib.h>     // For atoi, exit
#include <stdio.h>      // For standard I/O (TEMPORARY), perror, sprintf
#include <sys/socket.h> // For socket structures
#include <sys/types.h>  // For off_t, pid_t, ssize_t, size_t
#include "utils.h"
#include "structs.h" 

// ====================================================================
// GLOBAL VARIABLE DEFINITION
// Defined once here.
// ====================================================================
struct User current_user = {}; // Using {} for C++ style zero initialization


// ====================================================================
// I. SYSTEM CALL WRAPPERS (File I/O, Sockets)
// ====================================================================

// --- File I/O Wrappers ---
int sys_open(const char *pathname, int flags) { return open(pathname, flags, 0666); }
ssize_t sys_read(int fd, void *buf, size_t count) { return read(fd, buf, count); }
ssize_t sys_write(int fd, const void *buf, size_t count) { return write(fd, buf, count); }
off_t sys_lseek(int fd, off_t offset, int whence) { return lseek(fd, offset, whence); }
int sys_close(int fd) { return close(fd); }
ssize_t sys_write_string(const char *s) { return sys_write(1, s, strlen(s)); }

// --- Input Wrapper (TEMPORARY - Must be replaced) ---
int get_input(char *buffer, size_t size) {
    char temp_buf[size];
    if (fgets(temp_buf, size, stdin) != NULL) {
        temp_buf[strcspn(temp_buf, "\n")] = 0;
        strncpy(buffer, temp_buf, size);
        return 0; 
    }
    return -1; 
}

// --- Socket Wrappers ---
int sys_socket(int domain, int type, int protocol) { return socket(domain, type, protocol); }
int sys_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) { return bind(sockfd, addr, addrlen); }
int sys_listen(int sockfd, int backlog) { return listen(sockfd, backlog); }
int sys_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) { return accept(sockfd, addr, addrlen); }
int sys_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) { return connect(sockfd, addr, addrlen); }


// ====================================================================
// II. SYNCHRONIZATION: FILE LOCKING WRAPPERS
// ====================================================================

int sys_lock_record(int fd, int record_index, int type) {
    struct flock lock;
    lock.l_type = type; 
    lock.l_whence = SEEK_SET;
    lock.l_start = (record_index - 1) * sizeof(struct Account); 
    lock.l_len = sizeof(struct Account); 
    return fcntl(fd, F_SETLKW, &lock); 
}

int sys_unlock_record(int fd, int record_index) {
    struct flock lock;
    lock.l_type = F_UNLCK; 
    lock.l_whence = SEEK_SET;
    lock.l_start = (record_index - 1) * sizeof(struct Account);
    lock.l_len = sizeof(struct Account);
    return fcntl(fd, F_SETLKW, &lock);
}


// ====================================================================
// III. CORE UTILITIES
// ====================================================================

// Helper function to find the next available ID by counting records
int get_next_id(const char *filename, size_t struct_size) {
    int fd = sys_open(filename, O_RDONLY);
    if (fd == -1) return 1; // Assume ID 1 if file doesn't exist

    int count = 0;
    char buffer[struct_size];
    
    // FIX: Cast sys_read return value to size_t to prevent signed/unsigned warning
    while ((size_t)sys_read(fd, buffer, struct_size) == struct_size) { 
        count++;
    }
    sys_close(fd);
    return count + 1;
}

int authenticate_and_set_user(char *username, char *password, int expected_role) {
    int fd = sys_open("users.dat", O_RDONLY);
    if (fd == -1) { return 0; } 

    struct User user;
    ssize_t bytes_read;
    int auth_success = 0;

    while ((bytes_read = sys_read(fd, &user, sizeof(struct User))) > 0) {
        if (bytes_read != sizeof(struct User)) continue; 
        
        if (strcmp(user.username, username) == 0 && 
            strcmp(user.password, password) == 0 &&
            user.role == expected_role) {
            
            if (expected_role == CUSTOMER) {
                int account_fd = sys_open("accounts.dat", O_RDONLY);
                if (account_fd != -1) {
                    struct Account acc;
                    off_t offset = (user.id - 1) * sizeof(struct Account);
                    sys_lseek(account_fd, offset, SEEK_SET);

                    if (sys_read(account_fd, &acc, sizeof(struct Account)) == sizeof(struct Account)) {
                        if (acc.status == DEACTIVATED) {
                            sys_close(account_fd);
                            sys_close(fd);
                            return 0; 
                        }
                    }
                    sys_close(account_fd);
                }
            }

            current_user = user; 
            auth_success = 1;
            break;
        }
    }

    sys_close(fd);
    return auth_success;
}

void change_password_flow() {
    sys_write_string("This functionality must be requested via the server.\n");
}

void print_menu(int role) {
    sys_write_string("\n============================================\n");
    switch (role) {
        case CUSTOMER:
            sys_write_string("👤 Customer Menu\n");
            sys_write_string("1. View Account Balance\n"); 
            sys_write_string("2. Deposit Money\n"); 
            sys_write_string("3. Withdraw Money\n"); 
            sys_write_string("4. Transfer Funds\n"); 
            sys_write_string("5. Apply for a Loan\n"); 
            sys_write_string("6. Add Feedback\n"); 
            sys_write_string("7. View Transaction History\n"); 
            sys_write_string("8. Change Password\n"); 
            sys_write_string("9. Logout\n"); 
            sys_write_string("10. Exit\n"); 
            break;
        case EMPLOYEE:
            sys_write_string("👨‍💼 Employee Menu\n");
            sys_write_string("1. Add New Customer\n"); 
            sys_write_string("2. Modify Customer Details\n"); 
            sys_write_string("3. Process Loan Applications\n");
            sys_write_string("4. Approve/Reject Loans\n");
            sys_write_string("5. View Assigned Loan Applications\n");
            sys_write_string("6. View Customer Transactions\n");
            sys_write_string("7. Change Password\n");
            sys_write_string("8. Logout\n"); 
            sys_write_string("9. Exit\n"); 
            break;
        case MANAGER:
            sys_write_string("👔 Manager Menu\n");
            sys_write_string("1. Activate/Deactivate Customer Accounts\n"); 
            sys_write_string("2. Assign Loan Application Processes to Employees\n"); 
            sys_write_string("3. Review Customer Feedback\n"); 
            sys_write_string("4. Change Password\n"); 
            sys_write_string("5. Logout\n"); 
            sys_write_string("6. Exit\n");
            break;
        case ADMINISTRATOR:
            sys_write_string("💻 Administrator Menu\n");
            sys_write_string("1. Add New Bank Employee\n"); 
            sys_write_string("2. Modify Customer/Employee Details\n"); 
            sys_write_string("3. Manage User Roles\n"); 
            sys_write_string("4. Change Password\n"); 
            sys_write_string("5. Logout\n"); 
            sys_write_string("6. Exit\n"); 
            break;
        default:
            sys_write_string("Unknown Role.\n");
    }
    sys_write_string("============================================\n");
    sys_write_string("Enter your choice: ");
}


// ====================================================================
// IV. SERVER-SIDE SERVICE FUNCTIONS
// ====================================================================

// --- 1. View Balance (Read Lock) ---
void serve_view_balance(int client_sd, struct Message *request) {
    struct Message response;
    response.command = CMD_VIEW_BALANCE;
    response.success_status = 0; 
    int acc_id = request->source_id;
    int fd = sys_open("accounts.dat", O_RDONLY);
    
    if (fd != -1) {
        if (sys_lock_record(fd, acc_id, F_RDLCK) == 0) {
            struct Account acc;
            off_t offset = (acc_id - 1) * sizeof(struct Account);
            sys_lseek(fd, offset, SEEK_SET);
            if (sys_read(fd, &acc, sizeof(struct Account)) == sizeof(struct Account)) {
                response.account_data = acc;
                response.success_status = 1;
            }
            sys_unlock_record(fd, acc_id);
        }
        sys_close(fd);
    }
    sys_write(client_sd, &response, sizeof(struct Message));
}

// --- 2. Deposit Money (Write Lock) ---
void serve_deposit(int client_sd, struct Message *request) {
    struct Message response;
    response.command = CMD_DEPOSIT;
    response.success_status = 0;
    int acc_id = request->source_id;
    double amount = request->amount;
    int fd = sys_open("accounts.dat", O_RDWR);
    
    if (fd != -1) {
        if (sys_lock_record(fd, acc_id, F_WRLCK) == 0) {
            struct Account acc;
            off_t offset = (acc_id - 1) * sizeof(struct Account);
            sys_lseek(fd, offset, SEEK_SET);
            if (sys_read(fd, &acc, sizeof(struct Account)) == sizeof(struct Account)) {
                acc.balance += amount; 
                response.account_data = acc; 
                response.success_status = 1;
                sys_lseek(fd, offset, SEEK_SET);
                sys_write(fd, &acc, sizeof(struct Account));
            }
            sys_unlock_record(fd, acc_id);
        }
        sys_close(fd);
    }
    sys_write(client_sd, &response, sizeof(struct Message));
}

// --- 3. Withdraw Money (Write Lock) ---
void serve_withdraw(int client_sd, struct Message *request) {
    struct Message response;
    response.command = CMD_WITHDRAW;
    response.success_status = 0; 
    strcpy(response.data, "Withdrawal failed.");
    int acc_id = request->source_id;
    double amount = request->amount;
    int fd = sys_open("accounts.dat", O_RDWR);
    
    if (fd != -1) {
        if (sys_lock_record(fd, acc_id, F_WRLCK) == 0) {
            struct Account acc;
            off_t offset = (acc_id - 1) * sizeof(struct Account);
            sys_lseek(fd, offset, SEEK_SET);
            if (sys_read(fd, &acc, sizeof(struct Account)) == sizeof(struct Account)) {
                if (acc.balance >= amount) {
                    acc.balance -= amount;
                    response.account_data = acc; 
                    response.success_status = 1;
                    strcpy(response.data, "Withdrawal successful.");
                    sys_lseek(fd, offset, SEEK_SET);
                    sys_write(fd, &acc, sizeof(struct Account));
                } else {
                    strcpy(response.data, "Insufficient funds.");
                }
            }
            sys_unlock_record(fd, acc_id);
        }
        sys_close(fd);
    }
    sys_write(client_sd, &response, sizeof(struct Message));
}

// --- 4. Transfer Funds (Dual Write Lock) ---
void serve_transfer(int client_sd, struct Message *request) {
    struct Message response;
    response.command = CMD_TRANSFER;
    response.success_status = 0; 
    strcpy(response.data, "Transfer failed.");

    int source_id = request->source_id;
    int target_id = request->target_id;
    double amount = request->amount;

    if (source_id == target_id) {
        strcpy(response.data, "Cannot transfer to the same account.");
        sys_write(client_sd, &response, sizeof(struct Message));
        return;
    }

    int fd = sys_open("accounts.dat", O_RDWR);
    if (fd == -1) {
        sys_write(client_sd, &response, sizeof(struct Message));
        return;
    }

    // --- Critical Section: Dual Locking ---
    // Acquire locks in a consistent order (lower ID first) to prevent Deadlock.
    int id1 = (source_id < target_id) ? source_id : target_id;
    int id2 = (source_id < target_id) ? target_id : source_id;

    if (sys_lock_record(fd, id1, F_WRLCK) == 0) { // Lock first record
        if (sys_lock_record(fd, id2, F_WRLCK) == 0) { // Lock second record

            struct Account source_acc, target_acc;
            off_t offset_source = (source_id - 1) * sizeof(struct Account);
            off_t offset_target = (target_id - 1) * sizeof(struct Account);

            // 1. Read Source Account
            sys_lseek(fd, offset_source, SEEK_SET);
            if (sys_read(fd, &source_acc, sizeof(struct Account)) != sizeof(struct Account)) goto unlock_and_fail;

            // 2. Read Target Account
            sys_lseek(fd, offset_target, SEEK_SET);
            if (sys_read(fd, &target_acc, sizeof(struct Account)) != sizeof(struct Account)) goto unlock_and_fail;

            // 3. Consistency Check (Sufficient Funds)
            if (source_acc.balance >= amount) {
                // 4. Modify
                source_acc.balance -= amount;
                target_acc.balance += amount;

                // 5. Write Source Account
                sys_lseek(fd, offset_source, SEEK_SET);
                sys_write(fd, &source_acc, sizeof(struct Account));

                // 6. Write Target Account
                sys_lseek(fd, offset_target, SEEK_SET);
                sys_write(fd, &target_acc, sizeof(struct Account));
                
                // Success
                response.success_status = 1;
                response.account_data = source_acc; // Send back new source balance
                strcpy(response.data, "Transfer successful.");
            } else {
                strcpy(response.data, "Insufficient funds in source account.");
            }

            unlock_and_fail:; // Label for cleanup
            sys_unlock_record(fd, id2); // Unlock second record
        }
        sys_unlock_record(fd, id1); // Unlock first record
    }
    
    sys_close(fd);
    sys_write(client_sd, &response, sizeof(struct Message));
}

// --- 5. Add New Customer (Employee Function) ---
void serve_add_customer(int client_sd, struct Message *request) {
    struct Message response;
    response.command = CMD_ADD_CUSTOMER;
    response.success_status = 0; 
    strcpy(response.data, "Customer creation failed.");

    // Parse data: Username is stored in data[0], Password in data[MAX_NAME_LEN]
    char *username = request->data;
    char *password = request->data + MAX_NAME_LEN;
    
    // 1. Get New ID
    int new_id = get_next_id("users.dat", sizeof(struct User));

    // 2. Create User Record (using {} for safe C++ zero-initialization)
    struct User new_customer = {};
    new_customer.id = new_id;
    new_customer.role = CUSTOMER;
    strcpy(new_customer.username, username);
    strcpy(new_customer.password, password);
    // Placeholder details (these would normally come from the client request)
    strcpy(new_customer.name, "New Client");
    new_customer.age = 30;
    strcpy(new_customer.address, "Pending Address");
    
    // 3. Create Account Record
    struct Account new_account = {};
    new_account.id = new_id;
    new_account.balance = 0.00;
    new_account.status = ACTIVE;

    // Use O_CREAT to ensure the file is created if it doesn't exist.
    int fd_u = sys_open("users.dat", O_WRONLY | O_APPEND | O_CREAT);
    int fd_a = sys_open("accounts.dat", O_WRONLY | O_APPEND | O_CREAT);

    if (fd_u != -1 && fd_a != -1) {
        if (sys_write(fd_u, &new_customer, sizeof(struct User)) == sizeof(struct User) &&
            sys_write(fd_a, &new_account, sizeof(struct Account)) == sizeof(struct Account)) {
            
            response.success_status = 1;
            sprintf(response.data, "Customer ID %d created successfully!", new_id);
        } else {
             strcpy(response.data, "Error writing data to files.");
        }
    } else {
        strcpy(response.data, "File access error during creation.");
    }

    if (fd_u != -1) sys_close(fd_u);
    if (fd_a != -1) sys_close(fd_a);
    
    sys_write(client_sd, &response, sizeof(struct Message));
}


// --- 6. Modify Customer Details (Employee Function) ---
void serve_modify_customer(int client_sd, struct Message *request) {
    struct Message response;
    response.command = CMD_MODIFY_CUSTOMER;
    response.success_status = 0;
    strcpy(response.data, "Modification failed.");
    
    int target_id = request->target_id;
    
    // Data layout: [Name\0][Age\0][Address\0]
    char *new_name = request->data;
    char *new_age_str = request->data + MAX_NAME_LEN;
    char *new_address = request->data + MAX_NAME_LEN + 10; 

    int fd_u = sys_open("users.dat", O_RDWR);
    if (fd_u == -1) {
        strcpy(response.data, "Database access error.");
        sys_write(client_sd, &response, sizeof(struct Message));
        return;
    }

    off_t offset = (target_id - 1) * sizeof(struct User);

    // Acquire Write Lock on the specific user record
    // NOTE: Locking on the User record is necessary since we modify the User file.
    if (sys_lock_record(fd_u, target_id, F_WRLCK) == 0) { 

        struct User user_record;
        
        // 1. Read existing record
        sys_lseek(fd_u, offset, SEEK_SET);
        if (sys_read(fd_u, &user_record, sizeof(struct User)) == sizeof(struct User)) {

            // 2. Check if the user is a CUSTOMER 
            if (user_record.role == CUSTOMER) {
                
                // 3. Modify fields
                strcpy(user_record.name, new_name);
                user_record.age = atoi(new_age_str); 
                strcpy(user_record.address, new_address);

                // 4. Write modified record back
                sys_lseek(fd_u, offset, SEEK_SET);
                if (sys_write(fd_u, &user_record, sizeof(struct User)) == sizeof(struct User)) {
                    response.success_status = 1;
                    sprintf(response.data, "Details for Customer ID %d updated.", target_id);
                }
            } else {
                strcpy(response.data, "Target ID is not a Customer.");
            }
        } else {
             strcpy(response.data, "Customer ID not found or file error.");
        }
        
        // 5. Release Lock
        sys_unlock_record(fd_u, target_id);

    } else {
        strcpy(response.data, "Failed to acquire exclusive lock.");
    }
    
    sys_close(fd_u);
    sys_write(client_sd, &response, sizeof(struct Message));
}