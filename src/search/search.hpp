/**
 * Search Module - MCTS or Alpha-Beta
 * 
 * Implements selective search for human-like move selection.
 * Can use either MCTS (Monte Carlo Tree Search) or traditional alpha-beta.
 */

#ifndef SEARCH_HPP
#define SEARCH_HPP

#include <string>
#include <vector>

namespace Search {

struct SearchResult {
    std::string best_move;
    int score;  // centipawns
    int depth;
    long nodes;
    double time_ms;
    std::vector<std::string> pv;
};

// Initialize search
void initialize();

// Set search parameters
void set_threads(int n);
void set_hash_size(int mb);
void set_use_mcts(bool use_mcts);
void set_depth_limit(int depth);

// Search position
SearchResult search(const std::string& fen, int max_time_ms = 30000);

// Stop search
void stop();

// Check if search is running
bool is_searching();

} // namespace Search

#endif // SEARCH_HPP
