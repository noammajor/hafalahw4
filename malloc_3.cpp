#include <iostream>
#include <cmath>
#include <unistd.h>
#include <stdio.h>
#include <memory.h>

# define MOD_BLOCK_SIZE 4096


struct  MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata *next;
    MallocMetadata *prev;
};

void* memory_base = nullptr;

void memory_data()
{
    memory_base = sbrk(0);
    int relventSize = (long long int)memory_base % (32*128*1024);
    int diff = (32*128*1024) - relventSize;
    memory_base = sbrk(diff);
    memory_base = sbrk(11 * sizeof(MallocMetadata*) + 32*128*1024);
    memset(memory_base, 0,10*sizeof(MallocMetadata*));
    MallocMetadata* listPtr = (MallocMetadata*)memory_base+ 10;    //ptr from table to max_size linked list
    MallocMetadata* headAddress = listPtr + 1;
    memmove(listPtr, headAddress, 1);
    listPtr++;
    for (int i = 0; i < 32 ; ++i)
    {
        MallocMetadata mallocMetadata;
        if (i == 0)
            mallocMetadata = {128 * 1024, true, listPtr + MOD_BLOCK_SIZE, nullptr};
        else if (i == 31)
            mallocMetadata = {128 * 1024, true, nullptr, listPtr + MOD_BLOCK_SIZE * 31};
        else
            mallocMetadata = {128 * 1024, true, listPtr + MOD_BLOCK_SIZE * (i + 1),
                                          listPtr + MOD_BLOCK_SIZE * (i - 1)};
        memmove(listPtr + i * MOD_BLOCK_SIZE, &mallocMetadata, sizeof(MallocMetadata));
    }
}


int main()
{
    memory_data();
    std::cout << memory_base << std::endl;
    MallocMetadata* ptr = (MallocMetadata*)memory_base + 11;
    int i = 1;
    while (ptr->next && i < 40)
    {
        std::cout << "block #" << i << "  size: " << ptr->size << std::endl;
        i++;
        ptr = ptr->next;
    }
}


/*void cut_block(void* mem, size_t sizeBlock,size_t sizeNeeded,memory_data* memoryData)
{

}








}

      double result = log2(sizeNeeded);
      int roundedResult = static_cast<int>(ceil(result));
      size_t actual_cut = 1>>roundedResult;
      size_t remander = sizeBlock-actual_cut;
      int roundedRemander = log2(remander);
      void* temp = memoryData->memory;
      // moving the memory pointer to the place i need it in the array of ptrs
      char *charPtr = static_cast<char *>(temp);
      charPtr += roundedRemander*sizeof(MallocMetadata);
      temp = static_cast<void *>(charPtr);
      // cutting the memory
      char *charPtr1 = static_cast<char *>(mem);
      charPtr1 += actual_cut*sizeof(MallocMetadata);
      void* tempNewMemory = static_cast<void *>(charPtr1);
      MallocMetadata mallocMetadata{static_cast<size_t>(1>>roundedRemander), false, nullptr, nullptr};
      memmove(tempNewMemory, &mallocMetadata, sizeof(MallocMetadata));
      //putting it in
      MallocMetadata *head = static_cast<MallocMetadata *>(temp);
      while(tempNewMemory>head)
          head=head->next;
      head=head->prev;
      if(head== nullptr)//begining of the list
      {

      }
      else
      {
          MallocMetadata* temp2 = static_cast<MallocMetadata *>(tempNewMemory);
          MallocMetadata* temp1=head->next;
          head->next = temp2;
          temp2->prev=head;
          temp2->next=temp1;
          temp1->prev=temp2;
      }
      //putting the used memory in
      charPtr = static_cast<char *>(temp);
      charPtr += roundedResult*sizeof(MallocMetadata);
      temp = static_cast<void *>(charPtr);
      MallocMetadata mallocMetadata1{static_cast<size_t>(1>>roundedResult), false, nullptr, nullptr};
      memmove(mem, &mallocMetadata1, sizeof(MallocMetadata));
      MallocMetadata* temp1 = static_cast<MallocMetadata*>(temp);

      while(temp1<mem)
          temp1=temp1->next;
      temp1=temp1->prev;
      if(temp1== nullptr)//begining of the list
      {

      }
      else
      {
          MallocMetadata* temp2 = static_cast<MallocMetadata *>(mem);
          MallocMetadata* temp3=temp1->next;
          temp1->next = temp2;
          temp2->prev=temp1;
          temp2->next=temp3;
          temp3->prev=temp2;
      }*/