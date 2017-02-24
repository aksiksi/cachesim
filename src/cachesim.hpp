#ifndef CACHESIM_H
#define CACHESIM_H

#include <inttypes.h>
#include <string>

typedef uint64_t u64;

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
   
	double   hit_time;
    double   miss_penalty;
    double   miss_rate;
    double   avg_access_time;
};

static const uint64_t DEFAULT_C = 15;   /* 64KB Cache */
static const uint64_t DEFAULT_B = 5;    /* 32-byte blocks */
static const uint64_t DEFAULT_S = 3;    /* 8 blocks per set */
static const uint64_t DEFAULT_V = 4;    /* 4 victim blocks */
static const uint64_t DEFAULT_K = 3;	/* 8 byte sub-blocks */

/** Argument to cache_access rw. Indicates a load */
static const char     READ = 'r';
/** Argument to cache_access rw. Indicates a store */
static const char     WRITE = 'w';

#endif /* CACHESIM_H */
