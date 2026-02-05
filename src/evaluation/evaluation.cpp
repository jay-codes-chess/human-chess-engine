/**
 * Human Chess Engine - Evaluation Function
 * 
 * Based on Silman's Imbalance Theory and Russian Chess School methodology.
 * Evaluates positions using human-understandable concepts.
 */

#include "evaluation.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <map>
#include <cctype>

namespace Evaluation {

// Style weights storage
static StyleWeights current_weights;
static std::string current_style = "classical";

// Piece-square tables for positional evaluation
// PST[piece_type][square] - from White's perspective
static const int KNIGHT_PST[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20,   0,   0,   0,   0, -20, -40,
    -30,   0,  10,  15,  15,  10,   0, -30,
    -30,   5,  15,  20,  20,  15,   5, -30,
    -30,   0,  15,  20,  20,  15,   0, -30,
    -30,   5,  10,  15,  15,  10,   5, -30,
    -40, -20,   0,   5,   5,   0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50
};

static const int BISHOP_PST[64] = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   5,   5,  10,  10,   5,   5, -10,
    -10,   0,  10,  10,  10,  10,   0, -10,
    -10,  10,  10,  10,  10,  10,  10, -10,
    -10,   5,   0,   0,   0,   0,   5, -10,
    -20, -10, -10, -10, -10, -10, -10, -20
};

static const int ROOK_PST[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
      0,   0,   5,  10,  10,   5,   0,   0
};

static const int QUEEN_PST[64] = {
    -20, -10, -10,  -5,  -5, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,   5,   5,   5,   0, -10,
     -5,   0,   5,   5,   5,   5,   0,  -5,
     -5,   0,   5,   5,   5,   5,   0,  -5,
    -10,   0,   5,   5,   5,   5,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -20, -10, -10,  -5,  -5, -10, -10, -20
};

static const int PAWN_PST[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
     50,  50,  50,  50,  50,  50,  50,  50,
     10,  10,  20,  30,  30,  20,  10,  10,
      5,   5,  10,  25,  25,  10,   5,   5,
      0,   0,   0,  20,  20,   0,   0,   0,
      5,  -5, -10,   0,   0, -10,  -5,   5,
      5,  10,  10, -20, -20,  10,  10,   5,
      0,   0,   0,   0,   0,   0,   0,   0
};

// Simplified board representation for demonstration
struct Piece {
    int type;      // 0=empty, 1=pawn, 2=knight, 3=bishop, 4=rook, 5=queen, 6=king
    int color;     // 0=white, 1=black
};

struct Board {
    Piece squares[64];
    int side_to_move;  // 0=white, 1=black
    int castling[2][2]; // [color][side] - kingside, queenside
    int en_passant;
    int fullmove_number;
    int halfmove_clock;
};

// Mirror square for black
int mirror_square(int sq) {
    return 63 - sq;
}

// Get piece-square table value
int get_pst_value(int piece_type, int square, int color) {
    int idx = (color == 0) ? square : mirror_square(square);
    
    switch (piece_type) {
        case 1: return PAWN_PST[idx];
        case 2: return KNIGHT_PST[idx];
        case 3: return BISHOP_PST[idx];
        case 4: return ROOK_PST[idx];
        case 5: return QUEEN_PST[idx];
        default: return 0;
    }
}

// Convert FEN to board
Board parse_fen(const std::string& fen) {
    Board board = {};
    std::istringstream ss(fen);
    std::string board_str, castling_str, side_str;
    
    // Board position
    ss >> board_str;
    int sq = 56; // Start from a8
    for (char c : board_str) {
        if (c == '/') {
            sq -= 16; // Move to next rank
        } else if (c >= '1' && c <= '8') {
            sq += (c - '0'); // Empty squares
        } else {
            int piece = 0;
            bool is_white = true;
            char pc = std::tolower(c);
            switch (pc) {
                case 'p': piece = 1; break;
                case 'n': piece = 2; break;
                case 'b': piece = 3; break;
                case 'r': piece = 4; break;
                case 'q': piece = 5; break;
                case 'k': piece = 6; break;
            }
            board.squares[sq] = {piece, is_white ? 0 : 1};
            sq++;
        }
    }
    
    // Side to move
    ss >> side_str;
    board.side_to_move = (side_str == "w") ? 0 : 1;
    
    // Castling rights
    ss >> castling_str;
    board.castling[0][0] = castling_str.find('K') != std::string::npos;
    board.castling[0][1] = castling_str.find('Q') != std::string::npos;
    board.castling[1][0] = castling_str.find('k') != std::string::npos;
    board.castling[1][1] = castling_str.find('q') != std::string::npos;
    
    return board;
}

