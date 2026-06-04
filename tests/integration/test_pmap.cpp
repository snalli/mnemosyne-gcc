/*
 * Integration test: persistent memory mapping (m_pmap).
 * Ported from tests/test/pmap/write_read.test.cxx (UnitTest++ → GTest).
 *
 * Requires: libmnemosyne.so, /dev/shm/psegments, mnemosyne.ini
 */
#include <gtest/gtest.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdlib.h>

extern "C" {
#include <mnemosyne.h>
}

#define PREGION_SIZE (4 * 1024 * 1024) /* 4 MB — enough to exercise mapping */

static void *pregion = nullptr;

static void write_pattern(void *base, size_t size, uint8_t seed)
{
    uint8_t *p = static_cast<uint8_t *>(base);
    for (size_t i = 0; i < size; i++)
        p[i] = static_cast<uint8_t>((seed + i) & 0xFF);
}

static bool check_pattern(void *base, size_t size, uint8_t seed)
{
    uint8_t *p = static_cast<uint8_t *>(base);
    for (size_t i = 0; i < size; i++)
        if (p[i] != static_cast<uint8_t>((seed + i) & 0xFF))
            return false;
    return true;
}

TEST(PMap, MapReturnsNonNull) {
    pregion = m_pmap(nullptr, PREGION_SIZE, PROT_READ | PROT_WRITE, 0);
    ASSERT_NE(pregion, nullptr);
}

TEST(PMap, WriteAndReadBack) {
    if (!pregion)
        pregion = m_pmap(nullptr, PREGION_SIZE, PROT_READ | PROT_WRITE, 0);
    ASSERT_NE(pregion, nullptr);

    write_pattern(pregion, PREGION_SIZE, 0xAB);
    EXPECT_TRUE(check_pattern(pregion, PREGION_SIZE, 0xAB));
}

TEST(PMap, OverwriteWithDifferentPattern) {
    if (!pregion)
        pregion = m_pmap(nullptr, PREGION_SIZE, PROT_READ | PROT_WRITE, 0);
    ASSERT_NE(pregion, nullptr);

    write_pattern(pregion, PREGION_SIZE, 0x42);
    EXPECT_TRUE(check_pattern(pregion, PREGION_SIZE, 0x42));
}
