#include "database.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <windows.h>

// Windows ships with winsqlite3.dll. Loading it at runtime keeps this project
// runnable with the installed MinGW compiler, without a separate SQLite setup.
struct sqlite3;
struct sqlite3_stmt;

namespace {
constexpr int SQLITE_OK = 0;
constexpr int SQLITE_ROW = 100;
constexpr int SQLITE_DONE = 101;
const auto SQLITE_TRANSIENT = reinterpret_cast<void (*)(void*)>(-1);

class SQLiteApi {
public:
    using Open = int (*)(const char*, sqlite3**);
    using Close = int (*)(sqlite3*);
    using Exec = int (*)(sqlite3*, const char*, int (*)(void*, int, char**, char**), void*, char**);
    using Prepare = int (*)(sqlite3*, const char*, int, sqlite3_stmt**, const char**);
    using Finalize = int (*)(sqlite3_stmt*);
    using Step = int (*)(sqlite3_stmt*);
    using BindInt = int (*)(sqlite3_stmt*, int, int);
    using BindText = int (*)(sqlite3_stmt*, int, const char*, int, void (*)(void*));
    using ColumnInt = int (*)(sqlite3_stmt*, int);
    using ColumnText = const unsigned char* (*)(sqlite3_stmt*, int);
    using Changes = int (*)(sqlite3*);
    using LastInsertRowId = long long (*)(sqlite3*);
    using ErrorMessage = const char* (*)(sqlite3*);
    using Free = void (*)(void*);

    Open open{};
    Close close{};
    Exec exec{};
    Prepare prepare{};
    Finalize finalize{};
    Step step{};
    BindInt bindInt{};
    BindText bindText{};
    ColumnInt columnInt{};
    ColumnText columnText{};
    Changes changes{};
    LastInsertRowId lastInsertRowId{};
    ErrorMessage errorMessage{};
    Free freeMemory{};

    static SQLiteApi& instance() {
        static SQLiteApi api;
        return api;
    }

private:
    HMODULE module_{};

    SQLiteApi() {
        module_ = LoadLibraryA("winsqlite3.dll");
        if (!module_) {
            throw std::runtime_error("Windows SQLite library (winsqlite3.dll) was not found.");
        }
        open = load<Open>("sqlite3_open");
        close = load<Close>("sqlite3_close");
        exec = load<Exec>("sqlite3_exec");
        prepare = load<Prepare>("sqlite3_prepare_v2");
        finalize = load<Finalize>("sqlite3_finalize");
        step = load<Step>("sqlite3_step");
        bindInt = load<BindInt>("sqlite3_bind_int");
        bindText = load<BindText>("sqlite3_bind_text");
        columnInt = load<ColumnInt>("sqlite3_column_int");
        columnText = load<ColumnText>("sqlite3_column_text");
        changes = load<Changes>("sqlite3_changes");
        lastInsertRowId = load<LastInsertRowId>("sqlite3_last_insert_rowid");
        errorMessage = load<ErrorMessage>("sqlite3_errmsg");
        freeMemory = load<Free>("sqlite3_free");
    }

    template <typename Function>
    Function load(const char* name) {
        const auto function = reinterpret_cast<Function>(GetProcAddress(module_, name));
        if (!function) {
            throw std::runtime_error(std::string("SQLite function missing: ") + name);
        }
        return function;
    }
};

void check(int code, sqlite3* database, const std::string& operation) {
    if (code == SQLITE_OK) {
        return;
    }
    const char* details = database ? SQLiteApi::instance().errorMessage(database) : "unknown error";
    throw std::runtime_error(operation + ": " + (details ? details : "unknown error"));
}

void checkDone(int code, sqlite3* database, const std::string& operation) {
    if (code == SQLITE_DONE) {
        return;
    }
    const char* details = database ? SQLiteApi::instance().errorMessage(database) : "unknown error";
    throw std::runtime_error(operation + ": " + (details ? details : "unknown error"));
}

class DatabaseConnection {
public:
    explicit DatabaseConnection(const std::string& path) : api_(SQLiteApi::instance()) {
        const int result = api_.open(path.c_str(), &database_);
        if (result != SQLITE_OK) {
            const char* details = database_ ? api_.errorMessage(database_) : "cannot open database";
            if (database_) {
                api_.close(database_);
            }
            database_ = nullptr;
            throw std::runtime_error(std::string("SQLite open failed: ") + details);
        }
    }

    ~DatabaseConnection() {
        if (database_) {
            api_.close(database_);
        }
    }

    DatabaseConnection(const DatabaseConnection&) = delete;
    DatabaseConnection& operator=(const DatabaseConnection&) = delete;

    sqlite3* handle() const { return database_; }

    void run(const char* sql) const {
        char* error = nullptr;
        const int result = api_.exec(database_, sql, nullptr, nullptr, &error);
        if (result != SQLITE_OK) {
            std::string details = error ? error : "unknown error";
            if (error) {
                api_.freeMemory(error);
            }
            throw std::runtime_error("SQLite statement failed: " + details);
        }
    }

private:
    SQLiteApi& api_;
    sqlite3* database_{};
};

class Statement {
public:
    Statement(sqlite3* database, const char* sql) : database_(database), api_(SQLiteApi::instance()) {
        check(api_.prepare(database_, sql, -1, &statement_, nullptr), database_, "Preparing SQL");
    }

    ~Statement() {
        if (statement_) {
            api_.finalize(statement_);
        }
    }

    void bindInt(int index, int value) const {
        check(api_.bindInt(statement_, index, value), database_, "Binding integer");
    }

    void bindText(int index, const std::string& value) const {
        check(api_.bindText(statement_, index, value.c_str(), -1, SQLITE_TRANSIENT), database_, "Binding text");
    }

