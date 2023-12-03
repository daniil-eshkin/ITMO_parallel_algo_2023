#include <random>
#include "../include/parlay/parallel.h"
#include "../include/parlay/primitives.h"

int qsort_partition_number(parlay::sequence<int>::iterator l, parlay::sequence<int>::iterator r) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> rnd(0, r - l - 1);
    return *(l + rnd(gen));
}

void seq_qsort(parlay::sequence<int>::iterator l, parlay::sequence<int>::iterator r) {
    if (r - l <= 1) {
        return;
    }

    int x = qsort_partition_number(l, r);

    auto mid1 = l;
    for (auto i = l; i != r; i++) {
        if (*i < x) {
            std::swap(*i, *mid1);
            mid1++;
        }
    }
    auto mid2 = mid1;
    for (auto i = mid1; i != r; i++) {
        if (*i == x) {
            std::swap(*i, *mid2);
            mid2++;
        }
    }

    seq_qsort(l, mid1);
    seq_qsort(mid2, r);
}

void par_qsort(parlay::sequence<int>::iterator l, parlay::sequence<int>::iterator r, size_t block = 100) {
    if (r - l <= block) {
        seq_qsort(l, r);
        return;
    }

    int x = qsort_partition_number(l, r);

    auto left = parlay::filter(parlay::make_slice(l, r), [&](int a) { return a < x; });
    auto mid = parlay::filter(parlay::make_slice(l, r), [&](int a) { return a == x; });
    auto right = parlay::filter(parlay::make_slice(l, r), [&](int a) { return a > x; });

    parlay::parallel_do(
            [&]() {
                par_qsort(left.begin(), left.end(), block);
            },
            [&]() {
                par_qsort(right.begin(), right.end(), block);
            });

    parlay::parallel_for(0, left.size(), [&](size_t i) {
        *(l + i) = left[i];
    }, block);
    parlay::parallel_for(0, mid.size(), [&](size_t i) {
        *(l + left.size() + i) = mid[i];
    }, block);
    parlay::parallel_for(0, right.size(), [&](size_t i) {
        *(l + left.size() + mid.size() + i) = right[i];
    }, block);
}

template <typename F>
double bench(const F& f, const std::string& name = "Task") {
    std::cout << name << " started\n";

    const auto start{std::chrono::steady_clock::now()};

    f();

    const auto end{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds{end - start};

    std::cout << name << " finished. Time elapsed: " << elapsed_seconds.count() << "\n";

    return elapsed_seconds.count();
}

int main() {
    const int ARRAY_SIZE = 100000000;
    const int BLOCK_SIZE = 1000;
    const int PARALLEL_INVOCATIONS = 5;

    // Generating a random permutation
    auto perm = parlay::tabulate(ARRAY_SIZE, [](size_t i) {
        return static_cast<int>(i);
    });
    std::shuffle(perm.begin(), perm.end(), std::mt19937(std::random_device()()));

    // Sequential sort bench
    auto seq = perm;
    auto seq_time = bench([&]{
        seq_qsort(seq.begin(), seq.end());
    }, "Sequential sort");
    assert(parlay::is_sorted(seq));

    // Parallel sort bench
    auto par = perm;
    parlay::sequence<double> par_times(PARALLEL_INVOCATIONS);
    for (size_t i = 0; i < PARALLEL_INVOCATIONS; i++) {
        auto p = par;
        par_times[i] = bench([&]{
            par_qsort(p.begin(), p.end(), BLOCK_SIZE);
        }, "Parallel sort " + std::to_string(i + 1));
        assert(parlay::is_sorted(p));
    }



    auto par_time = parlay::reduce(par_times) / PARALLEL_INVOCATIONS;

    std::cout << "Average parallel sort time: " << par_time << std::endl;

    std::cout << "Seq / Par time ratio: " << seq_time / par_time << std::endl;

    return 0;
}
