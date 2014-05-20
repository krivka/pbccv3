#include <stdint.h>
#include <set>
#include <vector>
#include <algorithm>

#define stored_somewhere std::set<uint8_t> _stored_somewhere
#define stored_there std::vector<uint8_t> _stored_there
#define bankA std::vector<uint8_t> _bankA
#define bankB std::vector<uint8_t> _bankB

#ifdef TESTFILE
#   include TESTFILE
#else
#endif

#ifdef TESTRESULT
#   include TESTRESULT
#else
    std::vector<std::vector<uint8_t>> registers = {{},{}};
    std::vector<uint8_t> scratchpad_memory = { };
    bool carry = false;
    bool zero = false;
#endif //TESTRESULT

int main(void) {
    for (int i = 0; i < _stored_there.size(); i++) {
        if (scratchpad_memory.size() < i)
            break;
        if (_stored_there[i] != scratchpad_memory[i])
            return 1;
    }
    for (const uint8_t &i : _stored_somewhere) {
        if (i == 0)
            continue;
        int fail = 0;
        if (std::find(scratchpad_memory.begin(), scratchpad_memory.end(), i) == scratchpad_memory.end())
            fail++;
        for (std::vector<uint8_t> &v : registers) {
            if (std::find(v.begin(), v.end(), i) == v.end())
                fail++;
        }
        if (fail == 3)
            return 3;
    }
    for (int i = 0; i < _bankA.size() && i < registers[0].size(); i++) {
        if (_bankA[i] != registers[0][i])
            return 4;
    }
    for (int i = 0; i < _bankB.size() && i < registers[1].size(); i++) {
        if (_bankB[i] != registers[1][i])
            return 5;
    }
    return 0;
}