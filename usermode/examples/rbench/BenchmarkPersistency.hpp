#ifndef _BENCHMARK_PERSISTENCY_H_
#define _BENCHMARK_PERSISTENCY_H_

#include <atomic>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <typeinfo>

template<typename PE>
struct PersistentArray;

// This is defined in SConscript for builds done inside the mnemosyne-gcc repository
#include "Mnemosyne.hpp"


/* FAILURE: If the arraySize is 512*1024 then we get an assert on mtm_pwb_restart_transaction() */
static const int arraySize=512;


template<typename PE, typename W>
struct PersistentArrayWrapper {
    W* counters[arraySize];
    PersistentArrayWrapper(PE& pe){
        for (int i = 0; i < arraySize; i++){
            counters[i] = pe.template alloc<W>(0);
        }
    }
    ~PersistentArrayWrapper(){}
};

template<typename PE>
struct PersistentArrayInt {
    int64_t counters[arraySize];
    PersistentArrayInt(PE& pe) {
        for (int i = 0; i < arraySize; i++) counters[i] = 0;
    }
};


struct WrapperM {
    int64_t val;
    WrapperM(int64_t val): val {val} { }
};

template<>
struct PersistentArray<mnemosyne::Mnemosyne> {
    WrapperM* counters[arraySize];
    PersistentArray(mnemosyne::Mnemosyne& pe) {
        PTx {
            for (int i = 0; i < arraySize; i++){
                counters[i] = pe.alloc<WrapperM>(0);
            }
        }
    }
    ~PersistentArray() {
    }
};

using namespace std;
using namespace chrono;


/**
 * This is a micro-benchmark
 */
class BenchmarkPersistency {

private:

    // Performance benchmark constants
    static const long long kNumPairsWarmup =     1000000LL;     // Each threads does 1M iterations as warmup

    // Contants for Ping-Pong performance benchmark
    static const int kPingPongBatch = 1000;            // Each thread starts by injecting 1k items in the queue

    static const long long NSEC_IN_SEC = 1000000000LL;

    int numThreads;

    static const int PIDX_ARRAY = 0;
    static const int PIDX_QUEUE = 1;
    static const int PIDX_INT_ARRAY = 2;

public:
    struct UserData  {
        long long seq;
        int tid;
        UserData(long long lseq, int ltid) {
            this->seq = lseq;
            this->tid = ltid;
        }
        UserData() {
            this->seq = -2;
            this->tid = -2;
        }
        UserData(const UserData &other) : seq(other.seq), tid(other.tid) { }

        bool operator < (const UserData& other) const {
            return seq < other.seq;
        }
    };

    BenchmarkPersistency(int numThreads) {
        this->numThreads = numThreads;
    }


