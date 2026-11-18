# Page Vault - OS & DBMS Simulator

A complete, full-stack Virtual Memory Database Application simulating the intersection of Operating Systems (OS) and Database Management Systems (DBMS). It features a C++ / SQLite backend core paired with an interactive frontend dashboard.

## Project Structure & Architecture

| File / Directory | Responsibility |
|---|---|
| `src/database.hpp`, `src/database.cpp` | **DBMS:** SQLite schema, CRUD operations, data retrieval, and logical disk page grouping. |
| `src/memory.hpp`, `src/memory.cpp` | **OS:** RAM frames, page table, page hit/fault tracking, FIFO and LRU replacement policies. |
| `src/main.cpp` | Console backend core and integration. |
| `web/index.html` & assets | **Interactive Frontend:** Real-time dashboard, process scheduler (FCFS/SJL), page-level locking (mutex simulation), and live telemetry. |
| `data/page_vault.db` | Actual SQLite database, created automatically on first run. |

## Core Features

- **SQLite Database Manager:** Handles persistent student records with automated ID generation, validation checks, and CRUD operations.
- **Logical Paging:** Groups database rows into fixed logical disk pages before the OS loads them.
- **Virtual Memory & Replacement:** Simulates limited RAM frames with dynamic replacement algorithms (**FIFO** and **LRU**) and tracks hit/fault ratios.
- **Process Scheduler & Synchronization:** Implements CPU scheduling (FCFS / SJF) for query execution alongside page-level locking (mutex simulation) to prevent write conflicts.
- **Interactive Web UI:** Fully functional browser dashboard mirroring database states, memory frames, and logs in real-time.

## Database Schema

```sql
CREATE TABLE records (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    department TEXT NOT NULL,
    score INTEGER NOT NULL CHECK(score BETWEEN 0 AND 100)
);
