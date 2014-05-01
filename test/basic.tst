
// checks the whole memory image (registers too), if the values of this set are somewhere
std::set<uint8_t> stored_somewhere = {
    0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0
};

// checks the memory image for existence of every nonzero value in this array (on the same position)
std::vector<uint8_t> stored_there;