    /**
     * enqueue-dequeue pairs: in each iteration a thread executes an enqueue followed by a dequeue;
     * the benchmark executes 10^8 pairs partitioned evenly among all threads;
     */
    template<typename PE, typename Q>
    void enqDeqBenchmark(const long numPairs, const int numRuns) {
        PE pe {};   // Initialize the Persistency Engine
        Q* queue = nullptr;
        nanoseconds deltas[numThreads][numRuns];
        atomic<bool> startFlag = { false };
        auto enqdeq_lambda = [this,&startFlag,&numPairs,&queue,&pe](nanoseconds *delta, const int tid) {
            pe.transaction([this,&queue,&pe] () {
            	queue = pe.template get_object<Q>(PIDX_QUEUE);
                if (queue == nullptr) {
                	queue = pe.template alloc<Q>(pe);
                    pe.put_object(PIDX_QUEUE, queue);
                }
            });
            UserData ud(0,0);
            while (!startFlag.load()) {} // Spin until the startFlag is set
            // Warmup phase
            for (long long iter = 0; iter < kNumPairsWarmup/numThreads; iter++) {
                queue->enqueue(&ud, tid);
                if (queue->dequeue(tid) == nullptr) cout << "Error at warmup dequeueing iter=" << iter << "\n";
            }
            // Measurement phase
            auto startBeats = steady_clock::now();
            for (long long iter = 0; iter < numPairs/numThreads; iter++) {
                queue->enqueue(&ud, tid);
                if (queue->dequeue(tid) == nullptr) cout << "Error at measurement dequeueing iter=" << iter << "\n";
            }
            auto stopBeats = steady_clock::now();
            *delta = stopBeats - startBeats;
        };
        for (int irun = 0; irun < numRuns; irun++) {
            pe.transaction([this,&pe,&queue] () { queue = pe.template alloc<Q>(pe, numThreads); });
            if (irun == 0) cout << "##### " << queue->className() << " #####  \n";
            thread enqdeqThreads[numThreads];
            for (int tid = 0; tid < numThreads; tid++) enqdeqThreads[tid] = thread(enqdeq_lambda, &deltas[tid][irun], tid);
            startFlag.store(true);
            // Sleep for 2 seconds just to let the threads see the startFlag
            this_thread::sleep_for(2s);
            for (int tid = 0; tid < numThreads; tid++) enqdeqThreads[tid].join();
            startFlag.store(false);
            pe.transaction([this,&pe,&queue] () { pe.template free<Q>(queue); });
        }
        // Sum up all the time deltas of all threads so we can find the median run
        vector<nanoseconds> agg(numRuns);
        for (int irun = 0; irun < numRuns; irun++) {
            agg[irun] = 0ns;
            for (int tid = 0; tid < numThreads; tid++) {
                agg[irun] += deltas[tid][irun];
            }
        }
        // Compute the median. numRuns should be an odd number
        sort(agg.begin(),agg.end());
        auto median = agg[numRuns/2].count()/numThreads; // Normalize back to per-thread time (mean of time for this run)
        cout << "Total Ops/sec = " << numPairs*2*NSEC_IN_SEC/median << "\n";
    }


    /*
     * Runs a Persistency Engine over an array of integers with (hard-coded) size of 1024
     */
    template<typename PE, typename W>
    long long arrayBenchmark(const seconds testLengthSeconds, const long numWordsPerTransaction, const int numRuns) {
        long long ops[numRuns];
        long long lengthSec[numRuns];
        atomic<bool> startFlag = { false };
        atomic<bool> quit = { false };
        PE pe {};
        auto func = [this,&startFlag,&quit,&numWordsPerTransaction,&pe](long long *ops, const int tid) {
            uint64_t seed = tid+1234567890123456781ULL;
            // Create the array of integers and initialize it, unless it is already there
            PersistentArray<PE>* parray;
            pe.transaction([this,&parray,&pe] () {
                parray = pe.template get_object<PersistentArray<PE>>(PIDX_ARRAY);
                if (parray == nullptr) {
                    parray = pe.template alloc<PersistentArray<PE>>(pe);
                    pe.put_object(PIDX_ARRAY, parray);
                } else {
                    // Check that the array is consistent
                    int64_t sum = 0;
                    for (int i = 0; i < arraySize; i++) {
                        sum += parray->counters[i].pload()->val;
                    }
                    assert(sum == 0);
                }
            });
            // Spin until the startFlag is set
            while (!startFlag.load()) {}
            // Do transactions until the quit flag is set
            long long tcount = 0;
            while (!quit.load()) {
                pe.transaction([this,&seed,&parray,&numWordsPerTransaction,&pe] () {
                    for (int i = 0; i < numWordsPerTransaction/2; i++) {
                        seed = randomLong(seed);
                        W* old =parray->counters[seed%arraySize];
                        W* next =pe.template alloc<W>(old->val-1);
                        parray->counters[seed%arraySize]=next;  // Subtract from random places
                        pe.free(old);
                    }
                    for (int i = 0; i < numWordsPerTransaction/2; i++) {
                        seed = randomLong(seed);
                        W* old =parray->counters[seed%arraySize];
                        W* next =pe.template alloc<W>(old->val+1);
                        parray->counters[seed%arraySize]=next;  // Add from random places
                        pe.free(old);
                    }
                } );
                tcount++;
            }
            *ops = tcount;
            pe.transaction([&pe,&parray] () {
                for(int i=0;i<arraySize;i++){
                    pe.free(parray->counters[i].pload());
                }
                // This free() causes Mnemosyne to crash, don't know why (yet)
                pe.free(parray);
                pe.template put_object<PersistentArray<PE>>(0, nullptr);

            });
        };
        for (int irun = 0; irun < numRuns; irun++) {
            if (irun == 0) cout << "##### " << pe.className() << " #####  \n";
            thread enqdeqThreads[numThreads];
            for (int tid = 0; tid < numThreads; tid++) enqdeqThreads[tid] = thread(func, &ops[irun], tid);
            auto startBeats = steady_clock::now();
            startFlag.store(true);
            // Sleep for 20 seconds
            this_thread::sleep_for(testLengthSeconds);
            quit.store(true);
            auto stopBeats = steady_clock::now();
            for (int tid = 0; tid < numThreads; tid++) enqdeqThreads[tid].join();
            lengthSec[irun] = (stopBeats-startBeats).count();
            startFlag.store(false);
            quit.store(false);
        }
        // Accounting
        vector<long long> agg(numRuns);
        for (int irun = 0; irun < numRuns; irun++) {
            agg[irun] += ops[irun]*1000000000LL/lengthSec[irun];
        }
        // Compute the median. numRuns should be an odd number
        sort(agg.begin(),agg.end());
        auto maxops = agg[numRuns-1];
        auto minops = agg[0];
        auto medianops = agg[numRuns/2];
        auto delta = (long)(100.*(maxops-minops) / ((double)medianops));
        // Printed value is the median of the number of ops per second that all threads were able to accomplish (on average)
        std::cout << "Transactions/sec = " << medianops << "     delta = " << delta << "%   min = " << minops << "   max = " << maxops << "\n";
        return medianops;
    }


