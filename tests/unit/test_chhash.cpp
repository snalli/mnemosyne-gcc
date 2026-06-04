#include <gtest/gtest.h>
#include <cstdint>

extern "C" {
#include "chhash.h"
}

class ChhashTest : public ::testing::Test {
protected:
    m_chhash_t *ht = nullptr;

    void SetUp() override {
        ASSERT_EQ(m_chhash_create(&ht, 64, 0), M_R_SUCCESS);
        ASSERT_NE(ht, nullptr);
    }

    void TearDown() override {
        if (ht) m_chhash_destroy(&ht);
    }
};

TEST_F(ChhashTest, InsertAndLookup) {
    EXPECT_EQ(m_chhash_add(ht, (m_chhash_key_t)1, (m_chhash_value_t)100), M_R_SUCCESS);
    m_chhash_value_t val;
    EXPECT_EQ(m_chhash_lookup(ht, (m_chhash_key_t)1, &val), M_R_SUCCESS);
    EXPECT_EQ((uintptr_t)val, 100U);
}

TEST_F(ChhashTest, LookupMissingKeyFails) {
    m_chhash_value_t val;
    EXPECT_NE(m_chhash_lookup(ht, (m_chhash_key_t)99, &val), M_R_SUCCESS);
}

TEST_F(ChhashTest, RemoveKey) {
    m_chhash_add(ht, (m_chhash_key_t)2, (m_chhash_value_t)200);
    m_chhash_value_t val;
    EXPECT_EQ(m_chhash_remove(ht, (m_chhash_key_t)2, &val), M_R_SUCCESS);
    EXPECT_EQ((uintptr_t)val, 200U);
    EXPECT_NE(m_chhash_lookup(ht, (m_chhash_key_t)2, &val), M_R_SUCCESS);
}

TEST_F(ChhashTest, DuplicateKeyReturnsExists) {
    m_chhash_add(ht, (m_chhash_key_t)1, (m_chhash_value_t)100);
    EXPECT_EQ(m_chhash_add(ht, (m_chhash_key_t)1, (m_chhash_value_t)999), M_R_EXISTS);
    m_chhash_value_t val;
    m_chhash_lookup(ht, (m_chhash_key_t)1, &val);
    EXPECT_EQ((uintptr_t)val, 100U); /* original unchanged */
}

TEST_F(ChhashTest, IteratorCoversAllKeys) {
    m_chhash_add(ht, (m_chhash_key_t)1, (m_chhash_value_t)100);
    m_chhash_add(ht, (m_chhash_key_t)2, (m_chhash_value_t)200);
    m_chhash_add(ht, (m_chhash_key_t)3, (m_chhash_value_t)300);

    m_chhash_iter_t iter;
    m_chhash_iter_init(ht, &iter);
    int count = 0;
    m_chhash_key_t k; m_chhash_value_t v;
    while (m_chhash_iter_next(&iter, &k, &v) == M_R_SUCCESS)
        count++;
    EXPECT_EQ(count, 3);
}

TEST_F(ChhashTest, DestroySucceeds) {
    EXPECT_EQ(m_chhash_destroy(&ht), M_R_SUCCESS);
    ht = nullptr; /* prevent TearDown double-free */
}
