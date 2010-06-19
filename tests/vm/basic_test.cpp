#include <cstdlib>
#include <iostream>
#include <vector>
#include <time.h>
#include "code.h"

using namespace machine;

const byte simple_prog[] = 
{
        PUSH_BYTE, 43, 
        PUSH_BYTE, 42,
            PUSH_BYTE, 11, PUSH_BYTE, 13, ADD,
            PUSH_BYTE, 4, SUB,
        COND,
//    POP_RET
};

class Segment 
{
    byte x;
};

std::vector<byte> fuzzer(int);

int main(int argc, char *argv[])
{
    size_t repeats = 1;
    
    if (argc > 1)
        repeats = atoi(argv[1]);
    
    std::cout << "simple program size:    " << sizeof(simple_prog)+1 << " bytes" << std::endl;
    // Replicate the code copies times.
    std::vector<byte> big_prog;
    size_t copies = ((1 << 20)*4 + sizeof(simple_prog)-1) / sizeof(simple_prog);
    big_prog.push_back(simple_prog[0]);
    big_prog.push_back(simple_prog[1]);
    for(; copies; --copies)
        big_prog.insert(big_prog.end(), &simple_prog[2], simple_prog + sizeof(simple_prog));
    big_prog.push_back(POP_RET);
    std::cout << "amplified program size: " << big_prog.size() << " bytes" << std::endl;
    
    // Load the code.
    code prog(false, &big_prog[0], &big_prog[0] + big_prog.size());
    std::cout << "loaded program size:    " << prog.size() << " bytes" << std::endl
              << "                        " << prog.instruction_count() << " instructions" << std::endl;
    
    // run the program
    uint32 * stack = new uint32 [64];
    Segment seg;
    uint32 ret;
    for(int n = repeats; n; --n)
        ret = prog.run(stack, 64, seg, 0);
    std::cout << "result of program: " << ret << std::endl
              << "--------" << std::endl
              << "equivalent of " << prog.instruction_count()*repeats << " instructions executed" << std::endl;
    
    delete [] stack;
    
    return 0;
}


std::vector<byte> random_sequence(size_t n)
{
    std::vector<bool> done(n);
    std::vector<byte> seq(n);
    
    srand(time(NULL));
    
    while(n)
    {
        const size_t r = (rand()*n + RAND_MAX/2)/RAND_MAX;
        
        if (done[r]) continue;
        
        done[r] = true;
        seq[r]  = r;
        --n;
    }
    
    return seq;
}


std::vector<byte> fuzzer(int n)
{
    std::vector<byte>   code(256);
    std::vector<bool>   covered(256);
    
    // Track stack depth to ensure we don't create programs that
    //  overflow or underflow the stack.
    size_t stack_depth = 0;
    
    return code;   
}