    /*
     * Runs a Persistency Engine over an array of Wrappers with (hard-coded) size of 512
     */
    template<typename PE, typename W>
    long long arrayBenchmark_rw(const seconds testLengthSeconds, const long numWordsPerTransaction, const int numRuns) {
        long long ops[numRuns];
        long long lengthSec[numRuns];
        atomic<bool> startFlag = { false };
        atomic<bool> quit = { false };
        // Initialize the persistency engine
        PE pe {};
        // Create the array of integers and initialize it, unless it is already there
        PersistentArray<PE>* parray;
        pe.write_transaction([this,&parray,&pe] () {
            parray = pe.template get_object<PersistentArray<PE>>(PIDX_ARRAY);
            if (parray == nullptr) {
                parray = pe.template alloc<PersistentArray<PE>>(pe);
                pe.put_object(PIDX_ARRAY, parray);
            } else {
                // Check that the array is consistent
                int64_t sum = 0;
                for (int i = 0; i < arraySize; i++) {
                    sum += parray->counters[i].pload()->val;
                }
                assert(sum == 0);
                //pe.compare_main_and_back();
            }
        });
        auto func = [this,&startFlag,&quit,&numWordsPerTransaction,&pe,&parray](long long *ops, const int tid) {
            uint64_t seed = tid+1234567890123456781ULL;
            // Spin until the startFlag is set
            while (!startFlag.load()) {}
            // Do transactions until the quit flag is set
            long long tcount = 0;
            while (!quit.load()) {
                pe.write_transaction([this,&seed,&parray,&numWordsPerTransaction,&pe] () {
                    for (int i = 0; i < numWordsPerTransaction/2; i++) {
                        seed = randomLong(seed);
                        W* old =parray->counters[seed%arraySize];
                        W* next =pe.template alloc<W>(old->val-1);
                        parray->counters[seed%arraySize]=next;  // Subtract from random places
                        pe.free(old);
                    }
                    for (int i = 0; i < numWordsPerTransaction/2; i++) {
                        seed = randomLong(seed);
                        W* old =parray->counters[seed%arraySize];
                        W* next =pe.template alloc<W>(old->val+1);
                        parray->counters[seed%arraySize]=next;  // Add from random places
                        pe.free(old);
                    }
                } );

                pe.read_transaction([this,&seed,&parray,&numWordsPerTransaction,&pe] () {
                    // Check that the array is consistent
                    int64_t sum = 0;
                    for (int i = 0; i < arraySize; i++) {
                        sum += parray->counters[i].pload()->val;
                    }
                    assert(sum == 0);
                } );
                tcount=tcount+2;
            }
            *ops = tcount;
        };
        for (int irun = 0; irun < numRuns; irun++) {
            if (irun == 0) cout << "##### " << pe.className() << " #####  \n";
            thread enqdeqThreads[numThreads];
            for (int tid = 0; tid < numThreads; tid++) enqdeqThreads[tid] = thread(func, &ops[irun], tid);
            auto startBeats = steady_clock::now();
            startFlag.store(true);
            // Sleep for 20 seconds
            this_thread::sleep_for(testLengthSeconds);
            quit.store(true);
            auto stopBeats = steady_clock::now();
            for (int tid = 0; tid < numThreads; tid++) enqdeqThreads[tid].join();
            lengthSec[irun] = (stopBeats-startBeats).count();
            startFlag.store(false);
            quit.store(false);
        }

        pe.write_transaction([&pe,&parray] () {
            for(int i=0;i<arraySize;i++){
                pe.free(parray->counters[i].pload());
            }
            // This free() causes Mnemosyne to crash, don't know why (yet)
            pe.free(parray);
            pe.template put_object<PersistentArray<PE>>(PIDX_ARRAY, nullptr);

        });
        // Accounting
        vector<long long> agg(numRuns);
        for (int irun = 0; irun < numRuns; irun++) {
            agg[irun] += ops[irun]*1000000000LL/lengthSec[irun];
        }
        // Compute the median. numRuns should be an odd number
        sort(agg.begin(),agg.end());
        auto maxops = agg[numRuns-1];
        auto minops = agg[0];
        auto medianops = agg[numRuns/2];
        auto delta = (long)(100.*(maxops-minops) / ((double)medianops));
        // Printed value is the median of the number of ops per second that all threads were able to accomplish (on average)
        std::cout << "Transactions/sec = " << medianops << "     delta = " << delta << "%   min = " << minops << "   max = " << maxops << "\n";
        return medianops;
    }


