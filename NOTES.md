## Notes

- Just store addresses in a set data structure.
    - Checking this set determines if hit or miss.
- Keep track of *space* utilized by adding to some int upon cache insertion/deletion.
    - Block size, tag size, valid bits, all need to be considered for size calc.
- Create a *cache* class. L1 and victim can be instances of this class.
    - Parameters are C, B, S (?)
    