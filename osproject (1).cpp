#include <iostream>
#include <vector>
#include <limits>
#include <string>
#include <queue>
#include <iomanip>

using namespace std;

struct Block {
    int start;
    int size;
    bool free;
    int pid;  // -1 if free
};

class MemoryAllocator {
private:
    vector<Block> memory;
    queue<int> allocationOrder; // for swapping
    int totalSize;
    int pageSize;
    int lastPos; // for next fit

public:
    MemoryAllocator(int size, int pageSize = 100) : totalSize(size), pageSize(pageSize), lastPos(0) {
        memory.push_back({0, size, true, -1});
    }

    bool allocate(int pid, int size, const string& strategy) {
        int pages = (size + pageSize - 1) / pageSize;
        int allocSize = pages * pageSize;

        int index = -1;
        if (strategy == "first") index = findFirstFit(allocSize);
        else if (strategy == "best") index = findBestFit(allocSize);
        else if (strategy == "next") index = findNextFit(allocSize);
        else {
            cout << "Unknown strategy\n";
            return false;
        }

        if (index == -1) {
            cout << "No space, attempting swap...\n";
            if (!swapOut()) {
                cout << "Swap failed. No memory available.\n";
                return false;
            }
            return allocate(pid, size, strategy); // retry after swap
        }

        Block& blk = memory[index];
        if (blk.size > allocSize) {
            memory.insert(memory.begin() + index + 1,
                {blk.start + allocSize, blk.size - allocSize, true, -1});
        }

        blk.size = allocSize;
        blk.free = false;
        blk.pid = pid;
        allocationOrder.push(pid);

        cout << "Allocated " << allocSize << " units (in " << pages << " pages) to PID " << pid << " at address " << blk.start << "\n";
        return true;
    }

    void deallocate(int pid) {
        bool found = false;
        for (auto& blk : memory) {
            if (!blk.free && blk.pid == pid) {
                blk.free = true;
                blk.pid = -1;
                found = true;
                cout << "Freed memory of PID " << pid << " at address " << blk.start << "\n";
            }
        }
        if (!found) cout << "PID " << pid << " not found\n";
        merge();
    }

    void display() {
        cout << "\n--- Memory Blocks ---\n";
        for (const auto& blk : memory) {
            cout << "[" << blk.start << " - " << blk.start + blk.size - 1 << "] : "
                 << (blk.free ? "Free" : ("Allocated (PID " + to_string(blk.pid) + ")")) << "\n";
        }
    }

    void showVisual() {
        cout << "\n--- Memory Map ---\n";
        int blocks = 50;
        int blockSize = totalSize / blocks;
        vector<char> map(blocks, '.');

        for (const auto& blk : memory) {
            if (!blk.free) {
                int start = blk.start / blockSize;
                int end = (blk.start + blk.size - 1) / blockSize;
                for (int i = start; i <= end && i < blocks; ++i) {
                    map[i] = '#';
                }
            }
        }

        for (char c : map) cout << c;
        cout << "\nLegend: # = Allocated, . = Free\n";
    }

private:
    void merge() {
        for (size_t i = 0; i < memory.size() - 1;) {
            if (memory[i].free && memory[i + 1].free) {
                memory[i].size += memory[i + 1].size;
                memory.erase(memory.begin() + i + 1);
            } else ++i;
        }
    }

    bool swapOut() {
        while (!allocationOrder.empty()) {
            int victim = allocationOrder.front();
            allocationOrder.pop();
            for (auto& blk : memory) {
                if (blk.pid == victim) {
                    blk.free = true;
                    blk.pid = -1;
                    cout << "Swapped out PID " << victim << "\n";
                    merge();
                    return true;
                }
            }
        }
        return false;
    }

    int findFirstFit(int size) {
        for (int i = 0; i < memory.size(); ++i)
            if (memory[i].free && memory[i].size >= size)
                return i;
        return -1;
    }

    int findBestFit(int size) {
        int minSize = numeric_limits<int>::max(), index = -1;
        for (int i = 0; i < memory.size(); ++i)
            if (memory[i].free && memory[i].size >= size && memory[i].size < minSize)
                minSize = memory[i].size, index = i;
        return index;
    }

    int findNextFit(int size) {
        int n = memory.size();
        for (int count = 0; count < n; ++count) {
            int i = (lastPos + count) % n;
            if (memory[i].free && memory[i].size >= size) {
                lastPos = (i + 1) % n;
                return i;
            }
        }
        return -1;
    }
};

int main() {
    MemoryAllocator allocator(1000); // 1000 units of memory, page size = 100
    string cmd;

    cout << "=== Virtual Memory Allocator ===\n";
    cout << "Commands:\n  alloc - allocate memory\n  free - free memory\n  show - show memory table\n  map - ASCII memory map\n  exit - quit\n";

    while (true) {
        cout << "\n> ";
        cin >> cmd;

        if (cmd == "alloc") {
            int pid, size;
            string strategy;
            cout << "Enter PID: "; cin >> pid;
            cout << "Enter size: "; cin >> size;
            cout << "Strategy (first/best/next): "; cin >> strategy;
            allocator.allocate(pid, size, strategy);
        }
        else if (cmd == "free") {
            int pid;
            cout << "Enter PID: ";
            cin >> pid;
            allocator.deallocate(pid);
        }
        else if (cmd == "show") {
            allocator.display();
        }
        else if (cmd == "map") {
            allocator.showVisual();
        }
        else if (cmd == "exit") {
            break;
        }
        else {
            cout << "Unknown command.\n";
        }
    }

    return 0;
}
