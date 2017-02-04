#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <stdlib.h>

#include "cachesim.hpp"

// Cache::Cache(u64 C, u64 B, u64 S) {
//     this.C = C;
//     this.B = B;
//     this.S = S;
// }

// Global input args struct
inputargs_t inputargs;

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

    while ((c = getopt(argc, argv, ALLOWED_ARGS)) != -1) {
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
                // Create a pointer for persistence
                std::ifstream *ifs = new std::ifstream();
                ifs->open(optarg);
                inputargs.trace_file = ifs;
                break;
        }

        if (c != 'i')
            *arg = static_cast<uint64_t>(num);
    }
}

int main(int argc, char **argv) {
    // Parse command line args
    // Info stored in global inputargs struct
    parse_args(argc, argv);

    // Create cache_stats struct
    cache_stats_t cache_stats;

    // Create smart ptr to heap allocated istream
    // Deleted at end of scope
    std::unique_ptr<std::istream> lines (inputargs.trace_file);
    std::string line;

    // Core simulation loop
    while (!lines->eof()) {
        getline(*lines, line);
        std::cout << line << std::endl;
    }

    return 0;
}