// Count material
int count_material(const Board& board, int color) {
    int material = 0;
    for (int i = 0; i < 64; i++) {
        if (board.squares[i].color == color) {
            switch (board.squares[i].type) {
                case 1: material += PAWN_VALUE; break;
                case 2: material += KNIGHT_VALUE; break;
                case 3: material += BISHOP_VALUE; break;
                case 4: material += ROOK_VALUE; break;
                case 5: material += QUEEN_VALUE; break;
            }
        }
    }
    return material;
}

// Count pieces of each type
void count_pieces(const Board& board, int counts[7][2]) {
    for (int c = 0; c < 2; c++) {
        for (int p = 1; p <= 6; p++) counts[p][c] = 0;
    }
    for (int i = 0; i < 64; i++) {
        if (board.squares[i].type > 0) {
            counts[board.squares[i].type][board.squares[i].color]++;
        }
    }
}

// Evaluate piece activity and mobility
int evaluate_piece_activity(const Board& board, int color) {
    int activity = 0;
    
    for (int i = 0; i < 64; i++) {
        if (board.squares[i].color == color && board.squares[i].type > 0) {
            // Add PST value for piece placement
            activity += get_pst_value(board.squares[i].type, i, color);
            
            // Bonus for developed pieces (knights/bishops away from starting rank)
            if (board.squares[i].type == 2 || board.squares[i].type == 3) {
                int rank = i / 8;
                if (color == 0 && rank > 1) activity += 10;
                if (color == 1 && rank < 6) activity += 10;
            }
            
            // Bonus for centralized pieces
            int file = i % 8;
            int center_distance = std::abs(3 - file) + std::abs(3 - (i / 8));
            if (center_distance <= 2) activity += 5;
        }
    }
    
    return activity;
}

// Evaluate pawn structure
int evaluate_pawn_structure(const Board& board, int color) {
    int score = 0;
    
    for (int i = 0; i < 64; i++) {
        if (board.squares[i].type == 1 && board.squares[i].color == color) {
            int file = i % 8;
            int rank = i / 8;
            
            // Passed pawn bonus (simplified)
            bool is_passed = true;
            int pawn_ahead = rank + (color == 0 ? 1 : -1);
            for (int df = -1; df <= 1; df++) {
                int check_file = file + df;
                if (check_file >= 0 && check_file < 8) {
                    int check_sq = check_file * 8 + pawn_ahead;
                    if (check_sq >= 0 && check_sq < 64 &&
                        board.squares[check_sq].type == 1 && 
                        board.squares[check_sq].color != color) {
                        is_passed = false;
                        break;
                    }
                }
            }
            if (is_passed) score += 50;
            
            // Isolated pawn penalty
            bool has_neighbor = false;
            for (int df = -1; df <= 1; df += 2) {
                int neighbor_file = file + df;
                if (neighbor_file >= 0 && neighbor_file < 8) {
                    for (int dr = -1; dr <= 1; dr++) {
                        int neighbor_sq = neighbor_file * 8 + rank + dr;
                        if (neighbor_sq >= 0 && neighbor_sq < 64 &&
                            board.squares[neighbor_sq].type == 1 &&
                            board.squares[neighbor_sq].color == color) {
                            has_neighbor = true;
                            break;
                        }
                    }
                }
            }
            if (!has_neighbor) score -= 20;
            
            // Doubled pawn penalty
            for (int dr = -1; dr <= 1; dr += 2) {
                int stacked_sq = file * 8 + rank + dr;
                if (stacked_sq >= 0 && stacked_sq < 64 &&
                    board.squares[stacked_sq].type == 1 &&
                    board.squares[stacked_sq].color == color) {
                    score -= 10;
                    break;
                }
            }
        }
    }
    
    return score;
}

