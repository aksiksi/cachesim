#ifndef CACHESIM_H
#define CACHESIM_H

// Shorter way to write it
typedef uint64_t u64;

// Struct type for input argument storage
struct inputargs_t {
    u64 C, B, S, V, K;
    std::istream *trace_file;
};

// Args string for getopt()
static const char *ALLOWED_ARGS = "C:B:S:V:K:i:";

/*
    Maintains state for a single cache of any type.
*/
class Cache {
public:
    u64 C, B, S;
    Cache(u64 C, u64 B, u64 S) : C(C), B(B), S(S) {}
};

struct cache_stats_t {
    uint64_t accesses;

    uint64_t reads;
    uint64_t read_misses;
    uint64_t read_misses_combined;

    uint64_t writes;
    uint64_t write_misses;
    uint64_t write_misses_combined;

    uint64_t misses;
	uint64_t write_backs;
	uint64_t vc_misses;

    uint64_t subblock_misses;

	uint64_t bytes_transferred; 
   
	uint64_t hit_time;
    uint64_t miss_penalty;
    double   miss_rate;
    double   avg_access_time;
};

static const uint64_t DEFAULT_C = 15;   /* 32KB Cache */
static const uint64_t DEFAULT_B = 6;    /* 64-byte blocks */
static const uint64_t DEFAULT_S = 3;    /* 8 blocks per set */
static const uint64_t DEFAULT_V = 4;    /* 4 victim blocks */
static const uint64_t DEFAULT_K = 3;	/* 8 byte sub-blocks */

/** Argument to cache_access rw. Indicates a load */
static const char     READ = 'r';
/** Argument to cache_access rw. Indicates a store */
static const char     WRITE = 'w';

#endif /* CACHESIM_H */