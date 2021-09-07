#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include "SMCom.h"
#include "public_test.h"
//#include "private_test.h"

#include "thread_test.h"



int main(int argc, char const *argv[]){
    
    printf("Starting test\n");
    //public_test();
    
    std::vector<std::thread> tvec = thread_test();
    for(auto & x: tvec){
        x.join();
    }

    printf("Finished test\n");

    
    return 0;
}
