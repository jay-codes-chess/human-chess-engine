/**
 * Search Module - Stub Implementation
 * 
 * MCTS or Alpha-Beta search placeholder.
 */

#include "search.hpp"
#include <chrono>
#include <thread>

namespace Search {

static bool searching = false;
static int threads = 1;
static int hash_size = 64;
static bool use_mcts = true;

void initialize() {
    // Initialize search structures
}

void set_threads(int n) {
    threads = n;
}

void set_hash_size(int mb) {
    hash_size = mb;
}

void set_use_mcts(bool use) {
    use_mcts = use;
}

void set_depth_limit(int depth) {
    // Set maximum depth limit
}

SearchResult search(const std::string& fen, int max_time_ms) {
    SearchResult result;
    result.score = 0;
    result.depth = 10;
    result.nodes = 0;
    result.time_ms = max_time_ms;
    
    // Stub: Return null move for now
    result.best_move = "";
    
    return result;
}

void stop() {
    searching = false;
}

bool is_searching() {
    return searching;
}

} // namespace Search
