// data_initializer.c

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "structs.h"

#define USERS_FILE "users.dat"
#define ACCOUNTS_FILE "accounts.dat"

int main() {
    // 1. Setup Admin (ID 1)
    struct User admin = {1, ADMINISTRATOR, "admin", "adminpass", "Admin User", 40, "HQ"};

    // 2. Setup Employee (ID 2)
    struct User employee = {2, EMPLOYEE, "emp1", "emppass", "Bank Employee 1", 30, "Branch A"};

    // 3. Setup Customer A (ID 3)
    struct User custA = {3, CUSTOMER, "custA", "custApass", "Customer A", 25, "Address A"};
    struct Account accA = {3, 1000.00, ACTIVE}; // Balance $1000

    // 4. Setup Customer B (ID 4)
    struct User custB = {4, CUSTOMER, "custB", "custBpass", "Customer B", 50, "Address B"};
    struct Account accB = {4, 500.00, ACTIVE}; // Balance $500

    // --- Write Users to users.dat ---
    int fd_u = open(USERS_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_u == -1) { perror("open users.dat"); return 1; }
    
    write(fd_u, &admin, sizeof(struct User));
    write(fd_u, &employee, sizeof(struct User));
    write(fd_u, &custA, sizeof(struct User));
    write(fd_u, &custB, sizeof(struct User));
    close(fd_u);

    // --- Write Accounts to accounts.dat ---
    int fd_a = open(ACCOUNTS_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_a == -1) { perror("open accounts.dat"); return 1; }

    // Account files must contain records starting from ID 1, even if they are inactive placeholders
    // We assume ID 1 and 2 are staff accounts without a balance entry in this file
    // To maintain array-like indexing based on ID, we must pad (or rely on the logic handling missing IDs)
    // For simplicity, we assume linear records where ID = Index + 1:
    struct Account padding1 = {1, 0.0, DEACTIVATED}; // Admin
    struct Account padding2 = {2, 0.0, DEACTIVATED}; // Employee

    write(fd_a, &padding1, sizeof(struct Account));
    write(fd_a, &padding2, sizeof(struct Account));
    write(fd_a, &accA, sizeof(struct Account));
    write(fd_a, &accB, sizeof(struct Account));
    close(fd_a);

    printf("Successfully created %s and %s for testing.\n", USERS_FILE, ACCOUNTS_FILE);
    return 0;
}