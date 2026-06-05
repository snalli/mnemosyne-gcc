/*
 * Integration test: raw tornbit log read/write.
 * Ported from tests/test/rawlog/ (UnitTest++ → GTest).
 *
 * Requires: libmnemosyne.so, /dev/shm/psegments, mnemosyne.ini
 */
#include <gtest/gtest.h>
#include <vector>
#include <cstdlib>

extern "C" {
#include <mnemosyne.h>
#include <pcm.h>
#include <log.h>

/* The TORNBIT log type (used below) is registered with the log manager
 * by MTM's initialisation, which normally runs lazily on the first
 * transaction. This test allocates a log directly without ever entering
 * a transaction, so we force MTM init up front via this ABI entry point. */
int _ITM_initializeProcess(void);
}

/* The library is built with TMLOG_TYPE=TMLOG_TYPE_BASE, so MTM registers the
 * BASE persistent-log type (LF_TYPE_TM_BASE = 2) with the log manager. This
 * test exercises that same raw physical-log type. The enum lives in
 * tmlog_base.h via mtm_i.h, but that header chain pulls in C-only internals
 * that break C++ compilation, so we define the stable constant directly. */
#ifndef LF_TYPE_TM_BASE
#  define LF_TYPE_TM_BASE 2
#endif

#define SEQUENCE_END 0xDEADBEEFDEADBEEFULL

/* ---- minimal Sequence helper (was sequence.helper.h) ---- */

typedef struct m_rawlog_base_s m_rawlog_base_t;
struct m_rawlog_base_s { m_phlog_base_t phlog_base; };

static m_result_t rawlog_write(pcm_storeset_t *set, m_rawlog_base_t *log, pcm_word_t val)
{
    PHLOG_WRITE(base, set, &log->phlog_base, val);
    return M_R_SUCCESS;
}

static m_result_t rawlog_flush(pcm_storeset_t *set, m_rawlog_base_t *log)
{
    PHLOG_FLUSH(base, set, &log->phlog_base);
    return M_R_SUCCESS;
}

static std::vector<pcm_word_t> make_sequence(int len, int seed)
{
    std::vector<pcm_word_t> v;
    unsigned int s = static_cast<unsigned int>(seed);
    for (int i = 0; i < len; i++) {
        pcm_word_t val;
        do {
            val = ((pcm_word_t)rand_r(&s)) | (((pcm_word_t)rand_r(&s)) << 32);
        } while (val == SEQUENCE_END);
        v.push_back(val);
    }
    return v;
}

static void write_sequence(pcm_storeset_t *set, m_rawlog_base_t *log,
                           const std::vector<pcm_word_t> &seq)
{
    for (auto w : seq)
        rawlog_write(set, log, w);
    rawlog_write(set, log, SEQUENCE_END);
    rawlog_flush(set, log);
}

static std::vector<pcm_word_t> read_sequence(pcm_storeset_t *set,
                                              m_rawlog_base_t *log)
{
    std::vector<pcm_word_t> v;
    pcm_word_t val;
    while (m_phlog_base_read(&log->phlog_base, &val) == M_R_SUCCESS) {
        if (val == SEQUENCE_END) {
            m_phlog_base_next_chunk(&log->phlog_base);
            break;
        }
        v.push_back(val);
    }
    return v;
}

/* ---- GTest fixture ---- */

class RawlogTest : public ::testing::Test {
protected:
    pcm_storeset_t  *pcm_storeset = nullptr;
    m_log_dsc_t     *log_dsc      = nullptr;
    m_rawlog_base_t *rawlog       = nullptr;

    void SetUp() override {
        /* Register the BASE log type by initialising MTM. */
        ASSERT_EQ(_ITM_initializeProcess(), 0);
        pcm_storeset = pcm_storeset_get();
        ASSERT_NE(pcm_storeset, nullptr);
        ASSERT_EQ(m_logmgr_alloc_log(pcm_storeset, LF_TYPE_TM_BASE, 0, &log_dsc),
                  M_R_SUCCESS);
        rawlog = reinterpret_cast<m_rawlog_base_t *>(log_dsc->log);
        ASSERT_NE(rawlog, nullptr);
    }

    void TearDown() override {
        if (pcm_storeset) pcm_storeset_put();
    }
};

TEST_F(RawlogTest, WriteThenReadSingleSequence) {
    auto seq = make_sequence(8, 42);
    write_sequence(pcm_storeset, rawlog, seq);
    auto got = read_sequence(pcm_storeset, rawlog);
    EXPECT_EQ(seq, got);
}

TEST_F(RawlogTest, WriteMultipleSequencesReadInOrder) {
    const int N = 4;
    std::vector<std::vector<pcm_word_t>> seqs;
    for (int i = 0; i < N; i++) {
        int len = (rand() % 8) + 1;
        seqs.push_back(make_sequence(len, i * 7));
        write_sequence(pcm_storeset, rawlog, seqs.back());
    }
    for (int i = 0; i < N; i++) {
        auto got = read_sequence(pcm_storeset, rawlog);
        EXPECT_EQ(seqs[i], got) << "sequence " << i << " mismatch";
    }
}
