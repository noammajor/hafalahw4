#include <iostream>
#include <cmath>
struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};
/*
struct memory_data {
    void *memory;
    int Max_size = 10;
*/
void* memory= nullptr;
  void memory_data() {
      void* alline = sbrk(0);
      int relventSize = alline%(32*128*1024);
      int size = (32*128*1024)-relventSize;
      memory = sbrk(size+ 11 * sizeof(MallocMetadata*)+32*128*1024);
      memory = memory+size;
      memset(memory, 0,10*sizeof(MallocMetadata*));
      void* tempMovmment = memory+ 10*sizeof(MallocMetadata*);
      for (int i = 0; i < 32; ++i) {
          if(i==0)
          {
              MallocMetadata mallocMetadata{static_cast<size_t>(128 * 1024), true, tempMovmment+sizeof (MallocMetadata*)+128*1024, nullptr};
              memmove(tempMovmment+ sizeof(MallocMetadata*), &mallocMetadata, sizeof(MallocMetadata));
          }
          if(i==31)
          {
              MallocMetadata mallocMetadata{static_cast<size_t>(128 * 1024), true, nullptr,tempMovmment+ sizeof(MallocMetadata*)+128*1024*31}
              memmove(tempMovmment+ sizeof(MallocMetadata*), &mallocMetadata, sizeof(MallocMetadata));
          }
          else
          {
              MallocMetadata mallocMetadata{static_cast<size_t>(128 * 1024), true, tempMovmment+ sizeof(MallocMetadata*)+128*1024*(i-1),tempMovmment+
              sizeof(MallocMetadata*)+128*1024*(i+1)}
              memmove(tempMovmment+ sizeof(MallocMetadata*), &mallocMetadata, sizeof(MallocMetadata));
          }

};



void cut_block(void* mem, size_t sizeBlock,size_t sizeNeeded,memory_data* memoryData)
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
      }