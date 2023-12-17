#include <vector>
#include <queue>
#include <atomic>
#include <cassert>
#include "../include/parlay/parallel.h"
#include "../include/parlay/primitives.h"

////////////////////////////////////////////////////////////////
// graph definition
////////////////////////////////////////////////////////////////

using node_t = uint32_t; // (x, y, z) <=> (n + 1)^2 * x + (n + 1) * y + z
using graph_t = std::vector<std::vector<node_t>>;
using distance = size_t;

inline constexpr node_t NO_NODE = std::numeric_limits<node_t>::max();

node_t from_coordinates(uint32_t n, uint32_t x, uint32_t y, uint32_t z) {
    return (n + 1) * (n + 1) * x + (n + 1) * y + z;
}

graph_t cube_graph(size_t n) {
    static constexpr std::tuple<int, int, int> d[] = {
            {0, 0, 1},
            {0, 0, -1},
            {0, 1, 0},
            {0, -1, 0},
            {1, 0, 0},
            {-1, 0, 0}
    };

    graph_t graph((n + 1) * (n + 1) * (n + 1));
    for (uint32_t x = 0; x <= n; x++) {
        for (uint32_t y = 0; y <= n; y++) {
            for (uint32_t z = 0; z <= n; z++) {
                node_t node = from_coordinates(n, x, y, z);
                for (auto &[dx, dy, dz]: d) {
                    if (!(x + 1 + dx != 0 && x + dx <= n
                        && y + 1 + dy != 0 && y + dy <= n
                        && z + 1 + dz != 0 && x + dz <= n)) {
                        continue;
                    }
                    node_t other_node = from_coordinates(n, x + dx, y + dy, z + dz);
                    graph[node].push_back(other_node);
                }
            }
        }
    }

    return graph;
}

////////////////////////////////////////////////////////////////
// sequential bfs
////////////////////////////////////////////////////////////////

void seq_bfs(const graph_t& graph, node_t start, std::vector<distance>& dist) {
    std::vector<bool> visited(graph.size(), false);

    dist[start] = 0;
    visited[start] = true;
    std::queue<node_t> q;
    q.push(start);

    while (!q.empty()) {
        node_t v = q.front();
        q.pop();

        for (auto& u: graph[v]) {
            if (!visited[u]) {
                visited[u] = true;
                dist[u] = dist[v] + 1;
                q.push(u);
            }
        }
    }
}

////////////////////////////////////////////////////////////////
// parallel bfs
////////////////////////////////////////////////////////////////

void par_bfs(const graph_t& graph, node_t start, std::vector<distance>& dist, size_t block = 100) {
    parlay::sequence<std::atomic_bool> visited(graph.size());

    dist[start] = 0;
    visited[start].store(true);
    parlay::sequence<node_t> frontier(1, start);

    while (!frontier.empty()) {
        auto pref_deg = parlay::tabulate(frontier.size(), [&](size_t i) {
            return graph[frontier[i]].size();
        }, block);
        auto sum = parlay::scan_inplace(pref_deg);
        parlay::sequence<node_t> new_frontier(sum, NO_NODE);

        parlay::parallel_for(0, frontier.size(), [&](size_t i) {
            auto v = frontier[i];
            for (size_t j = 0; j < graph[v].size(); j++) {
                auto u = graph[v][j];
                bool expected = false;
                if (visited[u].compare_exchange_strong(expected, true)) {
                    dist[u] = dist[v] + 1;
                    new_frontier[pref_deg[i] + j] = u;
                }
            }
        }, block);

        frontier = parlay::filter(new_frontier, [](node_t node){
            return node != NO_NODE;
        });
    }
}

