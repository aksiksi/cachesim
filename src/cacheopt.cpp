#include <iostream>
#include <fstream>

#include "cache.hpp"
#include "util.hpp"

void print_data(double aat, CacheSize size) {
    std::cout << "C = " << size.C << ",";
    std::cout << "B = " << size.B << ",";
    std::cout << "S = " << size.S << ",";
    std::cout << "K = " << size.K << ",";
    std::cout << "V = " << size.V << std::endl;
    std::cout << "AAT = " << aat << std::endl;
}

int main() {
    u64 B, C, S, V, K;

    // C, B, S, K, V
    CacheSize size;
    CacheType ct;
    cache_stats_t stats;
    bool vc;

    Cache* L1;

    std::ifstream ifs;
    char mode;
    u64 address;

    for (B = 3; B <= 7; B++) {
        for (C = B; C <= 30; C++) {
            for (S = 0; S <= (C-B); S++) {
                for (V = 0; V <= 8; V++) {
                    for (K = 1; K <= (B-1); K++) {
                        ifs.open("traces/bzip2.trace");

                        size = {C, B, S, K, V};
                        ct = find_cache_type(size);
                        stats = {};

                        vc = false;

                        if (V > 0)
                            vc = true;

                        L1 = new Cache(size, ct, &stats, vc);

                        // Core simulation loop
                        while (ifs >> mode >> std::hex >> address) {
                            switch (mode) {
                                case 'r':
                                case 'R':
                                    L1->read(address);
                                    break;
                                case 'w':
                                case 'W':
                                    L1->write(address);
                                    break;
                                default:
                                    exit_on_error("Invalid input file format");
                            }
                        }

                        stats.misses = stats.read_misses + stats.write_misses;
                        stats.miss_rate = static_cast<double>(stats.misses + stats.subblock_misses) / stats.accesses;

                        if (vc) {
                            double vc_miss_rate = static_cast<double>(stats.vc_misses) / stats.misses;
                            stats.miss_rate *= vc_miss_rate;
                        }

                        stats.avg_access_time = stats.hit_time + stats.miss_rate * stats.miss_penalty;

                        print_data(stats.avg_access_time, size);

                        delete L1;

                        ifs.close();
                    }
                }
            }
        }
    }

    return 0;
}