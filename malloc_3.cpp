#include <iostream>
#include <cmath>
#include <unistd.h>
#include <stdio.h>
#include <memory.h>

# define MOD_BLOCK_SIZE 4096


struct  MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

void* memory_base = nullptr;
MallocMetadata* blocks_base = nullptr;
MallocMetadata** size_table = nullptr;


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
            mallocMetadata = {128 * 1024, true, blocks_base + MOD_BLOCK_SIZE, nullptr};
        else if (i == 31)
            mallocMetadata = {128 * 1024, true, nullptr, blocks_base + MOD_BLOCK_SIZE * 31};
        else
            mallocMetadata = {128 * 1024, true, blocks_base + MOD_BLOCK_SIZE * (i + 1),blocks_base + MOD_BLOCK_SIZE * (i - 1)};
        memmove(blocks_base + i * MOD_BLOCK_SIZE, &mallocMetadata, sizeof(MallocMetadata));
    }
}


void*  mapMalloc(size_t size)       /////////////////////////////////do later
{
    return nullptr;
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
        int i = 1;
        while (finalBlock && !finalBlock->is_free && finalBlock->size > 0)
        {
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
        finalBlock->size  = finalBlock->size / 2;
        long long unsigned int buddyAddress = (size_t)finalBlock ^ finalBlock->size;

        splitCnt--;
        sizeFactor--;
        MallocMetadata* tempNext = size_table[sizeFactor];
        MallocMetadata* buddy = (MallocMetadata*)buddyAddress;
        size_t curSize = 128 * pow(2, sizeFactor + splitCnt);
        *buddy = {curSize, true, tempNext, nullptr};
        size_table[sizeFactor] = buddy;
        if (tempNext)
            tempNext->prev = buddy;
    }

    MallocMetadata* tempNext = size_table[sizeFactor];
    size_table[sizeFactor] = finalBlock;
    size_t curSize = 128 * pow(2, sizeFactor);
    *finalBlock = {curSize, true, tempNext, nullptr};
    if (tempNext)
        tempNext->prev = finalBlock;
    return finalBlock;
}


void* smalloc(size_t size)
{
    if (size <= 0)
        return nullptr;
    else if (size > pow(2, 17) - sizeof(MallocMetadata))
        return mapMalloc(size);     ///////////////////////////////////// map allocation function

    if (!memory_base)       // allocate the heap on the first call
        memory_data();

    size_t maxSize = pow(2,7) - sizeof(MallocMetadata);
    int reqSizeFactor = 0;
    while (size > maxSize)
    {
        maxSize = (maxSize + sizeof(MallocMetadata)) * 2 - sizeof(MallocMetadata);
        reqSizeFactor++;
    }
    return findBlock(reqSizeFactor);
}


void* scalloc(size_t num, size_t size)
{
    void *address = smalloc(num * size);
    if (address)
        memset(address, 0, num * size);
    return address;
}


void* srealloc(void* oldp, size_t size) {
    if (size == 0 || size > 1e8)
        return nullptr;
    if (!oldp)
        return smalloc(size);
    if (((MallocMetadata*)oldp)->size >= size + 1)
        return oldp;

    ///////////////////////////// mmap realloc ?

    MallocMetadata *temp = oldp;
    double result = log2(size);
    int roundedResult = (int)round(result);
    if (result != roundedResult)
        roundedResult++;
    MallocMetadata *ptr = memory;
    ptr = ptr + sizeof(MallocMetadata *) * (roundedResult - 7);
    while (roundedResult - 7 < 11) {
        if (ptr) {
            MallocMetadata *temp = ptr;
            while (temp && !temp->is_free) {
                temp = temp->next;
            }
            if (temp) {
                if (temp->size < size * 2) {
                    memcpy(oldp, temp, static_cast<MallocMetadata *>(oldp)->size);
                    sfree(oldp);
                    temp->is_free = false;
                    return temp;
                } else {
                    void *tempRet = actual_cut();//later
                    memcpy(oldp, tempRet, static_cast<MallocMetadata *>(oldp)->size);
                    sfree(oldp);
                    return tempRet;
                }
            }
        }
        roundedResult++;
        ptr = ptr + sizeof(MallocMetadata *);
    }
    sfree(oldp);
    return nullptr;
}


int main()
{
    /*MallocMetadata* ptr = (MallocMetadata*)memory_base + 11;
    int i = 1;
    while (ptr->next)
    {
        std::cout << "block #" << i << "  size: " << ptr->size << std::endl;
        i++;
        ptr = ptr->next;
    }
    std::cout << "block #" << i << "  size: " << ptr->size << std::endl;
    size_t size = pow(2,7) - sizeof(MallocMetadata);
    std::cout << size ;*/
    std::cout << smalloc(30) << std::endl;
    return 0;
}
