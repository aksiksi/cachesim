#include <iostream>
#include <fstream>
#include <string>

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

    Cache* L1;

    std::ifstream ifs;
    char mode;
    u64 address;
    std::vector<std::string> traces = {
        "traces/astar.trace",
        "traces/bzip2.trace",
        "traces/mcf.trace",
        "traces/perlbench.trace"
    };

    // Select best params given 64 KB budget
    C = 15;
    V = 2;
    B = 7;
    K = 6;

    for (int i = 0; i < traces.size(); i++) {
        double aat_min = 999999;
        CacheSize best_size;

        for (S = 0; S <= (C - B); S++) {
            ifs.open(traces[i]);

            size = {C, B, S, K, V};
            ct = find_cache_type(size);
            stats = {};

            L1 = new Cache(size, ct, &stats);

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

            L1->compute_stats();

            // Determine if better than previous best
            if (stats.avg_access_time < aat_min) {
                aat_min = stats.avg_access_time;
                best_size = size;
            }

            delete L1;
            ifs.close();
        }

        std::cout << "Trace: " << traces[i] << std::endl;
        print_data(aat_min, best_size);
    }

    return 0;
}