#pragma once

#include "database.hpp"

#include <functional>
#include <vector>

// OS MODULE
// Simulates RAM frames, the page table, page faults, and page replacement.
enum class Policy { FIFO, LRU };

struct Frame {
    int id{};
    int pageId{-1};
    std::vector<Record> records;
    int loadedAt{};
    int lastAccess{};
    int accessCount{};
};

struct AccessResult {
    bool hit{};
    int frameId{};
    int evictedPage{-1};
    std::vector<Record> records;
};

class MemoryManager {
public:
    MemoryManager(int frameCount, Policy policy);

    AccessResult accessPage(int pageId, const std::function<std::vector<Record>(int)>& diskLoader);
    void setPolicy(Policy policy);
    void reset();
    void showState() const;

private:
    std::vector<Frame> frames_;
    Policy policy_;
    int clock_{};
    int hits_{};
    int faults_{};

    Frame* findFrameForLoad();
};

