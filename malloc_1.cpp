#include <unistd.h>
#include <stdio.h>
#include <iostream>

void* smalloc(size_t size)
{
    if (size == 0 || size > 1e8)
        return nullptr;
    void* result = sbrk(size);
    if (result == (void*)(-1))
        return nullptr;
    return result;
}

/*int main(){
    void* res = smalloc(0);
    if (!res)
        std::cout << "1 pass" << std::endl;

    res = smalloc(50);
    if (res)
        std::cout << "2 pass" << std::endl;

    res = smalloc(1e8+1);
    if (!res)
        std::cout << "3 pass" << std::endl;

    return 0;
}*/

// run with: g++ -std=c++11 malloc_1.cpp -o malloc.exe
//           ./malloc.exe
