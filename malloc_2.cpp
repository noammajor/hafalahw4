#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <memory.h>


struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

struct Stats {
    size_t free_blocks;
    size_t free_bytes;
    size_t allocated_blocks;
    size_t allocated_bytes;
    size_t size_meta_data;
};

MallocMetadata head { 0, true, nullptr, nullptr};

Stats stats {0, 0, 0, 0, sizeof(MallocMetadata)};


//Searches for a free block with at least ‘size’ bytes or allocates (sbrk()) one if none are found
void* smalloc(size_t size)
{
    if (size == 0 || size > 1e8)
        return nullptr;
    MallocMetadata* ptr = &head;
    while (ptr->next)       //try to find free block
    {
        ptr = ptr->next;
        if (ptr->is_free && ptr->size >= size)
        {
            ptr->is_free = false;
            stats.free_blocks--;
            stats.free_bytes -= ptr->size;
            return ptr + 1;     // adds sizeof ptr one time
        }
    }       //allocate new block:
    MallocMetadata mallocMetadata { size, false, nullptr, ptr};

    void* address = sbrk(sizeof(MallocMetadata));
    if (address == (void*)(-1))
        return nullptr;
    memmove(address, &mallocMetadata, sizeof(mallocMetadata));
    ptr->next = (MallocMetadata*)address;
    address = sbrk(size);
    if (address == (void*)(-1))
        return nullptr;
    stats.allocated_blocks++;
    stats.allocated_bytes += size;
    return address;
}

// Searches for a free block of at least ‘num’ elements, each ‘size’ bytes that are all set to 0 or allocates if none are found
void* scalloc(size_t num, size_t size)
{
    void* address = smalloc(num * size);
    if (address)
        memset(address, 0, num * size);
    return address;
}

//Releases the usage of the block that starts with the pointer ‘p’
void sfree(void* p)
{
    MallocMetadata* ptr = &head;
    while (ptr->next)       //try to find p
    {
        ptr = ptr->next;
        if (ptr + 1 == p)
        {
            ptr->is_free = true;
            stats.free_blocks++;
            stats.free_bytes += ptr->size;
            return;
        }
    }
}

//If ‘size’ is smaller than or equal to the current block’s size, reuses the same block.
//Otherwise, finds/allocates ‘size’ bytes for a new space, copies content of oldp into the new allocated space and frees the oldp
void* srealloc(void* oldp, size_t size)
{
    if (size == 0 || size > 1e8)
        return nullptr;

    if (!oldp)
        return smalloc(size);

    MallocMetadata* ptr = &head;
    while (ptr->next)       //try to find p
    {
        ptr = ptr->next;
        if (ptr + 1 == oldp)
        {
            if (ptr->size >= size)
                return oldp;
            else
                break;
        }
    }
    void* address = smalloc(size);
    if (!address)
        return nullptr;
    if (!memmove(address, oldp, ptr->size))
        return nullptr;
    sfree(oldp);
    return address;
}



//Returns the number of allocated blocks in the heap that are currently free
size_t _num_free_blocks()
{
    return stats.free_blocks;
}

//Returns the number of bytes in all allocated blocks in the heap that are currently free,
//        excluding the bytes used by the meta-data structs.
size_t _num_free_bytes()
{
    return stats.free_bytes;
}

//Returns the overall (free and used) number of allocated blocks in the heap.
size_t _num_allocated_blocks()
{
    return stats.allocated_blocks;
}

//Returns the overall number (free and used) of allocated bytes in the heap, excluding the bytes used by the meta-data structs.
size_t _num_allocated_bytes()
{
    return stats.allocated_bytes;
}

//Returns the overall number of meta-data bytes currently in the heap.
size_t _num_meta_data_bytes()
{
    return (stats.size_meta_data * (stats.allocated_blocks));
}

//Returns the number of bytes of a single meta-data structure in your system
size_t _size_meta_data()
{
    return stats.size_meta_data;
}

