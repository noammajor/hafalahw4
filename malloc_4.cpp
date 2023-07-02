#include <iostream>
#include <cmath>
#include <unistd.h>
#include <stdio.h>
#include <memory.h>
#include <sys/mman.h>

# define MOD_BLOCK_SIZE 4096
#define MAX_SIZE 128 * 1024

struct Stats {
    size_t free_blocks;
    size_t free_bytes;
    size_t allocated_blocks;
    size_t allocated_bytes;
    size_t size_meta_data;
};
struct  MallocMetadata {
    size_t size;
    bool is_free;
    int cookie;
    MallocMetadata* next;
    MallocMetadata* prev;
};
int cookie = rand();
void* memory_base = nullptr;
MallocMetadata* blocks_base = nullptr;
MallocMetadata** size_table = nullptr;
MallocMetadata* mmapBlock = nullptr;
Stats* stats;

//allocate space in heap at the first call to malloc
void memory_data()
{
    memory_base = sbrk(0);
    int relventSize = ((long long int)memory_base + 11 * sizeof(MallocMetadata**)) % (32*128*1024);
    int diff = (32*128*1024) - relventSize + 11 * sizeof(MallocMetadata**);
    memory_base = sbrk(diff);
    memory_base = sbrk(32*128*1024);
    size_table = (MallocMetadata**)memory_base - 11;        //points to the 11-size table
    blocks_base = (MallocMetadata*)memory_base;     //points to the start of the metadata blocks
    for (int i = 0 ; i < 10 ; i++)
        size_table[i] = nullptr;
    size_table[10] = blocks_base;   //ptr from table to max_size linked list
    for (int i = 0; i < 32 ; ++i)
    {
        MallocMetadata mallocMetadata;
        if (i == 0)
            mallocMetadata = {128 * 1024, true, cookie, blocks_base + MOD_BLOCK_SIZE, nullptr};
        else if (i == 31)
            mallocMetadata = {128 * 1024, true, cookie, nullptr, blocks_base + MOD_BLOCK_SIZE * 31};
        else
            mallocMetadata = {128 * 1024, true,cookie, blocks_base + MOD_BLOCK_SIZE * (i + 1),blocks_base + MOD_BLOCK_SIZE * (i - 1)};
        memmove(blocks_base + i * MOD_BLOCK_SIZE, &mallocMetadata, sizeof(MallocMetadata));
    }
    stats->allocated_bytes = 32*128*1024;
    stats->allocated_blocks = 32;
    stats->free_blocks = 32;
    stats->free_bytes = 32*128*1024;
    stats->size_meta_data = sizeof(MallocMetadata);
}


void* mapMalloc(size_t size)
{
    void* ptr = mmap(nullptr, size+sizeof(MallocMetadata), PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
    if(ptr == (void*) -1)
        return nullptr;
    stats->allocated_bytes+=size+sizeof(MallocMetadata);
    stats->allocated_blocks+=1;
    MallocMetadata meta = {cookie,size+sizeof (MallocMetadata), false, nullptr, mmapBlock};
    memmove(ptr, &meta, sizeof(MallocMetadata));
    mmapBlock = (MallocMetadata*)ptr;
    return (MallocMetadata*)ptr + 1;
}


void* findBlock(int sizeFactor)
{
    MallocMetadata* finalBlock;
    int splitCnt = 0;
    bool found = false;
    while (!found && sizeFactor < 11)
    {
        while (!size_table[sizeFactor])
        {
            if(size_table[sizeFactor]->cookie!=cookie)
                exit(0xdeadbeef);
            splitCnt++;
            sizeFactor++;
        }
        finalBlock = size_table[sizeFactor];
        int i = 1;
        while (finalBlock && !finalBlock->is_free && finalBlock->size > 0)
        {
            if (finalBlock->cookie!=cookie)
                exit(0xdeadbeef);
            i++;
            finalBlock = finalBlock->next;
        }
        if (finalBlock && finalBlock->is_free)
            found = true;
        splitCnt++;
        sizeFactor++;
    }
    splitCnt--;
    sizeFactor--;
    if (!found)
        return nullptr;

    if (finalBlock->next)       //remove the block from the list
        finalBlock->next->prev = finalBlock->prev;
    if (finalBlock->prev)
        finalBlock->prev->next = finalBlock->next;
    else
        size_table[sizeFactor] = finalBlock->next;

    while (splitCnt > 0)
    {
        stats->allocated_blocks+=1;
        stats->free_blocks+=1;
        finalBlock->size  = finalBlock->size / 2;
        long long unsigned int buddyAddress = (size_t)finalBlock ^ finalBlock->size;

        splitCnt--;
        sizeFactor--;
        MallocMetadata* tempNext = size_table[sizeFactor];
        MallocMetadata* buddy = (MallocMetadata*)buddyAddress;
        if(tempNext->cookie!=cookie || buddy->cookie!=cookie)
            exit(0xdeadbeef);
        size_t curSize = 128 * pow(2, sizeFactor + splitCnt);
        *buddy = {curSize, true, tempNext, nullptr};
        size_table[sizeFactor] = buddy;
        if (tempNext)
            tempNext->prev = buddy;
    }

    MallocMetadata* tempNext = size_table[sizeFactor];
    size_table[sizeFactor] = finalBlock;
    size_t curSize = 128 * pow(2, sizeFactor);
    *finalBlock = {curSize, false, tempNext, nullptr};
    if (tempNext)
        tempNext->prev = finalBlock;
    stats->free_bytes-=finalBlock->size;
    return finalBlock;
}

int main()
{
    std::cout << "Hello, World!" << std::endl;
    int j;
    for (int i = 0; i <10 ; ++i)
    {
        j=1<<i;
     std::cout<<j<<std::endl;
    }
    return 0;
}

