#include <iostream>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <random>
#include <thread>
#include <mutex>
#include <atomic>

using std::chrono::nanoseconds;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
#define cpu 10

void randomNumbers(std::vector<int>& A, int seed) {
    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> distribution(0, A.size() - 1);
    for (int i = 0; i < A.size(); i++) {
        A[i] = distribution(generator);
    }
}

void noParallel(const std::vector<int>& A, int& counter, int& maxVal) {
    for (int i = 0; i < A.size(); i++) {
        if (A[i] % 5 == 0) {
            ++counter;
            maxVal = std::max(maxVal, A[i]);
        }
    }
}

void mutexFunc(const std::vector<int>& A, int start, int stop, int& counter, int& maxVal, std::mutex& mtx) {
    int localCounter = 0;
    int localMaxVal = 0;
    for (int i = start; i < stop; ++i) {
        if (A[i] % 5 == 0) {
            localCounter++;
            localMaxVal = std::max(A[i], localMaxVal);
        }
    }
    std::lock_guard<std::mutex> lock(mtx);
    counter += localCounter;
    maxVal = std::max(maxVal, localMaxVal);
}

void AtomicFunc(const std::vector<int>& A, int start, int stop, std::atomic<int>& counterAtomic, std::atomic<int>& maxAtomic) {
    int localCounter = 0;
    int localMaxVal = 0;
    for (int i = start; i < stop; ++i) {
        if (A[i] % 5 == 0) {
            localCounter++;
            localMaxVal = std::max(A[i], localMaxVal);
        }
    }
    int oldCounter = counterAtomic.load();
    while (!counterAtomic.compare_exchange_weak(oldCounter, localCounter)) {

    }

    int oldMax = maxAtomic.load();
    int newMax = std::max(oldMax, localMaxVal);
    while (!maxAtomic.compare_exchange_weak(oldMax, newMax)) {

    }
}

int main() {
    const std::vector<int> sizes = {10000, 100000, 2500000, 5000000, 100000000};
    const std::vector<int> threadCounts = {cpu/2, cpu, cpu*2, cpu*4, cpu*8, cpu*16};

    for (int size : sizes) {
        int seed = static_cast<int>(std::time(nullptr)) + size;
        std::vector<int> v(size);
        randomNumbers(v, seed);

        std::cout << "\n\n★ ★ ★ ★ ★  Matrix Size: " << size << " ★ ★ ★ ★ ★ ";

        for (int numThreads : threadCounts) {
            int elemPerThread = size / numThreads;
            int remainder = size % numThreads;

            auto startTime = high_resolution_clock::now();
            std::vector<std::thread> threads;

            std::mutex mtx;
            int sharedCounter = 0;
            int sharedMax = 0;

            // std::atomic<int> counterAtomic(0);
            // std::atomic<int> maxAtomic(0);

            for (int k = 0; k < numThreads; ++k) {
                int starting = k * elemPerThread + std::min(k, remainder);
                int stopping = starting + elemPerThread + (k < remainder ? 1 : 0);

                // threads.emplace_back(AtomicFunc, std::cref(v), starting, stopping,
                //                      std::ref(counterAtomic), std::ref(maxAtomic));

                threads.emplace_back(mutexFunc, std::cref(v), starting, stopping,
                                     std::ref(sharedCounter), std::ref(sharedMax), std::ref(mtx));
            }

            for (auto& t : threads) t.join();

            auto endTime = high_resolution_clock::now();
            double elapsed = duration_cast<nanoseconds>(endTime - startTime).count() * 1e-9;

            std::cout << "\nThreads: " << std::setw(3) << numThreads << ", Time: " << std::fixed << std::setprecision(6) << elapsed << " seconds";
            // std::cout << ", Count: " << counterAtomic << ", Max: " << maxAtomic;

            std::cout << ", Count: " << sharedCounter << ", Max: " << sharedMax;
        }
    }
    return 0;
}
