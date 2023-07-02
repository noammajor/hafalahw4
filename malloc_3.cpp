#include <iostream>
#include <cmath>
#include <unistd.h>
#include <stdio.h>
#include <memory.h>
#include <sys/mman.h>

# define MOD_BLOCK_SIZE 2048
#define MAX_SIZE 128 * 1024

struct Stats {
    size_t free_blocks;
    size_t free_bytes;
    size_t allocated_blocks;
    size_t allocated_bytes;
    size_t size_meta_data;
};


struct  MallocMetadata {
    int cookie;
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};


int cookie;
void* memory_base = nullptr;
MallocMetadata* blocks_base = nullptr;
MallocMetadata** size_table = nullptr;
MallocMetadata* mmapBlock = nullptr;
Stats stats;

//allocate space in heap at the first call to malloc
void memory_data()
{
    cookie = rand();
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
            mallocMetadata = {cookie,128 * 1024, true,  blocks_base + MOD_BLOCK_SIZE, nullptr};
        else if (i == 31)
            mallocMetadata = {cookie, 128 * 1024, true, nullptr, blocks_base + MOD_BLOCK_SIZE * 31};
        else
            mallocMetadata = {cookie, 128 * 1024, true,blocks_base + MOD_BLOCK_SIZE * (i + 1),blocks_base + MOD_BLOCK_SIZE * (i - 1)};
        memmove(blocks_base + i * MOD_BLOCK_SIZE, &mallocMetadata, sizeof(MallocMetadata));
    }
    stats.allocated_bytes = 32*128*1024;
    stats.allocated_blocks = 32;
    stats.free_blocks = 32;
    stats.free_bytes = 32*128*1024;
    stats.size_meta_data = sizeof(MallocMetadata);
}


void* mapMalloc(size_t size)
{
    void* ptr = mmap(nullptr, size+sizeof(MallocMetadata), PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
    if(ptr == (void*) -1)
        return nullptr;
    stats.allocated_bytes+=size+sizeof(MallocMetadata);
    stats.allocated_blocks+=1;
    MallocMetadata meta = {cookie, size, false, nullptr, mmapBlock};
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
            splitCnt++;
            sizeFactor++;
        }
        finalBlock = size_table[sizeFactor];
        while (finalBlock && finalBlock->cookie == cookie && !finalBlock->is_free && finalBlock->size > 0)
            finalBlock = finalBlock->next;
        if (finalBlock && finalBlock->cookie != cookie)
            exit(0xdeadbeef);
        if (finalBlock && finalBlock->is_free)
            found = true;
        splitCnt++;
        sizeFactor++;
    }
    splitCnt--;
    sizeFactor--;
    if (!found)
        return nullptr;

    if (finalBlock->next && finalBlock->next->cookie == cookie)       //remove the block from the list
        finalBlock->next->prev = finalBlock->prev;
    if (finalBlock->prev && finalBlock->prev->cookie == cookie)
        finalBlock->prev->next = finalBlock->next;
    else
        size_table[sizeFactor] = finalBlock->next;

    while (splitCnt > 0)
    {
        stats.allocated_blocks+=1;
        stats.free_blocks+=1;
        finalBlock->size  = finalBlock->size / 2;
        long long unsigned int buddyAddress = (size_t)finalBlock ^ finalBlock->size;

        splitCnt--;
        sizeFactor--;
        MallocMetadata* tempNext = size_table[sizeFactor];
        MallocMetadata* buddy = (MallocMetadata*)buddyAddress;
        size_t curSize = 128 * pow(2, sizeFactor + splitCnt);
        *buddy = {cookie, curSize, true, tempNext, nullptr};
        size_table[sizeFactor] = buddy;
        if (tempNext)
        {
            if(tempNext->cookie != cookie)
                exit(0xdeadbeef);
            tempNext->prev = buddy;
        }
    }

    MallocMetadata* tempNext = size_table[sizeFactor];
    size_table[sizeFactor] = finalBlock;
    size_t curSize = 128 * pow(2, sizeFactor);
    *finalBlock = {cookie, curSize, false, tempNext, nullptr};
    if (tempNext)
        tempNext->prev = finalBlock;
    stats.free_bytes-=finalBlock->size;
    return finalBlock;
}


