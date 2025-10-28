// structs.h

#ifndef STRUCTS_H
#define STRUCTS_H

// Role definitions
#define CUSTOMER 1
#define EMPLOYEE 2
#define MANAGER 3
#define ADMINISTRATOR 4

// User Status
#define ACTIVE 1
#define DEACTIVATED 0

// Maximum lengths
#define MAX_NAME_LEN 50
#define MAX_PASS_LEN 30

// Structure for User (Login Credentials and Personal Info)
struct User {
    int id; // Unique User ID (serves as Account ID for customers)
    int role; // 1: Customer, 2: Employee, 3: Manager, 4: Admin
    char username[MAX_NAME_LEN];
    char password[MAX_PASS_LEN];
    char name[MAX_NAME_LEN];
    int age;
    char address[100];
};

// Structure for Bank Account (Only for Customers)
struct Account {
    int id; // Same as User ID
    double balance;
    int status; // 1: Active, 0: Deactivated
};

// Structure for Transactions
struct Transaction {
    int id; // Unique Transaction ID
    int account_id;
    int type; // 1: Deposit, 2: Withdraw, 3: Transfer Out, 4: Transfer In
    double amount;
    int target_account_id; // Used for transfers
    // In a real system, we'd add a timestamp, but we'll keep it simple for now.
};

// Inter-Process Communication Message Structure
struct Message {
    int command;
    int source_id;
    int target_id;
    double amount;
    char data[256]; // Generic field for username, password, feedback text, etc.
    struct Account account_data; // For sending account info back
    int success_status; // 1 for success, 0 for failure
};

// Command definitions
#define CMD_LOGIN 1
#define CMD_VIEW_BALANCE 2
#define CMD_DEPOSIT 3
#define CMD_WITHDRAW 4
#define CMD_TRANSFER 5 
#define CMD_ADD_CUSTOMER 6 // <-- NEW
#define CMD_MODIFY_CUSTOMER 7
#define CMD_LOGOUT 99

// Global variables for the current session (Declared here, Defined in utils.c)
extern struct User current_user;

#endif