#ifndef VIT_ARENA_ALLOCATOR_H
#define VIT_ARENA_ALLOCATOR_H

#include <vector>
#include <cstddef>
#include <utility>
#include <cstdint>

namespace vit {

class ArenaAllocator {
public:
    explicit ArenaAllocator(size_t blockSizeBytes = 64 * 1024)
        : defaultBlockSize(blockSizeBytes), currentBlock(nullptr), currentOffset(0) {}

    ~ArenaAllocator() {
        freeAll();
    }

    // Disable copy
    ArenaAllocator(const ArenaAllocator&) = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;

    // Enable move
    ArenaAllocator(ArenaAllocator&& other) noexcept
        : defaultBlockSize(other.defaultBlockSize),
          blocks(std::move(other.blocks)),
          currentBlock(other.currentBlock),
          currentOffset(other.currentOffset) {
        other.currentBlock = nullptr;
        other.currentOffset = 0;
    }

    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
        if (size == 0) return nullptr;

        size_t currentPtr = reinterpret_cast<size_t>(currentBlock + currentOffset);
        size_t alignedPtr = (currentPtr + alignment - 1) & ~(alignment - 1);
        size_t alignmentOffset = alignedPtr - currentPtr;

        if (currentBlock == nullptr || currentOffset + alignmentOffset + size > defaultBlockSize) {
            allocateNewBlock(size + alignment);
            currentPtr = reinterpret_cast<size_t>(currentBlock);
            alignedPtr = (currentPtr + alignment - 1) & ~(alignment - 1);
            alignmentOffset = alignedPtr - currentPtr;
        }

        currentOffset += alignmentOffset + size;
        return reinterpret_cast<void*>(alignedPtr);
    }

    template <typename T, typename... Args>
    T* alloc(Args&&... args) {
        void* ptr = allocate(sizeof(T), alignof(T));
        return new (ptr) T(std::forward<Args>(args)...);
    }

    void freeAll() {
        for (char* block : blocks) {
            delete[] block;
        }
        blocks.clear();
        currentBlock = nullptr;
        currentOffset = 0;
    }

private:
    void allocateNewBlock(size_t minimumSize) {
        size_t sizeToAlloc = (minimumSize > defaultBlockSize) ? minimumSize : defaultBlockSize;
        char* newBlock = new char[sizeToAlloc];
        blocks.push_back(newBlock);
        currentBlock = newBlock;
        currentOffset = 0;
    }

    size_t defaultBlockSize;
    std::vector<char*> blocks;
    char* currentBlock;
    size_t currentOffset;
};

} // namespace vit

#endif // VIT_ARENA_ALLOCATOR_H