void par_bfs_less_allocations(const graph_t& graph, node_t start, std::vector<distance>& dist, size_t block = 100) {
    static constexpr size_t MAX_FRONTIER = 200000;
    static constexpr size_t MAX_TMP_FRONTIER = 1200000;
    static constexpr size_t MAX_N = 130000000;

    static parlay::sequence<node_t> frontier(MAX_FRONTIER, NO_NODE);
    static parlay::sequence<node_t> tmp_frontier(MAX_TMP_FRONTIER, NO_NODE);
    static parlay::sequence<size_t> pref_flags(MAX_TMP_FRONTIER, 0);
    static parlay::sequence<size_t> pref_deg(MAX_N);
    static parlay::sequence<std::atomic_bool> visited(MAX_N);

    parlay::parallel_for(0, graph.size(), [&](size_t i) {
       visited[i].store(false);
    });

    dist[start] = 0;
    visited[start].store(true);
    frontier[0] = start;
    size_t frontier_size = 1;
    size_t tmp_frontier_size = 0;

    while (frontier_size != 0) {
        parlay::parallel_for(0, tmp_frontier_size, [&](size_t i) {
           tmp_frontier[i] = NO_NODE;
           pref_flags[i] = 0;
        }, block);

        parlay::parallel_for(0, frontier_size, [&](size_t i) {
           pref_deg[i] = graph[frontier[i]].size();
        }, block);
        tmp_frontier_size = parlay::scan_inplace(pref_deg.cut(0, frontier_size));

        parlay::parallel_for(0, frontier_size, [&](size_t i) {
            auto v = frontier[i];
            for (size_t j = 0; j < graph[v].size(); j++) {
                auto u = graph[v][j];
                bool expected = false;
                if (visited[u].compare_exchange_strong(expected, true)) {
                    dist[u] = dist[v] + 1;
                    tmp_frontier[pref_deg[i] + j] = u;
                    pref_flags[pref_deg[i] + j] = 1;
                }
            }
        }, block);

        frontier_size = parlay::pack_into(tmp_frontier.cut(0, tmp_frontier_size), pref_flags.cut(0, tmp_frontier_size), frontier);
    }
}

////////////////////////////////////////////////////////////////
// testing
////////////////////////////////////////////////////////////////

bool validate(const std::vector<distance>& dist, size_t n) {
    for (size_t x = 0; x <= n; x++) {
        for (size_t y = 0; y <= n; y++) {
            for (size_t z = 0; z <= n; z++) {
                if (dist[from_coordinates(n, x, y, z)] != x + y + z) {
                    return false;
                }
            }
        }
    }

    return true;
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
    constexpr size_t N = 500;
    constexpr size_t BLOCK_SIZE = 0;
    constexpr size_t PARALLEL_INVOCATIONS = 5;

    auto graph = cube_graph(N);

    std::vector<distance> dist(graph.size(), 0);

    auto seq_time = bench([&] {
        seq_bfs(graph, from_coordinates(N, 0, 0, 0), dist);
    }, "Sequential BFS");
    assert(validate(dist, N));

    parlay::sequence<double> par_times(PARALLEL_INVOCATIONS);
    for (size_t i = 0; i < PARALLEL_INVOCATIONS; i++) {
        dist.assign(graph.size(), 0);
        par_times[i] = bench([&]{
            par_bfs(graph, from_coordinates(N, 0, 0, 0), dist, BLOCK_SIZE);
        }, "Parallel BFS " + std::to_string(i + 1));
        assert(validate(dist, N));
    }

    dist.assign(graph.size(), 0);
    bench([&]{
        par_bfs_less_allocations(graph, from_coordinates(N, 0, 0, 0), dist, BLOCK_SIZE);
    }, "Parallel BFS less allocations warmup");

    parlay::sequence<double> par_less_alloc_times(PARALLEL_INVOCATIONS);
    for (size_t i = 0; i < PARALLEL_INVOCATIONS; i++) {
        dist.assign(graph.size(), 0);
        par_less_alloc_times[i] = bench([&]{
            par_bfs_less_allocations(graph, from_coordinates(N, 0, 0, 0), dist, BLOCK_SIZE);
        }, "Parallel BFS less allocations " + std::to_string(i + 1));
        assert(validate(dist, N));
    }

    auto par_time = parlay::reduce(par_times) / PARALLEL_INVOCATIONS;
    auto par_less_alloc_time = parlay::reduce(par_less_alloc_times) / PARALLEL_INVOCATIONS;

    std::cout << "Average parallel BFS time: " << par_time << std::endl;
    std::cout << "Seq / Par time ratio: " << seq_time / par_time << std::endl;

    std::cout << "Average parallel BFS less allocations time: " << par_less_alloc_time << std::endl;
    std::cout << "Seq / ParLA time ratio: " << seq_time / par_less_alloc_time << std::endl;

    return 0;
}
