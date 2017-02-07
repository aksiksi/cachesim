// C++ includes
#include <string>
#include <fstream>
#include <iostream>

// C includes
#include <stdlib.h>
#include <unistd.h>

#include "cachesim.hpp"
#include "cache.hpp"

void exit_on_error(std::string msg) {
    std::cout << "Error: " << msg << std::endl;
    exit(EXIT_FAILURE);
}

// Struct type for input argument storage
struct inputargs_t {
    u64 C, B, S, V, K, N;
    std::istream *trace_file;
};

/**
    Parse command line arguments using getopt().
*/
void parse_args(int argc, char **argv, inputargs_t& args) {
    // Variables for getopt()
    extern char *optarg;
    extern int optind;

    // Args string for getopt()
    static const char* ALLOWED_ARGS = "C:B:S:V:K:i:";
    
    int c;
    uint64_t num;  // Stores converted arg from char* to uint64_t
    uint64_t *arg; // Pointer to struct arg (DRY)

    // Set defaults for args
    args.C = DEFAULT_C;
    args.B = DEFAULT_B;
    args.S = DEFAULT_S;
    args.V = DEFAULT_V;
    args.K = DEFAULT_K;
    args.trace_file = &std::cin; // Stream defaults to stdin

    while ((c = getopt(argc, argv, ALLOWED_ARGS)) != -1) {
        if (c == 'C' || c == 'B' || c == 'S' || c == 'V' || c == 'K')
            num = strtol(optarg, NULL, 10);
        
        switch (c) {
            case 'C':
                arg = &(args.C);
                break;
            case 'B':
                arg = &(args.B);
                break;
            case 'S':
                arg = &(args.S);
                break;
            case 'V':
                arg = &(args.V);
                break;
            case 'K':
                arg = &(args.K);
                break;
            case 'i':
                // Create a pointer for persistence
                std::ifstream *ifs = new std::ifstream();
                ifs->open(optarg);
                args.trace_file = ifs;
                break;
        }
        
        if (c != 'i')
            // Store 2^value for simplicity
            *arg = static_cast<uint64_t>(num);
    }

    // Store number of blocks in cache
    args.N = args.C - args.B;

    // Error checking
    if (args.B > args.C)
        exit_on_error("B cannot be greater than C.");
}

int main(int argc, char **argv) {
    // Allocate a args struct and pass by ref
    inputargs_t args;

    // Parse command line args
    // Info stored in global args `struct`
    parse_args(argc, argv, args);

    // Create cache_stats `struct`
    cache_stats_t stats = {};

    // Create smart ptr to heap allocated `istream`
    // (deleted at end of scope)
    std::unique_ptr<std::istream> fs (args.trace_file);

    CacheSize cache_size = {
        args.C,
        args.B,
        args.S,
        args.K,
        args.N
    };

    Cache L1 (cache_size, &stats);

    // Variables for formatting trace input
    char mode;
    u64 address;
    bool hit;

    std::cout << L1.size.C << " " << L1.size.B << " " << L1.block_mask << std::endl;

    // Core simulation loop
    while (*fs >> mode >> address) {
        std::cout << mode << " " << address << std::endl;

        switch (mode) {
            case 'r':
            case 'R':
                hit = L1.read(address);
                if (hit)
                    std::cout << "Hit: " << address << std::endl;
                break;
            case 'w':
            case 'W':
                L1.write(address);
                break;
            default:
                exit_on_error("Invalid input file format");
        }
    }

    // Print some stats
    std::cout << std::endl << "Stats" << std::endl;
    std::cout << "=====" << std::endl;
    std::cout << "Reads: " << stats.reads << std::endl;
    std::cout << "Read Misses: " << stats.read_misses << std::endl;
    std::cout << "Bytes Xferred: " << stats.bytes_transferred << std::endl;


    return 0;
}
