#include <stdint.h>
#include <set>
#include <vector>
#include <algorithm>

#ifdef TESTFILE
#   include TESTFILE
#else // TESTFILE
    // checks the whole memory image (registers too), if the values of this set are somewhere
    std::set<uint8_t> stored_somewhere;
    // checks the memory image for existence of every nonzero value in this array (on the same position)
    std::vector<uint8_t> stored_there;
#endif // TESTFILE

#ifdef TESTRESULT
#   include TESTRESULT
#else
    std::vector<std::vector<uint8_t>> registers = {{},{}};
    std::vector<uint8_t> scratchpad_memory = { };
    bool carry = false;
    bool zero = false;
#endif //TESTRESULT

int main(void) {
    for (int i = 0; i < stored_there.size(); i++) {
        if (scratchpad_memory.size() < i)
            break;
        if (stored_there[i] != scratchpad_memory[i])
            return 1;
    }
    for (const uint8_t &i : stored_somewhere) {
        if (std::find(scratchpad_memory.begin(), scratchpad_memory.end(), i) == scratchpad_memory.end())
            return 1;
        for (std::vector<uint8_t> &v : registers) {
            if (std::find(v.begin(), v.end(), i) == v.end())
                return 1;
        }
    }
    return 0;
}