    int step() const { return api_.step(statement_); }
    int integer(int column) const { return api_.columnInt(statement_, column); }
    std::string text(int column) const {
        const auto value = api_.columnText(statement_, column);
        return value ? reinterpret_cast<const char*>(value) : "";
    }

private:
    sqlite3* database_{};
    SQLiteApi& api_;
    sqlite3_stmt* statement_{};
};
}  // namespace

RecordStore::RecordStore(std::string filePath) : filePath_(std::move(filePath)) {
    ensureDatabaseFile();
}

std::vector<Record> RecordStore::all() const {
    DatabaseConnection database(filePath_);
    Statement query(database.handle(), "SELECT id, name, department, score FROM records ORDER BY id;");
    std::vector<Record> records;
    while (query.step() == SQLITE_ROW) {
        records.push_back({query.integer(0), query.text(1), query.text(2), query.integer(3)});
    }
    return records;
}

int RecordStore::add(const std::string& name, const std::string& department, int score) {
    validate(name, department, score);
    DatabaseConnection database(filePath_);
    Statement command(database.handle(),
                      "INSERT INTO records (name, department, score) VALUES (?, ?, ?);");
    command.bindText(1, name);
    command.bindText(2, department);
    command.bindInt(3, score);
    checkDone(command.step(), database.handle(), "Inserting record");
    return static_cast<int>(SQLiteApi::instance().lastInsertRowId(database.handle()));
}

bool RecordStore::update(int id, const std::string& name, const std::string& department, int score) {
    validate(name, department, score);
    DatabaseConnection database(filePath_);
    Statement command(database.handle(),
                      "UPDATE records SET name = ?, department = ?, score = ? WHERE id = ?;");
    command.bindText(1, name);
    command.bindText(2, department);
    command.bindInt(3, score);
    command.bindInt(4, id);
    checkDone(command.step(), database.handle(), "Updating record");
    return SQLiteApi::instance().changes(database.handle()) == 1;
}

bool RecordStore::remove(int id) {
    DatabaseConnection database(filePath_);
    Statement command(database.handle(), "DELETE FROM records WHERE id = ?;");
    command.bindInt(1, id);
    checkDone(command.step(), database.handle(), "Deleting record");
    return SQLiteApi::instance().changes(database.handle()) == 1;
}

std::vector<Record> RecordStore::loadPage(int pageId, int pageSize) const {
    if (pageId < 0 || pageSize < 1) {
        return {};
    }
    DatabaseConnection database(filePath_);
    Statement query(database.handle(),
                    "SELECT id, name, department, score FROM records ORDER BY id LIMIT ? OFFSET ?;");
    query.bindInt(1, pageSize);
    query.bindInt(2, pageId * pageSize);
    std::vector<Record> records;
    while (query.step() == SQLITE_ROW) {
        records.push_back({query.integer(0), query.text(1), query.text(2), query.integer(3)});
    }
    return records;
}

int RecordStore::pageForRecord(int recordId, int pageSize) const {
    if (pageSize < 1) {
        return -1;
    }
    DatabaseConnection database(filePath_);
    Statement query(database.handle(),
                    "SELECT COUNT(*) FROM records WHERE id < ? AND EXISTS "
                    "(SELECT 1 FROM records WHERE id = ?);");
    query.bindInt(1, recordId);
    query.bindInt(2, recordId);
    if (query.step() != SQLITE_ROW) {
        return -1;
    }

    const int earlierRecords = query.integer(0);
    Statement exists(database.handle(), "SELECT 1 FROM records WHERE id = ?;");
    exists.bindInt(1, recordId);
    return exists.step() == SQLITE_ROW ? earlierRecords / pageSize : -1;
}

void RecordStore::ensureDatabaseFile() const {
    DatabaseConnection database(filePath_);
    database.run(
        "CREATE TABLE IF NOT EXISTS records ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "department TEXT NOT NULL, "
        "score INTEGER NOT NULL CHECK(score BETWEEN 0 AND 100)"
        ");");

    Statement count(database.handle(), "SELECT COUNT(*) FROM records;");
    if (count.step() == SQLITE_ROW && count.integer(0) > 0) {
        return;
    }

    database.run("BEGIN TRANSACTION;");
    try {
        const std::vector<Record> seedData = {
            {0, "Aarav Mehta", "Computer Science", 88},
            {0, "Diya Kapoor", "Computer Science", 91},
            {0, "Kabir Shah", "Information Technology", 76},
            {0, "Meera Iyer", "Computer Science", 84},
            {0, "Rohan Gupta", "Electronics", 72},
            {0, "Ishita Verma", "Information Technology", 95},
            {0, "Vivaan Rao", "Computer Science", 81},
            {0, "Ananya Bose", "Electronics", 89},
        };
        for (const auto& record : seedData) {
            Statement insert(database.handle(),
                             "INSERT INTO records (name, department, score) VALUES (?, ?, ?);");
            insert.bindText(1, record.name);
            insert.bindText(2, record.department);
            insert.bindInt(3, record.score);
            checkDone(insert.step(), database.handle(), "Adding seed record");
        }
        database.run("COMMIT;");
    } catch (...) {
        try {
            database.run("ROLLBACK;");
        } catch (...) {
        }
        throw;
    }
}

void RecordStore::validate(const std::string& name, const std::string& department, int score) {
    if (name.empty() || department.empty()) {
        throw std::invalid_argument("Name and department cannot be empty.");
    }
    if (score < 0 || score > 100) {
        throw std::invalid_argument("Score must be between 0 and 100.");
    }
}

void RecordStore::save(const std::vector<Record>&) const {
    // Kept only for compatibility with the RecordStore interface. SQLite
    // writes each CRUD change directly using INSERT, UPDATE, or DELETE.
}
