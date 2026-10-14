# Page Vault - C++ SQLite + OS prototype

This is a working 40% C++14 console prototype for the OS and DBMS project.
The persistence and paging foundation is complete; concurrency, locks,
scheduling, dashboard charts, and the GUI are future milestones.

## VS Code project structure

| File | Responsibility |
|---|---|
| `src/database.hpp`, `src/database.cpp` | **DBMS:** SQLite schema, CRUD, data retrieval, logical disk pages |
| `src/memory.hpp`, `src/memory.cpp` | **OS:** RAM frames, page table, page hit/fault, FIFO and LRU |
| `src/main.cpp` | Console menu and DBMS-to-OS integration |
| `data/page_vault.db` | Actual SQLite database, created automatically on first run |

## Database table

The program creates this SQLite table automatically:

```sql
CREATE TABLE records (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    department TEXT NOT NULL,
    score INTEGER NOT NULL CHECK(score BETWEEN 0 AND 100)
);
```

`RecordStore` uses prepared SQLite statements:

- `SELECT` to retrieve rows and logical pages
- `INSERT` to add a record
- `UPDATE` to edit a record
- `DELETE` to remove a record

The initial database has eight seed records. Records are grouped into fixed
logical pages of three records. The OS `MemoryManager` loads those pages into
two simulated RAM frames and applies FIFO or LRU when RAM is full.

## Run in Visual Studio Code

1. Open this `PageVault_CPP` folder in VS Code.
2. Press **Ctrl+Shift+B** and choose **Build PageVault (C++)**.
3. In the VS Code terminal, run `./PageVault.exe`.

## Viva demonstration

1. Choose option `1` to show the SQLite records.
2. Choose option `2`, then access record IDs `1`, `4`, `1`, and `7`.
3. Explain: first two accesses are page faults; the third is a page hit; the
   fourth requires FIFO replacement because RAM has two frames.
4. Choose option `7` to show RAM frames and the page table.

## Viva line

“The DBMS module stores persistent records in SQLite and retrieves the
required logical page using SQL. The OS module checks the page table, loads a
missing page from SQLite into a limited RAM-frame pool, and handles page
replacement using FIFO or LRU.”