MallocMetadata* mergeBuddies(MallocMetadata* block, size_t size)
{
    long long unsigned int blockAddress = (size_t)block;
    long long unsigned int buddyAddress = blockAddress ^ block->size;
    MallocMetadata* buddy = (MallocMetadata*)buddyAddress;
    size_t curSize = block->size;
    int sizeFactor = log2((block->size)) - 7;

    if (block->next)
        block->next->prev = block->prev;
    if (block->prev)
        block->prev->next = block->next;
    else
        size_table[sizeFactor] = block->next;

    while (buddy->is_free && buddy->size == block->size && curSize < size)
    {
        stats.allocated_blocks -= 1;
        stats.free_blocks -= 1;
        curSize *= 2;
        if (buddy->next)
            buddy->next->prev = buddy->prev;
        if (buddy->prev)
            buddy->prev->next = buddy->next;
        else
            size_table[sizeFactor] = buddy->next;

        sizeFactor++;
        blockAddress = blockAddress < buddyAddress ? blockAddress : buddyAddress;
        block = (MallocMetadata*)blockAddress;
        block->size = curSize;
        buddyAddress = blockAddress ^ curSize;
        buddy = (MallocMetadata*)buddyAddress;
    }

    block->next = size_table[sizeFactor];
    block->prev = nullptr;
    size_table[sizeFactor] = block;
    if (block->next)
        block->next->prev = block;
    return block;
}


void* smalloc(size_t size)
{
    if (size <= 0)
        return nullptr;
    if (!memory_base)       // allocate the heap on the first call
        memory_data();

    if (size > MAX_SIZE - sizeof(MallocMetadata))
        return mapMalloc(size);

    size_t maxSize = pow(2,7) - sizeof(MallocMetadata);
    int reqSizeFactor = 0;
    while (size > maxSize)
    {
        maxSize = (maxSize + sizeof(MallocMetadata)) * 2 - sizeof(MallocMetadata);
        reqSizeFactor++;
    }
    MallocMetadata* result = (MallocMetadata*)findBlock(reqSizeFactor) + 1;     //add meteData size
    return result;
}


void* scalloc(size_t num, size_t size)
{
    void *address = smalloc(num * size);
    if (address)
        memset(address, 0, num * size);
    return address;
}

void mmapFree(void* ptr)
{
    MallocMetadata *temp = (MallocMetadata *) ptr;
    temp--;
    if (temp->next)
        temp->next->prev = temp->prev;
    if(temp->prev)
        temp->prev->next = temp->next;
    stats.allocated_blocks -= 1;
    stats.allocated_bytes -= temp->size;
    munmap(temp, temp->size);
}

void sfree(void* p)
{
    MallocMetadata* block = (MallocMetadata*)p - 1;
    if(block->cookie != cookie)
        exit(0xdeadbeef);
    if (block->is_free)
        return;

    if (block->size > MAX_SIZE)
        mmapFree(p);

    else
    {
        MallocMetadata* mergedBlock = mergeBuddies(block,  MAX_SIZE);
        mergedBlock->is_free = true;
        stats.free_bytes += block->size;
        stats.free_blocks+=1;
    }
}