// Evaluate space control
float evaluate_space(const Board& board, int color) {
    float space = 0;
    int start_rank = (color == 0) ? 4 : 3;
    int end_rank = 7;
    
    for (int rank = start_rank; rank <= end_rank; rank++) {
        for (int file = 0; file < 8; file++) {
            int sq = rank * 8 + file;
            if (board.squares[sq].color == color && board.squares[sq].type > 0) {
                space += 1.0f;
            }
        }
    }
    
    return space;
}

// Evaluate king safety
int evaluate_king_safety(const Board& board, int color) {
    int safety = 0;
    
    // Find the king
    int king_sq = -1;
    for (int i = 0; i < 64; i++) {
        if (board.squares[i].type == 6 && board.squares[i].color == color) {
            king_sq = i;
            break;
        }
    }
    
    if (king_sq == -1) return -10000; // King captured!
    
    int king_file = king_sq % 8;
    int king_rank = king_sq / 8;
    int shield_bonus = 0;
    
    // Pawn shield evaluation (simplified)
    int shield_rank = (color == 0) ? king_rank - 1 : king_rank + 1;
    if (shield_rank >= 0 && shield_rank < 8) {
        for (int df = -1; df <= 1; df++) {
            int file = king_file + df;
            if (file >= 0 && file < 8) {
                int sq = shield_rank * 8 + file;
                if (board.squares[sq].type == 1 && board.squares[sq].color == color) {
                    shield_bonus += 10;
                }
            }
        }
    }
    
    safety += shield_bonus;
    
    // Castling bonus
    if (color == 0) {
        if (board.castling[0][0] || board.castling[0][1]) safety += 20;
    } else {
        if (board.castling[1][0] || board.castling[1][1]) safety += 20;
    }
    
    // Exposed king penalty
    int center_distance = std::abs(3 - king_file) + std::abs(3 - king_rank);
    safety -= center_distance * 3;
    
    return safety;
}

// Minor piece evaluation (bishop vs knight)
void evaluate_minor_pieces(const Board& board, Imbalances& imbalances) {
    int counts[7][2];
    count_pieces(board, counts);
    
    int white_knights = counts[2][0];
    int black_knights = counts[2][1];
    int white_bishops = counts[3][0];
    int black_bishops = counts[3][1];
    
    int white_minor_score = white_knights * 3 + white_bishops * 3;
    int black_minor_score = black_knights * 3 + black_bishops * 3;
    
    if (white_minor_score > black_minor_score + 3) {
        imbalances.white_has_better_minor = true;
    }
    if (black_minor_score > white_minor_score + 3) {
        imbalances.black_has_better_minor = true;
    }
}

// Evaluate development (early game only)
int evaluate_development(const Board& board, int color) {
    int development = 0;
    int pieces_at_home = 0;
    int start_rank = (color == 0) ? 0 : 7;
    
    for (int file = 0; file < 8; file++) {
        int sq = start_rank * 8 + file;
        Piece p = board.squares[sq];
        if (p.type > 0 && p.color == color) {
            if (p.type == 2 || p.type == 3) pieces_at_home++;
            if (p.type >= 4 && p.type <= 6) pieces_at_home++;
        }
    }
    
    development -= pieces_at_home * 15;
    return development;
}

// Check if position is in opening (simplified)
bool is_opening(const Board& board) {
    int total_material = count_material(board, 0) + count_material(board, 1);
    return total_material > 4000;
}

// Main evaluation function
int evaluate(const std::string& fen) {
    Board board = parse_fen(fen);
    
    // Get material difference
    int white_material = count_material(board, 0);
    int black_material = count_material(board, 1);
    int material_diff = white_material - black_material;
    
    // Evaluate components
    int white_activity = evaluate_piece_activity(board, 0);
    int black_activity = evaluate_piece_activity(board, 1);
    int activity_diff = white_activity - black_activity;
    
    int white_pawns = evaluate_pawn_structure(board, 0);
    int black_pawns = evaluate_pawn_structure(board, 1);
    int pawn_diff = white_pawns - black_pawns;
    
    float white_space = evaluate_space(board, 0);
    float black_space = evaluate_space(board, 1);
    float space_diff = white_space - black_space;
    
    int white_king = evaluate_king_safety(board, 0);
    int black_king = evaluate_king_safety(board, 1);
    int king_diff = white_king - black_king;
    
    // Development (opening only)
    int development_diff = 0;
    if (is_opening(board)) {
        development_diff = evaluate_development(board, 0) - evaluate_development(board, 1);
    }
    
    // Apply style weights
    const StyleWeights& w = current_weights;
    
    int score = 0;
    score += material_diff * w.material;
    score += activity_diff * w.piece_activity;
    score += pawn_diff * w.pawn_structure;
    score += static_cast<int>(space_diff * w.space * 10);
    score += king_diff * w.king_safety;
    score += development_diff * w.development;
    
    return score;
}

