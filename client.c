// client.c

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h> // For exit, atoi, atof
#include <stdio.h>  // For sprintf (TEMPORARY - MUST BE REPLACED)
#include <string.h> // For strncpy

#include "utils.h"
#include "structs.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

// Global variable for the connected socket descriptor
int server_sd = -1;

// Function declarations
int connect_to_server();
void client_login_flow(int role);
void customer_menu_handler();
// ... other menu handlers

// CRITICAL FIX: The definition of current_user is in utils.c.
// We rely on the extern declaration in structs.h to access it.

int connect_to_server() {
    struct sockaddr_in server_addr;

    server_sd = sys_socket(AF_INET, SOCK_STREAM, 0);
    if (server_sd < 0) {
        perror("[CLIENT] Socket creation failed");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        sys_write_string("[CLIENT] Invalid address/ Address not supported \n");
        sys_close(server_sd);
        return -1;
    }

    if (sys_connect(server_sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("[CLIENT] Connection Failed");
        sys_close(server_sd);
        return -1;
    }
    sys_write_string("[CLIENT] Connected to the Bank Server.\n");
    return 0;
}

void client_login_flow(int role) {
    char username[MAX_NAME_LEN];
    char password[MAX_PASS_LEN];
    struct Message request, response;

    sys_write_string("--- Login ---\n");
    sys_write_string("Username: ");
    get_input(username, MAX_NAME_LEN);
    sys_write_string("Password: ");
    get_input(password, MAX_PASS_LEN);

    request.command = CMD_LOGIN;
    request.source_id = role; 
    strncpy(request.data, username, MAX_NAME_LEN);
    strncpy(request.data + MAX_NAME_LEN, password, MAX_PASS_LEN);
    
    sys_write(server_sd, &request, sizeof(struct Message));
    sys_read(server_sd, &response, sizeof(struct Message));

    if (response.success_status) {
        sys_write_string("✅ Login Successful!\n");
        current_user.id = response.source_id; 
        current_user.role = role;
    } else {
        sys_write_string("❌ Login Failed: Invalid credentials or deactivated account.\n");
    }
}

void employee_menu_handler() {
    char choice_str[10];
    int choice;
    struct Message request, response;

    while (current_user.id != 0) {
        print_menu(EMPLOYEE);
        get_input(choice_str, sizeof(choice_str));
        choice = atoi(choice_str);

        switch (choice) {
            case 1: // Add New Customer
                {
                    char username[MAX_NAME_LEN], password[MAX_PASS_LEN];
                    
                    sys_write_string("--- Add New Customer ---\n");
                    sys_write_string("Enter Username: ");
                    get_input(username, MAX_NAME_LEN);
                    sys_write_string("Enter Password: ");
                    get_input(password, MAX_PASS_LEN);
                    
                    // NOTE: We only send username and password for now.
                    // The server uses placeholder data for name, age, address.
                    request.command = CMD_ADD_CUSTOMER;
                    request.source_id = current_user.id; // Employee ID
                    strncpy(request.data, username, MAX_NAME_LEN);
                    strncpy(request.data + MAX_NAME_LEN, password, MAX_PASS_LEN);
                    
                    sys_write(server_sd, &request, sizeof(struct Message));
                    sys_read(server_sd, &response, sizeof(struct Message));
                    
                    if (response.success_status) {
                        sys_write_string("✅ ");
                        sys_write_string(response.data);
                        sys_write_string("\n");
                    } else {
                        sys_write_string("❌ Customer creation failed: ");
                        sys_write_string(response.data);
                        sys_write_string("\n");
                    }
                }
                break;
            case 2: // Modify Customer Details
                {
                    char target_id_str[10];
                    char new_name[MAX_NAME_LEN], new_age_str[10], new_address[100];
                    int target_id;
                    
                    sys_write_string("--- Modify Customer Details ---\n");
                    sys_write_string("Enter Customer ID to modify: ");
                    get_input(target_id_str, sizeof(target_id_str));
                    target_id = atoi(target_id_str);
                    
                    sys_write_string("Enter NEW Name: ");
                    get_input(new_name, MAX_NAME_LEN);
                    sys_write_string("Enter NEW Age: ");
                    get_input(new_age_str, 10);
                    sys_write_string("Enter NEW Address: ");
                    get_input(new_address, 100);

                    // Package data: [Name\0][Age_str\0][Address\0]
                    request.command = CMD_MODIFY_CUSTOMER;
                    request.target_id = target_id;
                    
                    // Copy Name
                    strncpy(request.data, new_name, MAX_NAME_LEN);
                    // Copy Age String (starts at offset MAX_NAME_LEN)
                    strncpy(request.data + MAX_NAME_LEN, new_age_str, 10);
                    // Copy Address (starts at offset MAX_NAME_LEN + 10)
                    strncpy(request.data + MAX_NAME_LEN + 10, new_address, 100);
                    
                    sys_write(server_sd, &request, sizeof(struct Message));
                    sys_read(server_sd, &response, sizeof(struct Message));
                    
                    if (response.success_status) {
                        sys_write_string("✅ ");
                        sys_write_string(response.data);
                        sys_write_string("\n");
                    } else {
                        sys_write_string("❌ Modification failed: ");
                        sys_write_string(response.data);
                        sys_write_string("\n");
                    }
                }
                break;

            case 8: // Logout
                request.command = CMD_LOGOUT;
                sys_write(server_sd, &request, sizeof(struct Message));
                sys_read(server_sd, &response, sizeof(struct Message));
                
                if (response.success_status) {
                    sys_write_string("Logging out...\n");
                    current_user.id = 0; 
                } else {
                    sys_write_string("❌ Logout failed on server.\n");
                }
                break;
                
            case 9: // Exit
                sys_write_string("Exiting system. Goodbye!\n");
                sys_close(server_sd);
                exit(0);

            default:
                sys_write_string("Option is not yet implemented.\n");
        }
    }
}


void customer_menu_handler() {
    char choice_str[10];
    int choice;
    struct Message request, response;

    while (current_user.id != 0) {
        print_menu(CUSTOMER);
        get_input(choice_str, sizeof(choice_str));
        choice = atoi(choice_str);

        switch (choice) {
            case 1: // View Balance
                request.command = CMD_VIEW_BALANCE;
                request.source_id = current_user.id;
                sys_write(server_sd, &request, sizeof(struct Message));
                sys_read(server_sd, &response, sizeof(struct Message));
                
                if (response.success_status) {
                    char balance_output[100];
                    sprintf(balance_output, "✅ Current Balance: %.2f\n", response.account_data.balance);
                    sys_write_string(balance_output);
                } else {
                    sys_write_string("❌ Failed to view balance.\n");
                }
                break;
            
            case 2: // Deposit Money
            case 3: // Withdraw Money
                { 
                    char amount_str[20];
                    double amount;
                    
                    sys_write_string(choice == 2 ? "Enter deposit amount: " : "Enter withdrawal amount: ");
                    get_input(amount_str, sizeof(amount_str));
                    amount = atof(amount_str);
                    
                    request.command = (choice == 2) ? CMD_DEPOSIT : CMD_WITHDRAW;
                    request.source_id = current_user.id;
                    request.amount = amount;
                    
                    sys_write(server_sd, &request, sizeof(struct Message));
                    sys_read(server_sd, &response, sizeof(struct Message));
                    
                    if (response.success_status) {
                        char output[150];
                        sprintf(output, "✅ %s successful! New Balance: %.2f\n", 
                                (choice == 2) ? "Deposit" : "Withdrawal", 
                                response.account_data.balance);
                        sys_write_string(output);
                    } else {
                        sys_write_string("❌ Transaction Failed: ");
                        sys_write_string(response.data);
                        sys_write_string("\n");
                    }
                }
                break;
            
            case 4: // Transfer Funds
                { 
                    char amount_str[20], target_id_str[10];
                    double amount;
                    int target_id;
                    
                    sys_write_string("Enter target Account ID: ");
                    get_input(target_id_str, sizeof(target_id_str));
                    target_id = atoi(target_id_str);

                    sys_write_string("Enter transfer amount: ");
                    get_input(amount_str, sizeof(amount_str));
                    amount = atof(amount_str);
                    
                    request.command = CMD_TRANSFER;
                    request.source_id = current_user.id;
                    request.target_id = target_id; 
                    request.amount = amount;
                    
                    sys_write(server_sd, &request, sizeof(struct Message));
                    sys_read(server_sd, &response, sizeof(struct Message));
                    
                    if (response.success_status) {
                        char output[200];
                        sprintf(output, "✅ Transfer to Account %d successful! New Balance: %.2f\n", 
                                target_id, response.account_data.balance);
                        sys_write_string(output);
                    } else {
                        sys_write_string("❌ Transfer Failed: ");
                        sys_write_string(response.data);
                        sys_write_string("\n");
                    }
                }
                break;
            
            case 9: // Logout
                request.command = CMD_LOGOUT;
                sys_write(server_sd, &request, sizeof(struct Message));
                sys_read(server_sd, &response, sizeof(struct Message));
                
                if (response.success_status) {
                    sys_write_string("Logging out...\n");
                    current_user.id = 0; 
                } else {
                    sys_write_string("❌ Logout failed on server.\n");
                }
                break;
                
            case 10: // Exit
                sys_write_string("Exiting system. Goodbye!\n");
                sys_close(server_sd);
                exit(0);
            
            default:
                sys_write_string("Option is not yet implemented.\n");
        }
    }
}

int main() {
    char choice_str[10];
    int choice = 0;

    if (connect_to_server() != 0) {
        sys_write_string("Could not start client. Exiting.\n");
        return 1;
    }

    while (1) {
        if (current_user.id == 0) {
            sys_write_string("\n=== Banking Management System ===\n");
            sys_write_string("Select your role:\n");
            sys_write_string("1. Customer\n");
            sys_write_string("2. Bank Employee\n");
            sys_write_string("3. Manager\n");
            sys_write_string("4. Administrator\n");
            sys_write_string("5. Exit System\n");
            sys_write_string("Enter choice (1-5): ");
            
            get_input(choice_str, sizeof(choice_str));
            choice = atoi(choice_str);

            if (choice >= CUSTOMER && choice <= ADMINISTRATOR) {
                client_login_flow(choice);
                
                if (current_user.id != 0) {
                    switch (current_user.role) {
                        case CUSTOMER:
                            customer_menu_handler();
                            break;
                        case EMPLOYEE: // <-- CHANGE THIS CASE
                            employee_menu_handler(); // <-- Ensure this is called
                            break;
                        case MANAGER:
                        case ADMINISTRATOR:
                        default:
                            sys_write_string("[CLIENT] Role menu not yet implemented.\n");
                            current_user.id = 0; 
                            break;
                    }
                }
            } else if (choice == 5) {
                sys_write_string("Goodbye! Exiting system.\n");
                sys_close(server_sd);
                exit(0);
            } else {
                sys_write_string("Invalid choice. Please try again.\n");
            }
        }
    }
    return 0;
}