void* srealloc(void* oldp, size_t size) {
    MallocMetadata* temp = (MallocMetadata*) oldp;
    temp--;
    if(temp->cookie != cookie)
        exit(0xdeadbeef);
    if(temp->size == size)
        return oldp;
    if(temp->size > MAX_SIZE - sizeof(MallocMetadata))
    {
        void* ptr = smalloc(size);
        memmove(ptr,oldp,temp->size);
        mmapFree(oldp);
        return ptr;
    }
    if (size == 0 || size > 1e8)
        return nullptr;
    if (!oldp)
        return smalloc(size);
    size_t reqSize = size + sizeof(MallocMetadata);
    if (((MallocMetadata*)oldp-1)->size >= reqSize)
        return oldp;
    MallocMetadata* mergedBlock = mergeBuddies((MallocMetadata*)oldp - 1,  reqSize);           //////////////write mergeBlocks func
    if (mergedBlock->size >= reqSize)
    {
        sfree(oldp);
        memmove(mergedBlock + 1, oldp, ((MallocMetadata*)oldp-1)->size - sizeof(MallocMetadata));
        mergedBlock->is_free = false;
        stats.free_bytes -= mergedBlock->size;
        stats.free_blocks --;
        return mergedBlock + 1;
    }

    MallocMetadata* newBlock = (MallocMetadata*)smalloc(size);
    memmove(newBlock, oldp, ((MallocMetadata*)oldp-1)->size - sizeof(MallocMetadata));
    sfree(mergedBlock + 1);
    return newBlock;
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

int main()
{
    void* temp1 = smalloc(100);
    void* temp2 = smalloc(100);
    void* temp3 = smalloc(100);
    void* temp4 = smalloc(2*128*1024);
    MallocMetadata* tempM = (MallocMetadata*)temp1 -1;
    std::cout<<temp1<<' '<< tempM->size<<std::endl;
    tempM = (MallocMetadata*)temp2-1;
    std::cout<<temp2<<' '<< tempM->size<<std::endl;
    tempM = (MallocMetadata*)temp3-1;
    std::cout<<temp3<<' '<<tempM->size<<std::endl;
    tempM=(MallocMetadata*)temp4-1;
    std::cout<<temp4<<' '<<tempM->size<<std::endl;
    temp4= realloc(temp4,3*128*1024);
    tempM=(MallocMetadata*)temp4;
    std::cout<<temp4<<' '<<tempM->size<<std::endl;
    temp3 = realloc(temp3,550);
    tempM=(MallocMetadata*)temp3;
    std::cout<<temp3<<' '<<tempM->size<<std::endl;
    for (int i = 0; i < 10; ++i)
    {
      void* ptr = smalloc(100*(i+1));
      tempM=(MallocMetadata*)ptr;
      std::cout<<ptr<<' '<<tempM->size<<std::endl;
      sfree(ptr);
    }
    std::cout<<"free blocks: "<<stats.free_blocks<<" allocated blocks: "<<stats.allocated_blocks<<std::endl;
    std::cout<<"free bytes: "<<stats.free_bytes<< " allocated bytes:"<<stats.allocated_bytes<<std::endl;
    std::cout<<stats.size_meta_data<<std::endl;
    sfree(temp1);
    sfree(temp2);
    sfree(temp3);
    sfree(temp4);
    std::cout<<"free blocks: "<<stats.free_blocks<<" allocated blocks: "<<stats.allocated_blocks<<std::endl;
    std::cout<<"free bytes: "<<stats.free_bytes<< " allocated bytes:"<<stats.allocated_bytes<<std::endl;
    temp1 = smalloc(100);
    std::cout<<"free blocks: "<<stats.free_blocks<<" allocated blocks: "<<stats.allocated_blocks<<std::endl;
    std::cout<<"free bytes: "<<stats.free_bytes<< " allocated bytes:"<<stats.allocated_bytes<<std::endl;
    sfree(temp1);
    temp2 = smalloc(3*128*1024);
    std::cout<<"free blocks: "<<stats.free_blocks<<" allocated blocks: "<<stats.allocated_blocks<<std::endl;
    std::cout<<"free bytes: "<<stats.free_bytes<< " allocated bytes:"<<stats.allocated_bytes<<std::endl;
    sfree(temp2);
    temp1 = smalloc(100);
    std::cout<<"free blocks: "<<stats.free_blocks<<" allocated blocks: "<<stats.allocated_blocks<<std::endl;
    std::cout<<"free bytes: "<<stats.free_bytes<< " allocated bytes:"<<stats.allocated_bytes<<std::endl;
    temp1 = realloc(temp1,300);
    std::cout<<"free blocks: "<<stats.free_blocks<<" allocated blocks: "<<stats.allocated_blocks<<std::endl;
    std::cout<<"free bytes: "<<stats.free_bytes<< " allocated bytes:"<<stats.allocated_bytes<<std::endl;

    return 0;
}