// Analyze imbalances
Imbalances analyze_imbalances(const std::string& fen) {
    Board board = parse_fen(fen);
    Imbalances imb = {};
    
    // Material
    imb.material_diff = count_material(board, 0) - count_material(board, 1);
    
    // Minor pieces
    evaluate_minor_pieces(board, imb);
    
    // Pawn structure
    int white_pawns = evaluate_pawn_structure(board, 0);
    int black_pawns = evaluate_pawn_structure(board, 1);
    imb.white_weak_pawns = (white_pawns < -30) ? 1 : 0;
    imb.black_weak_pawns = (black_pawns < -30) ? 1 : 0;
    
    // Space
    float white_space = evaluate_space(board, 0);
    float black_space = evaluate_space(board, 1);
    if (white_space > black_space + 5) imb.white_has_passed_pawn = true;
    if (black_space > white_space + 5) imb.black_has_passed_pawn = true;
    
    // King safety
    imb.white_king_safety = evaluate_king_safety(board, 0);
    imb.black_king_safety = evaluate_king_safety(board, 1);
    
    // Development
    if (is_opening(board)) {
        imb.white_development_score = evaluate_development(board, 0);
        imb.black_development_score = evaluate_development(board, 1);
    }
    
    return imb;
}

// Get verbal explanation
VerbalExplanation explain(int score, const std::string& fen) {
    VerbalExplanation exp;
    Imbalances imb = analyze_imbalances(fen);
    
    // Material explanation
    if (imb.material_diff > 0) {
        exp.move_reasons.push_back("White has material advantage");
    } else if (imb.material_diff < 0) {
        exp.move_reasons.push_back("Black has material advantage");
    }
    
    // Minor piece explanation
    if (imb.white_has_better_minor) {
        exp.imbalance_notes.push_back("White has better minor pieces");
    }
    if (imb.black_has_better_minor) {
        exp.imbalance_notes.push_back("Black has better minor pieces");
    }
    
    // King safety
    if (imb.white_king_safety > imb.black_king_safety + 20) {
        exp.imbalance_notes.push_back("White's king is safer");
    } else if (imb.black_king_safety > imb.white_king_safety + 20) {
        exp.imbalance_notes.push_back("Black's king is safer");
    }
    
    // Development
    if (imb.white_development_score > imb.black_development_score + 20) {
        exp.imbalance_notes.push_back("White leads in development");
    } else if (imb.black_development_score > imb.white_development_score + 20) {
        exp.imbalance_notes.push_back("Black leads in development");
    }
    
    return exp;
}

// Initialize evaluation
void initialize() {
    set_style("classical");
}

// Set style weights
void set_style(const std::string& style_name) {
    current_style = style_name;
    
    if (style_name == "classical") {
        current_weights = {1.0f, 0.5f, 0.5f, 0.3f, 0.4f, 0.6f, 0.3f, 0.4f};
    } else if (style_name == "attacking") {
        current_weights = {0.8f, 0.8f, 0.4f, 0.4f, 1.0f, 0.3f, 0.2f, 0.2f};
    } else if (style_name == "tactical") {
        current_weights = {0.7f, 1.0f, 0.3f, 0.3f, 1.2f, 0.4f, 0.2f, 0.2f};
    } else if (style_name == "positional") {
        current_weights = {1.0f, 0.6f, 0.8f, 0.6f, 0.3f, 0.5f, 0.4f, 0.6f};
    } else if (style_name == "technical") {
        current_weights = {1.0f, 0.4f, 0.6f, 0.4f, 0.2f, 0.8f, 0.3f, 0.5f};
    } else {
        current_weights = {1.0f, 0.5f, 0.5f, 0.3f, 0.4f, 0.6f, 0.3f, 0.4f};
    }
}

// Get current style name
std::string get_style_name() {
    return current_style;
}

} // namespace Evaluation
