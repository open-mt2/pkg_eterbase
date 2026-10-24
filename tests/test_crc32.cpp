#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include "CRC32.h"

int main()
{
    printf("[TEST] Starting CRC32 tests...\n");

    const char* str1 = "TestStringForCRC32";
    const char* str2 = "TestStringForCRC32";
    const char* str3 = "DifferentString";

    DWORD crc1 = GetCRC32(str1, strlen(str1));
    DWORD crc2 = GetCRC32(str2, strlen(str2));
    DWORD crc3 = GetCRC32(str3, strlen(str3));

    // Consistency
    assert(crc1 != 0);
    assert(crc1 == crc2);
    assert(crc1 != crc3);
    printf("  [PASS] Consistency: crc1 == crc2 (0x%08X), crc1 != crc3 (0x%08X)\n", crc1, crc3);

    // Case CRC check
    DWORD case1 = GetCaseCRC32(str1, strlen(str1));
    assert(case1 == crc1);
    printf("  [PASS] GetCaseCRC32 matches GetCRC32 (0x%08X)\n", case1);

    // Differentiation between cases
    const char* upper = "METIN2";
    const char* lower = "metin2";
    DWORD crcUpper = GetCRC32(upper, strlen(upper));
    DWORD crcLower = GetCRC32(lower, strlen(lower));
    assert(crcUpper != crcLower);
    printf("  [PASS] Case difference verified: 'METIN2' (0x%08X) != 'metin2' (0x%08X)\n", crcUpper, crcLower);

    printf("[TEST] CRC32 tests PASSED successfully.\n");
    return 0;
}
