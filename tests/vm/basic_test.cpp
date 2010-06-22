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

#define _msg(m) #m

const char * prog_error_msg[] = {
    _msg(loaded),
    _msg(alloc_failed), 
    _msg(invalid_opcode), 
    _msg(unimplemented_opcode_used),
    _msg(jump_past_end),
    _msg(arguments_exhausted),
    _msg(missing_return)
};

const char * run_error_msg[] = {
    _msg(finished),
    _msg(stack_underflow),
    _msg(stack_not_empty),
    _msg(stack_overflow)
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
    byte context[256];
    code prog(false, &big_prog[0], &big_prog[0] + big_prog.size(),context);
    if (!prog) {    // Find out why it did't work
        // For now just dump an error message.
        std::cerr << "program failed to load due to: " 
                << prog_error_msg[prog.status()] << std::endl;
        return 1;
    }
    std::cout << "loaded program size:    " 
              << prog.data_size() + prog.instruction_count()*sizeof(instr) 
              << " bytes" << std::endl
              << "                        " 
              << prog.instruction_count() << " instructions" << std::endl;
    
    // run the program
    uint32 * stack = new uint32 [64];
    Segment seg;
    int is=0;
    uint32 ret;
    machine::status_t status;
    for(size_t n = repeats; n; --n) {
        ret = prog.run(stack, 64, seg, is, status);
        switch (status) {
            case stack_underflow:
            case stack_overflow:
                std::cerr << "program terminated early: " 
                          << run_error_msg[status] << std::endl;
                std::cout << "--------" << std::endl
                          << "between " << prog.instruction_count()*(repeats-n) 
                          << " and "    << prog.instruction_count()*(repeats-std::min(n-1,repeats))
                          << " instructions executed" << std::endl;
                return 2;
            case stack_not_empty:
                std::cerr << "program completed but stack not empty." << std::endl;
                repeats -= n-1;
                n=1;
                break;
        }
    }
    
    std::cout << "result of program: " << ret << std::endl
              << "--------" << std::endl
              << "equivalent of " << prog.instruction_count()*repeats 
              << " instructions executed" << std::endl;
    
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