    /*
     * Runs a Persistency Engine over an array of integers with (hard-coded) size of 512
     */
    template<typename PE>
    long long integerArrayBenchmark_rw(const seconds testLengthSeconds, const long numWordsPerTransaction, const int numRuns) {
        long long ops[numRuns];
        long long lengthSec[numRuns];
        atomic<bool> startFlag = { false };
        atomic<bool> quit = { false };
        // Initialize the persistency engine
        PE pe {};
        // Create the array of integers and initialize it, unless it is already there
        PersistentArrayInt<PE>* parray;
        pe.write_transaction([this,&parray,&pe] () {
            parray = pe.template get_object<PersistentArrayInt<PE>>(PIDX_INT_ARRAY);
            if (parray == nullptr) {
                parray = pe.template alloc<PersistentArrayInt<PE>>(pe);
                pe.put_object(PIDX_INT_ARRAY, parray);
            } else {
                // Check that the array is consistent
                int sum = 0;
                for (int i = 0; i < arraySize; i++) {
                    sum += parray->counters[i];
                }
                assert(sum == 0);
            }
        });
        auto func = [this,&startFlag,&quit,&numWordsPerTransaction,&pe,&parray](long long *ops, const int tid) {
            uint64_t seed = tid+1234567890123456781ULL;
            // Spin until the startFlag is set
            while (!startFlag.load()) {}
            // Do transactions until the quit flag is set
            long long tcount = 0;
            while (!quit.load()) {
                pe.write_transaction([this,&seed,&parray,&numWordsPerTransaction,&pe] () {
                    for (int i = 0; i < numWordsPerTransaction/2; i++) {
                        seed = randomLong(seed);
                        parray->counters[seed%arraySize] = parray->counters[seed%arraySize]-1;
                    }
                    for (int i = 0; i < numWordsPerTransaction/2; i++) {
                        seed = randomLong(seed);
                        parray->counters[seed%arraySize] = parray->counters[seed%arraySize]+1;
                    }
                } );
/*
                pe.read_transaction([this,&seed,&parray,&numWordsPerTransaction,&pe] () {
                    // Check that the array is consistent
                    int sum = 0;
                    for (int i = 0; i < arraySize; i++) {
                        sum += parray->counters[i];
                    }
                    assert(sum == 0);
                } );
*/
                tcount=tcount+2;
            }
            *ops = tcount;
        };
        for (int irun = 0; irun < numRuns; irun++) {
            if (irun == 0) cout << "##### " << pe.className() << " #####  \n";
            thread enqdeqThreads[numThreads];
            for (int tid = 0; tid < numThreads; tid++) enqdeqThreads[tid] = thread(func, &ops[irun], tid);
            auto startBeats = steady_clock::now();
            startFlag.store(true);
            // Sleep for 20 seconds
            this_thread::sleep_for(testLengthSeconds);
            quit.store(true);
            auto stopBeats = steady_clock::now();
            for (int tid = 0; tid < numThreads; tid++) enqdeqThreads[tid].join();
            lengthSec[irun] = (stopBeats-startBeats).count();
            startFlag.store(false);
            quit.store(false);
        }

        pe.write_transaction([&pe,&parray] () {
            // This free() causes Mnemosyne to crash, don't know why (yet)
            pe.pfree(parray);
            pe.template put_object<PersistentArrayInt<PE>>(PIDX_INT_ARRAY, nullptr);

        });
        // Accounting
        vector<long long> agg(numRuns);
        for (int irun = 0; irun < numRuns; irun++) {
            agg[irun] += ops[irun]*1000000000LL/lengthSec[irun];
        }
        // Compute the median. numRuns should be an odd number
        sort(agg.begin(),agg.end());
        auto maxops = agg[numRuns-1];
        auto minops = agg[0];
        auto medianops = agg[numRuns/2];
        auto delta = (long)(100.*(maxops-minops) / ((double)medianops));
        // Printed value is the median of the number of ops per second that all threads were able to accomplish (on average)
        std::cout << "Transactions/sec = " << medianops << "     delta = " << delta << "%   min = " << minops << "   max = " << maxops << "\n";
        return medianops;
    }

