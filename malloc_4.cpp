#include <iostream>

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

