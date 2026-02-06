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

// Killer moves - moves that caused cutoffs at sibling nodes
static int killer_moves[64][2];  // [depth][2 killer moves]

// History heuristic - scores moves based on success
static int history_scores[64][64];  // [from][to] history score

// Update history score
void update_history(int move, int depth, int bonus) {
    int from = move >> 6;
    int to = move & 63;
    history_scores[from][to] += bonus;
    
    // Cap history scores to prevent overflow
    if (history_scores[from][to] > 10000) {
        for (int i = 0; i < 64; i++) {
            for (int j = 0; j < 64; j++) {
                history_scores[i][j] = std::max(0, history_scores[i][j] / 2);
            }
        }
    }
}

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
    // Initialize history and killer tables
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 2; j++) {
            killer_moves[i][j] = 0;
        }
        for (int j = 0; j < 64; j++) {
            history_scores[i][j] = 0;
        }
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
    
    // Only stop if we exceed max time
    if (elapsed > max_time_ms) return true;
    
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

// Get piece value for ordering
int get_piece_value(int piece) {
    switch(piece) {
        case PAWN: return 100;
        case KNIGHT: return 320;
        case BISHOP: return 330;
        case ROOK: return 500;
        case QUEEN: return 900;
        case KING: return 20000;
        default: return 0;
    }
}

// Score move for ordering (used in order_moves)
int score_move_for_order(Board& board, int move, int tt_move, int depth) {
    int score = 0;
    
    // TT move gets highest priority
    if (move == tt_move) return 100000;
    
    int from = move >> 6;
    int to = move & 63;
    int piece = board.piece_at(from);
    int captured = board.piece_at(to);
    
    // Captures: MVV-LVA ordering
    if (captured != NO_PIECE) {
        int attacker_val = get_piece_value(piece);
        int victim_val = get_piece_value(captured);
        score = 10000 + victim_val * 10 - attacker_val;
    }
    
    // Killer moves
    if (move == killer_moves[depth][0]) {
        score = 8000;
    } else if (move == killer_moves[depth][1]) {
        score = 7000;
    }
    
    // History heuristic
    score += history_scores[from][to];
    
    return score;
}

// Order moves for better alpha-beta performance
void order_moves(std::vector<int>& moves, Board& board, int tt_move, int depth) {
    // Score and sort all moves
    std::sort(moves.begin(), moves.end(), [&](int a, int b) {
        int score_a = score_move_for_order(board, a, tt_move, depth);
        int score_b = score_move_for_order(board, b, tt_move, depth);
        return score_a > score_b;
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

// Quiescence search - extends search in tactical positions
// Avoids the horizon effect by only searching captures/checks
int quiescence_search(Board& board, int alpha, int beta, int color) {
    nodes_searched++;
    
    // Stand-pat evaluation
    int stand_pat = evaluate_position(board, color);
    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;
    
    // Generate only capture moves
    auto all_moves = generate_moves(board);
    std::vector<int> captures;
    
    for (int move : all_moves) {
        if (!is_legal(board, move)) continue;
        
        int to = move & 63;
        int captured = board.piece_at(to);
        
        // Only include captures (skip quiet moves)
        if (captured != NO_PIECE) {
            captures.push_back(move);
        }
    }
    
    // Order captures using MVV-LVA
    std::sort(captures.begin(), captures.end(), [&](int a, int b) {
        int from_a = a >> 6;
        int to_a = a & 63;
        int from_b = b >> 6;
        int to_b = b & 63;
        
        int piece_a = board.piece_at(from_a);
        int captured_a = board.piece_at(to_a);
        int piece_b = board.piece_at(from_b);
        int captured_b = board.piece_at(to_b);
        
        int score_a = get_piece_value(captured_a) * 10 - get_piece_value(piece_a);
        int score_b = get_piece_value(captured_b) * 10 - get_piece_value(piece_b);
        return score_a > score_b;
    });
    
    // Search captures
    for (int move : captures) {
        if (should_stop()) return alpha;
        
        Board new_board = make_move(board, move);
        int score = -quiescence_search(new_board, -beta, -alpha, 1 - color);
        
        if (score > alpha) {
            alpha = score;
            if (alpha >= beta) return beta;
        }
    }
    
    return alpha;
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
    
    // Terminal position or max depth - use quiescence search
    if (depth == 0) {
        return quiescence_search(board, alpha, beta, color);
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
    order_moves(moves, board, tt_move, depth);
    
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
                    // Update killer move
                    if (moves.size() > 1) {
                        killer_moves[depth][1] = killer_moves[depth][0];
                        killer_moves[depth][0] = move;
                    }
                    // Update history
                    update_history(move, depth, depth * depth);
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
    
    // Generate legal moves for fallback
    auto legal_moves = generate_moves(board);
    if (legal_moves.empty()) {
        result.nodes = nodes_searched;
        result.time_ms = get_elapsed_ms();
        return result;
    }
    
    // Iterative deepening: search depth 1, 2, 3... up to max_search_depth
    for (int depth = 1; depth <= max_search_depth; depth++) {
        if (should_stop()) break;
        
        search_depth = depth;
        
        // Search at this depth
        int score = alpha_beta(board, depth, -std::numeric_limits<int>::max(), 
                              std::numeric_limits<int>::max(), board.side_to_move);
        
        result.score = score;
        result.depth = depth;
        
        // Get best move from TT or use first legal move
        int idx = tt_index(board.hash);
        int tt_move = transposition_table[idx].move;
        
        // Validate TT move is legal
        bool tt_move_legal = false;
        for (int m : legal_moves) {
            if (m == tt_move) {
                tt_move_legal = true;
                break;
            }
        }
        
        result.best_move = tt_move_legal ? tt_move : legal_moves[0];
        
        // Output progress info for GUI
        std::string best_move_uci = Bitboards::move_to_uci(result.best_move);
        std::cout << "info depth " << depth;
        std::cout << " score cp " << (score / 100);
        std::cout << " nodes " << nodes_searched;
        std::cout << " time " << get_elapsed_ms();
        std::cout << " pv " << best_move_uci << std::endl;
        
        // Check time
        if (should_stop()) break;
    }
    
    result.nodes = nodes_searched;
    result.time_ms = get_elapsed_ms();
    
    // Fallback: if no move found, use first legal move
    if (result.best_move == 0 && !legal_moves.empty()) {
        result.best_move = legal_moves[0];
    }
    
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
