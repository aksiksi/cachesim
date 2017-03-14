# cachesim

A basic cache simulator written in C++ for ECE6100 at Georgia Tech.

## Build

Navigate to root directory and run `make`. 

## Run

To run the cache simualtor, just execute `./cachesim`. The simulator takes a number of optional parameters:

- C: total cache size
- B: block size
- S: blocks per set (if S=0, then direct-mapped)
- K: number of bytes per subblock
- V: victim cache blocks (if victim cache enabled!)
- i: path to input trace file (see below)

Example: `./cachesim -C 10 -B 4 -S 2 -K 2 -V 8`

Upon completing execution, the simulator will return a summary of cache statistics for the given trace file.

## Trace File Format

A list of cache accesses, one per line.

Format:
- Read: `r <address>`
- Write: `w <address>`

For questions, open an issue or catch me on Twitter ([aksiksi](https://twitter.com/aksiksi)).
