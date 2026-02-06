/**
 * Search Module - Alpha-Beta with Human-Like Selectivity
 * 
 * Implements selective search that mirrors human thinking:
 * - Uses candidate move generation (Kotov method)
 * - Selective depth based on position complexity
 * - Time-aware thinking patterns
 * 
 * Reference: Alexander Kotov "Play Like a Grandmaster"
 */

#include "search.hpp"
#include "../utils/board.hpp"
#include "../evaluation/evaluation.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <random>
#include <limits>

namespace Search {

// Search state
static bool stop_search = false;
static int search_depth = 10;
static int max_depth = 20;
static long nodes_searched = 0;
static int best_move_found = 0;
static int search_score = 0;

// Time management
static std::chrono::steady_clock::time_point start_time;
static int max_time_ms = 30000;
static int optimal_time_ms = 3000;
static int panic_time_ms = 5000;

// Transposition table
struct TTEntry {
    uint64_t hash;
    int depth;
    int score;
    int move;
    uint8_t flag;  // 0=empty, 1=alpha, 2=beta, 3=exact
};

static const int TT_SIZE = 1 << 20;  // 1M entries
static TTEntry* transposition_table;

// Initialize search
void initialize() {
    transposition_table = new TTEntry[TT_SIZE];
    for (int i = 0; i < TT_SIZE; i++) {
        transposition_table[i].hash = 0;
        transposition_table[i].depth = 0;
        transposition_table[i].score = 0;
        transposition_table[i].move = 0;
        transposition_table[i].flag = 0;
    }
}

// Get TT index
int tt_index(uint64_t hash) {
    return hash & (TT_SIZE - 1);
}

// Store in transposition table
void tt_store(uint64_t hash, int depth, int score, int move, int flag) {
    int idx = tt_index(hash);
    // Simple replacement strategy
    transposition_table[idx].hash = hash;
    transposition_table[idx].depth = depth;
    transposition_table[idx].score = score;
    transposition_table[idx].move = move;
    transposition_table[idx].flag = flag;
}

// Probe transposition table
bool tt_probe(uint64_t hash, int depth, int& score, int& move) {
    int idx = tt_index(hash);
    if (transposition_table[idx].hash == hash && 
        transposition_table[idx].depth >= depth) {
        score = transposition_table[idx].score;
        move = transposition_table[idx].move;
        return true;
    }
    return false;
}

// Get current time
int get_elapsed_ms() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
}

// Check if we should stop
bool should_stop() {
    if (stop_search) return true;
    
    int elapsed = get_elapsed_ms();
    
    // Time pressure checks
    if (elapsed > max_time_ms) return true;
    if (elapsed > panic_time_ms && search_depth > 3) return true;
    
    return false;
}

// Evaluate position (using our Silman-based evaluation)
int evaluate_position(const Board& board, int color) {
    std::string fen = board.get_fen();
    int score = Evaluation::evaluate(fen);
    return (color == WHITE) ? score : -score;
}

// Make move on board
Board make_move(const Board& board, int move) {
    Board new_board = board;
    int from = move >> 6;
    int to = move & 63;
    int piece = board.piece_at(from);
    int color = board.color_at(from);
    int captured = board.piece_at(to);
    
    // Move piece
    new_board.remove_piece(from);
    new_board.add_piece(to, piece, color);
    
    // Handle captures
    if (captured != NO_PIECE && captured != KING) {
        // Capture - nothing else needed, piece removed above
    }
    
    // Handle pawn promotion (simplified - always to queen)
    if (piece == PAWN) {
        int rank = Bitboards::rank_of(to);
        if ((color == WHITE && rank == 7) || (color == BLACK && rank == 0)) {
            new_board.remove_piece(to);
            new_board.add_piece(to, QUEEN, color);
        }
        
        // Handle en passant
        if (board.en_passant_square == to) {
            int ep_rank = (color == WHITE) ? 4 : 3;
            int ep_sq = (to & 7) + ep_rank * 8;
            new_board.remove_piece(ep_sq);
        }
        
        // Set en passant square
        if (std::abs(to - from) == 16) {
            new_board.en_passant_square = (from + to) / 2;
        } else {
            new_board.en_passant_square = -1;
        }
    } else {
        new_board.en_passant_square = -1;
    }
    
    // Update castling
    if (piece == KING) {
        if (color == WHITE) {
            new_board.castling[WHITE][0] = false;
            new_board.castling[WHITE][1] = false;
        } else {
            new_board.castling[BLACK][0] = false;
            new_board.castling[BLACK][1] = false;
        }
    }
    if (piece == ROOK) {
        if (color == WHITE) {
            if (from == 0) new_board.castling[WHITE][1] = false;
            if (from == 7) new_board.castling[WHITE][0] = false;
        } else {
            if (from == 56) new_board.castling[BLACK][1] = false;
            if (from == 63) new_board.castling[BLACK][0] = false;
        }
    }
    
    // Switch side to move
    new_board.side_to_move = 1 - color;
    new_board.compute_hash();
    
    return new_board;
}

