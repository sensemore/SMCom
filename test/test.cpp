#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include "SMCom.h"
#include "public_test.h"
//#include "private_test.h"




int main(int argc, char const *argv[]){
    (void)argc;
    (void)argv;
    printf("Calling public_test:\n");
    public_test();
    

    
    return 0;
}
