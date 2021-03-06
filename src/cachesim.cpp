// C++ includes
#include <string>
#include <fstream>
#include <iostream>

#include "cachesim.hpp"
#include "cache.hpp"
#include "util.hpp" // exit_on_error

// C includes
#include <unistd.h>

void print_statistics(cache_stats_t* p_stats) {
    printf("\nCache Statistics\n");
    printf("================\n");
    printf("Accesses: %" PRIu64 "\n", p_stats->accesses);
    printf("Reads: %" PRIu64 "\n", p_stats->reads);
    printf("Read misses: %" PRIu64 "\n", p_stats->read_misses);
    printf("Read misses combined: %" PRIu64 "\n", p_stats->read_misses_combined);
    printf("Writes: %" PRIu64 "\n", p_stats->writes);
    printf("Write misses: %" PRIu64 "\n", p_stats->write_misses);
    printf("Write misses combined: %" PRIu64 "\n", p_stats->write_misses_combined);
    printf("Misses: %" PRIu64 "\n", p_stats->misses);
    printf("Writebacks: %" PRIu64 "\n", p_stats->write_backs);
    printf("Victim cache misses: %" PRIu64 "\n", p_stats->vc_misses);
    printf("Sub-block misses: %" PRIu64 "\n", p_stats->subblock_misses);
    printf("Bytes transferred to/from memory: %" PRIu64 "\n", p_stats->bytes_transferred);
    printf("Hit Time: %f\n", p_stats->hit_time);
    printf("Miss Penalty: %f\n", p_stats->miss_penalty);
    printf("Miss rate: %f\n", p_stats->miss_rate);
    printf("Average access time (AAT): %f\n", p_stats->avg_access_time);
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
    u64 num = 0;  // Stores converted arg from char* to uint64_t
    u64 *arg; // Pointer to struct arg (DRY)

    // Set defaults for args
    args.C = DEFAULT_C;
    args.B = DEFAULT_B;
    args.S = DEFAULT_S;
    args.V = DEFAULT_V;
    args.K = DEFAULT_K;
    args.trace_file = nullptr;

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
                // Then open the file in read mode
                std::ifstream *ifs = new std::ifstream();
                ifs->open(optarg);
                
                if (!ifs->good())
                    exit_on_error("File not found.");

                args.trace_file = ifs;
        }
        
        if (c != 'i')
            *arg = static_cast<uint64_t>(num);
    }

    // Store number of blocks in cache
    args.N = args.C - args.B;

    // Error checking
    if (args.B > args.C)
        exit_on_error("B cannot be greater than C.");
}

int main(int argc, char **argv) {
    // Allocate a args struct
    inputargs_t args;

    // Parse command line args
    parse_args(argc, argv, args);

    // Create cache_stats `struct`
    cache_stats_t stats = {};

    // Set fs to either std::cin or file
    // depending on value of trace_file
    std::istream *fs;
    bool file = false;

    if (args.trace_file == nullptr)
        fs = &std::cin;
    else {
        fs = args.trace_file;
        file = true;
    }

    CacheSize cache_size = {
        args.C,
        args.B,
        args.S,
        args.K,
        args.V
    };

    // Find cache type (DM, FA, or SA)
    // Exits if invalid parameters
    CacheType ct = find_cache_type(cache_size);

    // Create L1 cache with given size, type
    // Pass in stats object
    Cache L1 (cache_size, ct, &stats);

    // Variables for formatting trace input
    char mode;
    u64 address;

    // Core simulation loop
    while (*fs >> mode >> std::hex >> address) {
        switch (mode) {
            case 'r':
            case 'R':
                L1.read(address);
                break;
            case 'w':
            case 'W':
                L1.write(address);
                break;
            default:
                exit_on_error("Invalid input file format");
        }
    }

    L1.compute_stats();

    print_statistics(&stats);

    // Free file stream (if applicable)
    if (file)
        delete fs;

    return 0;
}
