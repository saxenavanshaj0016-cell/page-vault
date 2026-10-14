#include "memory.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <stdexcept>

MemoryManager::MemoryManager(int frameCount, Policy policy) : policy_(policy) {
    if (frameCount < 1) {
        throw std::invalid_argument("At least one RAM frame is required.");
    }
    for (int id = 0; id < frameCount; ++id) {
        frames_.push_back({id, -1, {}, 0, 0, 0});
    }
}

AccessResult MemoryManager::accessPage(
    int pageId, const std::function<std::vector<Record>(int)>& diskLoader) {
    ++clock_;
    for (auto& frame : frames_) {
        if (frame.pageId == pageId) {
            ++hits_;
            frame.lastAccess = clock_;
            ++frame.accessCount;
            return {true, frame.id, -1, frame.records};
        }
    }

    const auto diskRecords = diskLoader(pageId);
    if (diskRecords.empty()) {
        throw std::runtime_error("Requested logical page does not exist on disk.");
    }

    ++faults_;
    Frame* target = findFrameForLoad();
    const int evictedPage = target->pageId;
    target->pageId = pageId;
    target->records = diskRecords;
    target->loadedAt = clock_;
    target->lastAccess = clock_;
    target->accessCount = 1;
    return {false, target->id, evictedPage, target->records};
}

void MemoryManager::setPolicy(Policy policy) {
    policy_ = policy;
}

void MemoryManager::reset() {
    for (auto& frame : frames_) {
        frame.pageId = -1;
        frame.records.clear();
        frame.loadedAt = 0;
        frame.lastAccess = 0;
        frame.accessCount = 0;
    }
    hits_ = faults_ = clock_ = 0;
}

void MemoryManager::showState() const {
    std::cout << "\nRAM FRAMES\n";
    std::cout << "Frame  Page     Records  Loaded  Last access  Uses\n";
    std::cout << "--------------------------------------------------\n";
    for (const auto& frame : frames_) {
        std::cout << std::left << std::setw(7) << frame.id;
        if (frame.pageId < 0) {
            std::cout << "Empty\n";
        } else {
            std::cout << std::setw(9) << frame.pageId << std::setw(9) << frame.records.size()
                      << std::setw(8) << frame.loadedAt << std::setw(13) << frame.lastAccess
                      << frame.accessCount << '\n';
        }
    }

    std::cout << "\nPAGE TABLE\n";
    bool hasPages = false;
    for (const auto& frame : frames_) {
        if (frame.pageId >= 0) {
            std::cout << "Logical page " << frame.pageId << " -> RAM frame " << frame.id << '\n';
            hasPages = true;
        }
    }
    if (!hasPages) {
        std::cout << "No pages currently loaded.\n";
    }

    const int total = hits_ + faults_;
    const double hitRatio = total == 0 ? 0.0 : static_cast<double>(hits_) / total;
    std::cout << "\nSTATS: accesses=" << total << ", hits=" << hits_ << ", faults=" << faults_
              << ", hit ratio=" << std::fixed << std::setprecision(2) << hitRatio * 100 << "%\n";
}

Frame* MemoryManager::findFrameForLoad() {
    for (auto& frame : frames_) {
        if (frame.pageId < 0) {
            return &frame;
        }
    }
    if (policy_ == Policy::FIFO) {
        return &*std::min_element(frames_.begin(), frames_.end(),
                                  [](const Frame& left, const Frame& right) {
                                      return left.loadedAt < right.loadedAt;
                                  });
    }
    return &*std::min_element(frames_.begin(), frames_.end(),
                              [](const Frame& left, const Frame& right) {
                                  return left.lastAccess < right.lastAccess;
                              });
}