    /**
     * An imprecise but fast random number generator
     */
    uint64_t randomLong(uint64_t x) {
        x ^= x >> 12; // a
        x ^= x << 25; // b
        x ^= x >> 27; // c
        return x * 2685821657736338717LL;
    }

public:

    static void allThroughputTests() {
        vector<int> threadList = { 16 };
        vector<long> wordsPerTransList = { /*4, 8, 16, 32, 64, 128,*/ 256 };
        const seconds testLength = 20s;   // 20s for the paper
        const int numRuns = 1;            // 5 runs for the paper



/*
        // Array Benchmarks single threaded
		for (int nWords : wordsPerTransList) {
			BenchmarkPersistency bench(1);
			std::cout << "\n----- Array Benchmark   length=" << testLength.count() << "s   arraySize=" << arraySize << "   words/transaction=" << nWords << " -----\n";
			bench.arrayBenchmark<mnemosyne::Mnemosyne, WrapperM>(testLength, nWords, numRuns);
		}*/

/*
		// Wrapper Array Benchmarks multi-threaded
        for (int nThreads : threadList) {
            for (int nWords : wordsPerTransList) {
                BenchmarkPersistency bench(nThreads);
                std::cout << "\n----- Wrapper Array Benchmark (multi-threaded)   numThreads=" << nThreads << "   length=" << testLength.count() << "s   arraySize=" << arraySize << "   words/transaction=" << nWords << " -----\n";
                bench.arrayBenchmark_rw<mnemosyne::Mnemosyne, WrapperM>(testLength, nWords, numRuns);
            }
        }
*/

        /* FAILURE: With 16 threads, 256 words per transaction, this one gives core dump on mtm_local_rollback() */
        // Integer Array Benchmarks multi-threaded
        for (int nThreads : threadList) {
            for (int nWords : wordsPerTransList) {
                BenchmarkPersistency bench(nThreads);
                std::cout << "\n----- Integer Array Benchmark (multi-threaded)   numThreads=" << nThreads << "   length=" << testLength.count() << "s   arraySize=" << arraySize << "   words/transaction=" << nWords << " -----\n";
                bench.integerArrayBenchmark_rw<mnemosyne::Mnemosyne>(testLength, nWords, numRuns);
            }
        }

    }
};

#endif
