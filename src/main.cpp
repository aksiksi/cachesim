#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <stdlib.h>

#include "cachesim.h"

struct inputargs_t {
    uint64_t C, B, S, V, K;
    std::istream *trace_file;
} inputargs;

void parse_args(int argc, char **argv) {
    // Variables for getopt()
    extern char *optarg; 
    extern int optind;
    
    int c;
    uint64_t num;  // Stores converted arg from char* to uint64_t
    uint64_t *arg; // Pointer to struct arg (DRY)

    // Set defaults for args
    inputargs.C = DEFAULT_C;
    inputargs.B = DEFAULT_B;
    inputargs.S = DEFAULT_S;
    inputargs.V = DEFAULT_V;
    inputargs.K = DEFAULT_K;
    inputargs.trace_file = &std::cin; // Defaults to stdin

    while ((c = getopt(argc, argv, "C:B:S:V:K:i:")) != -1) {
        if (c == 'C' || c == 'B' || c == 'S' || c == 'V' || c == 'K')
            num = strtol(optarg, NULL, 10);
        
        switch (c) {
            case 'C':
                arg = &(inputargs.C);
                break;
            case 'B':
                arg = &(inputargs.B);
                break;
            case 'S':
                arg = &(inputargs.S);
                break;
            case 'V':
                arg = &(inputargs.V);
                break;
            case 'K':
                arg = &(inputargs.K);
                break;
            case 'i':
                std::ifstream ifs;
                ifs.open(optarg);
                inputargs.trace_file = &ifs;
                break;
        }

        *arg = static_cast<uint64_t>(num);
    }
}

int main(int argc, char **argv) {
    inputargs.C = 100;

    parse_args(argc, argv);

    std::cout << "Updated: " << inputargs.C << inputargs.V << std::endl;

    return 0;
}
