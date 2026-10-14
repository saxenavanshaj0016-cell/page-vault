#include "database.hpp"
#include "memory.hpp"

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// APPLICATION MODULE
// This file connects the DBMS module (RecordStore) and the OS module
// (MemoryManager) through a small console menu.

int readInteger(const std::string& prompt) {
    while (true) {
        std::cout << prompt;
        std::string input;
        std::getline(std::cin, input);
        try {
            std::size_t processed{};
            const int value = std::stoi(input, &processed);
            if (processed == input.size()) {
                return value;
            }
        } catch (const std::exception&) {
        }
        std::cout << "Please enter a valid whole number.\n";
    }
}

std::string readText(const std::string& prompt) {
    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    return input;
}

void showRecords(const RecordStore& store) {
    const auto records = store.all();
    std::cout << "\nDATABASE: data/page_vault.db (SQLite)\n";
    std::cout << "ID   Name                  Department              Score\n";
    std::cout << "----------------------------------------------------------\n";
    for (const auto& record : records) {
        std::cout << std::left << std::setw(5) << record.id << std::setw(22) << record.name
                  << std::setw(24) << record.department << record.score << '\n';
    }
}

void showPageRecords(const std::vector<Record>& records) {
    std::cout << "Records in loaded page:\n";
    for (const auto& record : records) {
        std::cout << "  [" << record.id << "] " << record.name << " | " << record.department
                  << " | score " << record.score << '\n';
    }
}

int main() {
    try {
        // DBMS setup: records remain in the SQLite database even after RAM reset.
        RecordStore store("data/page_vault.db");

        // OS setup: only two simulated RAM frames; one page contains 3 records.
        constexpr int pageSize = 3;
        MemoryManager memory(2, Policy::FIFO);
        Policy currentPolicy = Policy::FIFO;

        std::cout << "==============================================\n";
        std::cout << " PAGE VAULT - C++ Virtual Memory DB Prototype\n";
        std::cout << "==============================================\n";
        std::cout << "Implemented: CRUD, disk pages, RAM frames, FIFO/LRU.\n";
        std::cout << "Not yet implemented: scheduling, locks, GUI, I/O charts.\n";

        while (true) {
            std::cout << "\n1. Show database\n2. Access a record (simulate page access)\n"
                      << "3. Add record\n4. Update record\n5. Delete record\n"
                      << "6. Switch FIFO/LRU\n7. Show RAM and page table\n8. Reset RAM\n0. Exit\n";
            const int choice = readInteger("Choose: ");

            if (choice == 0) {
                std::cout << "Goodbye.\n";
                break;
            }
            if (choice == 1) {
                showRecords(store);
            } else if (choice == 2) {
                const int recordId = readInteger("Record ID: ");
                const int pageId = store.pageForRecord(recordId, pageSize);
                if (pageId < 0) {
                    std::cout << "Record not found.\n";
                    continue;
                }

                // DBMS returns a logical page; OS MemoryManager decides hit/fault.
                const AccessResult result = memory.accessPage(
                    pageId, [&store, pageSize](int requestedPage) {
                        return store.loadPage(requestedPage, pageSize);
                    });
                if (result.hit) {
                    std::cout << "PAGE HIT: logical page " << pageId << " is already in frame "
                              << result.frameId << ".\n";
                } else if (result.evictedPage < 0) {
                    std::cout << "PAGE FAULT: loaded page " << pageId << " into empty frame "
                              << result.frameId << ".\n";
                } else {
                    std::cout << "PAGE FAULT: replaced page " << result.evictedPage << " with page "
                              << pageId << " in frame " << result.frameId << ".\n";
                }
                showPageRecords(result.records);
            } else if (choice == 3) {
                const std::string name = readText("Name: ");
                const std::string department = readText("Department: ");
                const int score = readInteger("Score (0-100): ");
                const int newId = store.add(name, department, score);
                std::cout << "Record inserted with ID " << newId << ".\n";
            } else if (choice == 4) {
                const int id = readInteger("Record ID: ");
                const std::string name = readText("New name: ");
                const std::string department = readText("New department: ");
                const int score = readInteger("New score: ");
                std::cout << (store.update(id, name, department, score) ? "Record updated.\n" : "Record not found.\n");
            } else if (choice == 5) {
                const int id = readInteger("Record ID: ");
                std::cout << (store.remove(id) ? "Record deleted.\n" : "Record not found.\n");
            } else if (choice == 6) {
                currentPolicy = currentPolicy == Policy::FIFO ? Policy::LRU : Policy::FIFO;
                memory.setPolicy(currentPolicy);
                std::cout << "Replacement policy is now "
                          << (currentPolicy == Policy::FIFO ? "FIFO" : "LRU") << ".\n";
            } else if (choice == 7) {
                memory.showState();
            } else if (choice == 8) {
                memory.reset();
                std::cout << "RAM frames and page table reset. Disk records were kept.\n";
            } else {
                std::cout << "Choose an option from 0 to 8.\n";
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}