// Generate pseudo-legal moves
std::vector<int> generate_moves(const Board& board) {
    return board.generate_moves();
}

// Check if move is legal (king not in check)
bool is_legal(const Board& board, int move) {
    Board test_board = make_move(board, move);
    int color = 1 - board.side_to_move;
    return !test_board.is_in_check(color);
}

// Order moves for better alpha-beta
void order_moves(std::vector<int>& moves, const Board& board, int tt_move) {
    // Killer move heuristic - TT move first
    std::sort(moves.begin(), moves.end(), [&](int a, int b) {
        if (a == tt_move) return true;
        if (b == tt_move) return false;
        
        // Capture ordering (MVV-LVA simplified)
        int capture_a = board.piece_at(a & 63);
        int capture_b = board.piece_at(b & 63);
        if (capture_a != NO_PIECE && board.color_at(a & 63) != board.color_at(a & 63)) {
            capture_a = 5;  // Promote to capture
        }
        return capture_a > capture_b;
    });
}

// Generate candidate moves (Kotov method)
std::vector<int> generate_candidates(const Board& board) {
    auto moves = generate_moves(board);
    
    // Filter to reasonable moves (reduce candidates like humans do)
    std::vector<int> candidates;
    
    for (int move : moves) {
        if (!is_legal(board, move)) continue;
        
        int from = move >> 6;
        int to = move & 63;
        int piece = board.piece_at(from);
        int captured = board.piece_at(to);
        int color = board.color_at(from);
        
        // Always include:
        // 1. Captures
        // 2. Checks
        // 3. King moves
        // 4. Pawn advances
        // 5. Some reasonable piece moves
        
        bool include = false;
        
        // Captures
        if (captured != NO_PIECE) include = true;
        
        // King moves
        if (piece == KING) include = true;
        
        // Pawn moves
        if (piece == PAWN) {
            int push = to - from;
            if (push == 8 || push == -8 || push == 16 || push == -16) include = true;
            if (push == 7 || push == 9 || push == -7 || push == -9) include = true;
        }
        
        // Check if move gives check
        Board test = make_move(board, move);
        if (test.is_in_check(1 - color)) include = true;
        
        // Include some random piece moves (representing candidate moves)
        if (!include && piece != PAWN && piece != KING) {
            static std::mt19937 rng(12345);
            std::uniform_int_distribution<int> dist(0, 100);
            if (dist(rng) < 30) include = true;  // 30% of piece moves
        }
        
        if (include) candidates.push_back(move);
    }
    
    return candidates;
}

