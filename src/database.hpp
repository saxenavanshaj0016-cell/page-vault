#pragma once

#include <string>
#include <vector>

// DBMS MODULE
// Represents the persistent record table stored in data/records.csv.
struct Record {
    int id{};
    std::string name;
    std::string department;
    int score{};
};

class RecordStore {
public:
    explicit RecordStore(std::string filePath);

    std::vector<Record> all() const;
    int add(const std::string& name, const std::string& department, int score);
    bool update(int id, const std::string& name, const std::string& department, int score);
    bool remove(int id);

    // DBMS-to-OS bridge: records are grouped into fixed-size logical pages.
    std::vector<Record> loadPage(int pageId, int pageSize) const;
    int pageForRecord(int recordId, int pageSize) const;

private:
    std::string filePath_;

    void ensureDatabaseFile() const;
    static void validate(const std::string& name, const std::string& department, int score);
    void save(const std::vector<Record>& records) const;
};