// Alpha-beta search
int alpha_beta(Board& board, int depth, int alpha, int beta, int color) {
    nodes_searched++;
    
    // Check for time
    if (should_stop()) {
        return 0;
    }
    
    // Check transposition table
    int tt_score, tt_move;
    if (depth > 0 && tt_probe(board.hash, depth, tt_score, tt_move)) {
        if (tt_score >= beta) return beta;
        if (tt_score <= alpha) return alpha;
    }
    
    // Terminal position or max depth
    if (depth == 0) {
        return evaluate_position(board, color);
    }
    
    // Check for checkmate (simplified)
    if (board.is_in_check(color)) {
        // Simple check detection - in real engine, detect actual mates
        return -10000 + (max_depth - depth);
    }
    
    // Generate candidate moves
    auto moves = generate_candidates(board);
    
    if (moves.empty()) {
        // No legal moves - stalemate or checkmate
        return evaluate_position(board, color);
    }
    
    // Order moves
    order_moves(moves, board, tt_move);
    
    int best_move = moves[0];
    int best_score = -std::numeric_limits<int>::max();
    int flag = 1;  // alpha
    
    // Search all moves
    for (int move : moves) {
        if (should_stop()) return 0;
        
        Board new_board = make_move(board, move);
        int score = -alpha_beta(new_board, depth - 1, -beta, -alpha, 1 - color);
        
        if (score > best_score) {
            best_score = score;
            best_move = move;
            
            if (score > alpha) {
                alpha = score;
                flag = 3;  // exact
                
                if (score >= beta) {
                    flag = 2;  // beta cutoff
                    break;
                }
            }
        }
    }
    
    // Store in transposition table
    tt_store(board.hash, depth, best_score, best_move, flag);
    
    return best_score;
}

// Calculate thinking time based on position
int calculate_think_time(const Board& board, int base_time) {
    // Humans think longer in:
    // - Complex positions (many imbalances)
    // - King safety concerns
    // - Unclear tactical positions
    
    std::string fen = board.get_fen();
    auto imbalances = Evaluation::analyze_imbalances(fen);
    
    float complexity = 1.0f;
    
    // King safety concerns
    if (imbalances.white_king_safety < 0 || imbalances.black_king_safety < 0) {
        complexity += 0.5f;
    }
    
    // Many imbalances = complex
    if (imbalances.material_diff > 200 || imbalances.material_diff < -200) {
        complexity += 0.3f;
    }
    
    // Passed pawns increase complexity
    if (imbalances.white_has_passed_pawn || imbalances.black_has_passed_pawn) {
        complexity += 0.3f;
    }
    
    // Opening = quick decisions
    int total_material = Evaluation::evaluate(fen);
    if (total_material > 7000) {  // Early opening
        complexity *= 0.7f;
    }
    
    return static_cast<int>(base_time * complexity);
}

// Main search function with iterative deepening
SearchResult search(const std::string& fen, int max_time_ms_param, int max_search_depth) {
    Board board;
    board.set_from_fen(fen);
    
    SearchResult result = {};
    result.time_ms = max_time_ms_param;
    result.depth = max_search_depth;
    result.nodes = 0;
    result.score = 0;
    result.best_move = 0;
    
    // Set time
    max_time_ms = max_time_ms_param;
    start_time = std::chrono::steady_clock::now();
    stop_search = false;
    nodes_searched = 0;
    
    // Calculate adaptive time per depth
    int total_time = max_time_ms_param;
    int time_per_depth = total_time / max_search_depth;
    
    // Iterative deepening: search depth 1, 2, 3... up to max_search_depth
    for (int depth = 1; depth <= max_search_depth; depth++) {
        if (should_stop()) break;
        
        search_depth = depth;
        
        // Search at this depth
        int score = alpha_beta(board, depth, -std::numeric_limits<int>::max(), 
                              std::numeric_limits<int>::max(), board.side_to_move);
        
        result.score = score;
        result.depth = depth;
        
        // Get best move from TT
        int idx = tt_index(board.hash);
        result.best_move = transposition_table[idx].move;
        
        // Check time
        if (should_stop()) break;
    }
    
    result.nodes = nodes_searched;
    result.time_ms = get_elapsed_ms();
    
    return result;
}

void stop() {
    stop_search = true;
}

bool is_searching() {
    return !stop_search && get_elapsed_ms() < max_time_ms;
}

void set_threads(int n) {
    // For single-threaded alpha-beta, threads not used
    (void)n;
}

void set_hash_size(int mb) {
    // Resize transposition table
    delete[] transposition_table;
    int new_size = 1 << (20 + mb / 64);  // Scale with MB
    transposition_table = new TTEntry[new_size];
}

void set_use_mcts(bool use) {
    (void)use;  // Not used in alpha-beta implementation
}

void set_depth_limit(int depth) {
    max_depth = depth;
}

} // namespace Search
