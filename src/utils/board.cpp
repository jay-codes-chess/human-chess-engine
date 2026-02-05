/**
 * Board Representation Implementation
 */

#include "board.hpp"
#include <sstream>
#include <algorithm>

void Board::set_start_position() {
    clear();
    
    // White pieces
    add_piece(0, ROOK, WHITE);
    add_piece(1, KNIGHT, WHITE);
    add_piece(2, BISHOP, WHITE);
    add_piece(3, QUEEN, WHITE);
    add_piece(4, KING, WHITE);
    add_piece(5, BISHOP, WHITE);
    add_piece(6, KNIGHT, WHITE);
    add_piece(7, ROOK, WHITE);
    for (int i = 8; i < 16; i++) add_piece(i, PAWN, WHITE);
    
    // Black pieces
    add_piece(56, ROOK, BLACK);
    add_piece(57, KNIGHT, BLACK);
    add_piece(58, BISHOP, BLACK);
    add_piece(59, QUEEN, BLACK);
    add_piece(60, KING, BLACK);
    add_piece(61, BISHOP, BLACK);
    add_piece(62, KNIGHT, BLACK);
    add_piece(63, ROOK, BLACK);
    for (int i = 48; i < 56; i++) add_piece(i, PAWN, BLACK);
    
    side_to_move = WHITE;
    castling[WHITE][0] = castling[WHITE][1] = true;
    castling[BLACK][0] = castling[BLACK][1] = true;
    en_passant_square = -1;
    fullmove_number = 1;
    halfmove_clock = 0;
    
    compute_hash();
}

bool Board::set_from_fen(const std::string& fen) {
    clear();
    
    std::istringstream ss(fen);
    std::string board_str, side_str, castling_str, ep_str;
    ss >> board_str >> side_str >> castling_str >> ep_str;
    
    // Parse board
    int sq = 56;
    for (char c : board_str) {
        if (c == '/') {
            sq -= 16;
        } else if (c >= '1' && c <= '8') {
            sq += (c - '0');
        } else {
            int piece_type = NO_PIECE;
            bool is_white = !std::islower(c);
            char pc = std::tolower(c);
            switch (pc) {
                case 'p': piece_type = PAWN; break;
                case 'n': piece_type = KNIGHT; break;
                case 'b': piece_type = BISHOP; break;
                case 'r': piece_type = ROOK; break;
                case 'q': piece_type = QUEEN; break;
                case 'k': piece_type = KING; break;
            }
            add_piece(sq, piece_type, is_white ? WHITE : BLACK);
            sq++;
        }
    }
    
    // Side to move
    side_to_move = (side_str == "w") ? WHITE : BLACK;
    
    // Castling
    castling[WHITE][0] = castling_str.find('K') != std::string::npos;
    castling[WHITE][1] = castling_str.find('Q') != std::string::npos;
    castling[BLACK][0] = castling_str.find('k') != std::string::npos;
    castling[BLACK][1] = castling_str.find('q') != std::string::npos;
    
    // En passant
    if (ep_str == "-") {
        en_passant_square = -1;
    } else {
        int file = ep_str[0] - 'a';
        int rank = ep_str[1] - '1';
        en_passant_square = file + rank * 8;
    }
    
    // Move counters
    std::string move_str;
    ss >> move_str;
    if (!move_str.empty()) {
        fullmove_number = std::stoi(move_str);
    }
    ss >> move_str;
    if (!move_str.empty()) {
        halfmove_clock = std::stoi(move_str);
    }
    
    compute_hash();
    return true;
}

std::string Board::get_fen() const {
    std::ostringstream ss;
    
    // Board
    for (int rank = 7; rank >= 0; rank--) {
        int empty = 0;
        for (int file = 0; file < 8; file++) {
            int sq = file + rank * 8;
            int p = piece_at(sq);
            int c = color_at(sq);
            
            if (p == NO_PIECE) {
                empty++;
            } else {
                if (empty > 0) {
                    ss << empty;
                    empty = 0;
                }
                char ch = " pnbrqk"[p];
                if (c == BLACK) ch = std::tolower(ch);
                ss << ch;
            }
        }
        if (empty > 0) ss << empty;
        if (rank > 0) ss << '/';
    }
    
    ss << ' ';
    ss << (side_to_move == WHITE ? 'w' : 'b');
    ss << ' ';
    
    // Castling
    std::string cast;
    if (castling[WHITE][0]) cast += 'K';
    if (castling[WHITE][1]) cast += 'Q';
    if (castling[BLACK][0]) cast += 'k';
    if (castling[BLACK][1]) cast += 'q';
    ss << (cast.empty() ? "-" : cast);
    ss << ' ';
    
    // En passant
    if (en_passant_square == -1) {
        ss << '-';
    } else {
        ss << char('a' + Bitboards::file_of(en_passant_square));
        ss << char('1' + Bitboards::rank_of(en_passant_square));
    }
    
    ss << ' ' << fullmove_number << ' ' << halfmove_clock;
    
    return ss.str();
}

void Board::clear() {
    for (int i = 0; i < 7; i++) pieces[i] = 0;
    colors[WHITE] = colors[BLACK] = 0;
}

void Board::add_piece(int square, int piece_type, int color) {
    if (piece_type == NO_PIECE) return;
    
    Bitboards::set(pieces[piece_type], square);
    Bitboards::set(colors[color], square);
}

void Board::remove_piece(int square) {
    for (int pt = PAWN; pt <= KING; pt++) {
        if (Bitboards::test(pieces[pt], square)) {
            pieces[pt] &= ~(1ULL << square);
        }
    }
    colors[WHITE] &= ~(1ULL << square);
    colors[BLACK] &= ~(1ULL << square);
}

void Board::move_piece(int from, int to) {
    int pt = piece_at(from);
    int c = color_at(from);
    
    remove_piece(from);
    add_piece(to, pt, c);
}

int Board::piece_at(int square) const {
    if (square < 0 || square >= 64) return NO_PIECE;
    for (int pt = PAWN; pt <= KING; pt++) {
        if (Bitboards::test(pieces[pt], square)) return pt;
    }
    return NO_PIECE;
}

int Board::color_at(int square) const {
    if (square < 0 || square >= 64) return -1;
    if (Bitboards::test(colors[WHITE], square)) return WHITE;
    if (Bitboards::test(colors[BLACK], square)) return BLACK;
    return -1;
}

bool Board::is_empty(int square) const {
    return piece_at(square) == NO_PIECE;
}

uint64_t Board::pieces_of_color(int color) const {
    return colors[color];
}

uint64_t Board::all_pieces() const {
    return colors[WHITE] | colors[BLACK];
}

void Board::compute_hash() {
    // Simple hash - just xor pieces together
    hash = 0;
    uint64_t seed = 1469598103934665603ULL;  // FNV offset basis
    for (int sq = 0; sq < 64; sq++) {
        int p = piece_at(sq);
        if (p != NO_PIECE) {
            seed ^= (sq + p * 7);
            seed *= 1099511628211ULL;  // FNV prime
        }
    }
    seed ^= side_to_move;
    hash = seed;
}

bool Board::is_in_check(int color) const {
    int king_sq = -1;
    for (int sq = 0; sq < 64; sq++) {
        if (piece_at(sq) == KING && color_at(sq) == color) {
            king_sq = sq;
            break;
        }
    }
    if (king_sq == -1) return false;
    return Bitboards::is_square_attacked(*this, king_sq, 1 - color);
}

std::vector<int> Board::generate_moves() const {
    std::vector<int> moves;
    
    uint64_t our_pieces = pieces_of_color(side_to_move);
    uint64_t enemy_pieces = pieces_of_color(1 - side_to_move);
    uint64_t all = all_pieces();
    
    // Pawn moves
    uint64_t pawns = pieces[PAWN] & colors[side_to_move];
    while (pawns) {
        int sq = Bitboards::pop_lsb(pawns);
        
        // Forward moves
        int forward = sq + Bitboards::PAWN_MOVE[side_to_move];
        if (forward >= 0 && forward < 64 && is_empty(forward)) {
            moves.push_back((sq << 6) | forward);
            
            // Double move from starting rank
            int start_rank = (side_to_move == WHITE) ? 1 : 6;
            if (Bitboards::rank_of(sq) == start_rank) {
                int double_forward = forward + Bitboards::PAWN_MOVE[side_to_move];
                if (is_empty(double_forward)) {
                    moves.push_back((sq << 6) | double_forward);
                }
            }
        }
        
        // Captures
        for (int i = 0; i < 2; i++) {
            int cap = sq + Bitboards::PAWN_ATTACK[side_to_move][i];
            if (cap >= 0 && cap < 64) {
                if (Bitboards::test(enemy_pieces, cap)) {
                    moves.push_back((sq << 6) | cap);
                } else if (cap == en_passant_square) {
                    moves.push_back((sq << 6) | cap);
                }
            }
        }
    }
    
    // Knight moves
    uint64_t knights = pieces[KNIGHT] & colors[side_to_move];
    while (knights) {
        int sq = Bitboards::pop_lsb(knights);
        uint64_t attacks = Bitboards::knight_attacks(sq) & ~our_pieces;
        while (attacks) {
            int to = Bitboards::pop_lsb(attacks);
            moves.push_back((sq << 6) | to);
        }
    }
    
    // King moves
    uint64_t king = pieces[KING] & colors[side_to_move];
    while (king) {
        int sq = Bitboards::pop_lsb(king);
        uint64_t attacks = Bitboards::king_attacks(sq) & ~our_pieces;
        while (attacks) {
            int to = Bitboards::pop_lsb(attacks);
            moves.push_back((sq << 6) | to);
        }
    }
    
    // Sliding pieces (bishop, rook, queen)
    uint64_t bishops = pieces[BISHOP] & colors[side_to_move];
    while (bishops) {
        int sq = Bitboards::pop_lsb(bishops);
        uint64_t attacks = Bitboards::bishop_attacks(sq, all) & ~our_pieces;
        while (attacks) {
            int to = Bitboards::pop_lsb(attacks);
            moves.push_back((sq << 6) | to);
        }
    }
    
    uint64_t rooks = pieces[ROOK] & colors[side_to_move];
    while (rooks) {
        int sq = Bitboards::pop_lsb(rooks);
        uint64_t attacks = Bitboards::rook_attacks(sq, all) & ~our_pieces;
        while (attacks) {
            int to = Bitboards::pop_lsb(attacks);
            moves.push_back((sq << 6) | to);
        }
    }
    
    uint64_t queens = pieces[QUEEN] & colors[side_to_move];
    while (queens) {
        int sq = Bitboards::pop_lsb(queens);
        uint64_t attacks = Bitboards::queen_attacks(sq, all) & ~our_pieces;
        while (attacks) {
            int to = Bitboards::pop_lsb(attacks);
            moves.push_back((sq << 6) | to);
        }
    }
    
    return moves;
}

namespace Bitboards {

uint64_t knight_attacks(int square) {
    static const uint64_t table[64] = {
        0x0000000000020400ULL, 0x0000000000050800ULL, 0x00000000000A1100ULL, 0x0000000000142200ULL,
        0x0000000000284400ULL, 0x0000000000508800ULL, 0x0000000000A01000ULL, 0x0000000000400200ULL,
        0x0000000002040004ULL, 0x0000000005080008ULL, 0x000000000A110011ULL, 0x0000000014220022ULL,
        0x0000000028440044ULL, 0x0000000050880088ULL, 0x00000000A0100010ULL, 0x0000000040002000ULL,
        0x0000000204000402ULL, 0x0000000508000805ULL, 0x0000000A1100110AULL, 0x0000001422002214ULL,
        0x0000002844004428ULL, 0x0000005088008850ULL, 0x000000A0100010A0ULL, 0x0000004000020040ULL,
        0x0000020400040200ULL, 0x0000050800080500ULL, 0x00000A1100110A00ULL, 0x0000142200221400ULL,
        0x0000284400442800ULL, 0x0000508800885000ULL, 0x0000A0100010A000ULL, 0x0000400002004000ULL,
        0x0002040004020000ULL, 0x0005080008050000ULL, 0x000A1100110A0000ULL, 0x0014220022140000ULL,
        0x0028440044280000ULL, 0x0050880088500000ULL, 0x00A0100010A00000ULL, 0x0040000200400000ULL,
        0x0204000402000000ULL, 0x0508000805000000ULL, 0x0A1100110A000000ULL, 0x1422002214000000ULL,
        0x2844004428000000ULL, 0x5088008850000000ULL, 0xA0100010A0000000ULL, 0x4000020040000000ULL,
        0x0400040200000000ULL, 0x0800080500000000ULL, 0x1100110A00000000ULL, 0x2200221400000000ULL,
        0x4400442800000000ULL, 0x8800885000000000ULL, 0x100010A000000000ULL, 0x0000020040000000ULL,
        0x0004000200000000ULL, 0x0008000500000000ULL, 0x0011000A00000000ULL, 0x0022001400000000ULL,
        0x0044002800000000ULL, 0x0088005000000000ULL, 0x001000A000000000ULL, 0x0000004000000000ULL
    };
    return table[square];
}

uint64_t king_attacks(int square) {
    static const uint64_t table[64] = {
        0x0000000000000302ULL, 0x0000000000000507ULL, 0x0000000000000A0EULL, 0x000000000000141CULL,
        0x0000000000002838ULL, 0x0000000000005070ULL, 0x000000000000A0E0ULL, 0x000000000000403CULL,
        0x0000000000030203ULL, 0x0000000000070507ULL, 0x00000000000E0E0EULL, 0x00000000001C1C1CULL,
        0x0000000000383838ULL, 0x0000000000707070ULL, 0x0000000000E0E0E0ULL, 0x0000000000C0C0C0ULL,
        0x0000000003020300ULL, 0x0000000007050700ULL, 0x000000000E0E0E00ULL, 0x000000001C1C1C00ULL,
        0x0000000038383800ULL, 0x0000000070707000ULL, 0x00000000E0E0E000ULL, 0x00000000C0C0C000ULL,
        0x0000000302030000ULL, 0x0000000705070000ULL, 0x0000000E0E0E0000ULL, 0x0000001C1C1C0000ULL,
        0x0000003838380000ULL, 0x0000007070700000ULL, 0x000000E0E0E00000ULL, 0x000000C0C0C00000ULL,
        0x0000030203000000ULL, 0x0000070507000000ULL, 0x00000E0E0E000000ULL, 0x00001C1C1C000000ULL,
        0x0000383838000000ULL, 0x0000707070000000ULL, 0x0000E0E0E0000000ULL, 0x0000C0C0C0000000ULL,
        0x0003020300000000ULL, 0x0007050700000000ULL, 0x000E0E0E00000000ULL, 0x001C1C1C00000000ULL,
        0x0038383800000000ULL, 0x0070707000000000ULL, 0x00E0E0E000000000ULL, 0x00C0C0C000000000ULL,
        0x0302030000000000ULL, 0x0705070000000000ULL, 0x0E0E0E0000000000ULL, 0x1C1C1C0000000000ULL,
        0x3838380000000000ULL, 0x7070700000000000ULL, 0xE0E0E00000000000ULL, 0xC0C0C00000000000ULL,
        0x0203000000000000ULL, 0x0507000000000000ULL, 0x0E0E000000000000ULL, 0x1C1C000000000000ULL,
        0x3838000000000000ULL, 0x7070000000000000ULL, 0xE0E0000000000000ULL, 0xC0C0000000000000ULL
    };
    return table[square];
}

uint64_t bishop_attacks(int square, uint64_t blockers) {
    uint64_t attacks = 0;
    int file = file_of(square);
    int rank = rank_of(square);
    
    // Diagonals
    int df = 1, dr = 1;
    while (file + df < 8 && rank + dr < 8) {
        int sq = square + df + dr * 8;
        Bitboards::set(attacks, sq);
        if (blockers & (1ULL << sq)) break;
        df++; dr++;
    }
    
    df = -1; dr = 1;
    while (file + df >= 0 && rank + dr < 8) {
        int sq = square + df + dr * 8;
        Bitboards::set(attacks, sq);
        if (blockers & (1ULL << sq)) break;
        df--; dr++;
    }
    
    df = 1; dr = -1;
    while (file + df < 8 && rank + dr >= 0) {
        int sq = square + df + dr * 8;
        Bitboards::set(attacks, sq);
        if (blockers & (1ULL << sq)) break;
        df++; dr--;
    }
    
    df = -1; dr = -1;
    while (file + df >= 0 && rank + dr >= 0) {
        int sq = square + df + dr * 8;
        Bitboards::set(attacks, sq);
        if (blockers & (1ULL << sq)) break;
        df--; dr--;
    }
    
    return attacks;
}

uint64_t rook_attacks(int square, uint64_t blockers) {
    uint64_t attacks = 0;
    int file = file_of(square);
    int rank = rank_of(square);
    
    // Files
    for (int r = rank + 1; r < 8; r++) {
        int sq = file + r * 8;
        Bitboards::set(attacks, sq);
        if (blockers & (1ULL << sq)) break;
    }
    for (int r = rank - 1; r >= 0; r--) {
        int sq = file + r * 8;
        Bitboards::set(attacks, sq);
        if (blockers & (1ULL << sq)) break;
    }
    
    // Ranks
    for (int f = file + 1; f < 8; f++) {
        int sq = f + rank * 8;
        Bitboards::set(attacks, sq);
        if (blockers & (1ULL << sq)) break;
    }
    for (int f = file - 1; f >= 0; f--) {
        int sq = f + rank * 8;
        Bitboards::set(attacks, sq);
        if (blockers & (1ULL << sq)) break;
    }
    
    return attacks;
}

uint64_t queen_attacks(int square, uint64_t blockers) {
    return bishop_attacks(square, blockers) | rook_attacks(square, blockers);
}

uint64_t pawn_attacks(int square, int color) {
    uint64_t attacks = 0;
    int file = file_of(square);
    int rank = rank_of(square);
    
    int forward = (color == WHITE) ? 1 : -1;
    
    if (file > 0 && rank + forward >= 0 && rank + forward < 8) {
        Bitboards::set(attacks, square + forward * 8 - 1);
    }
    if (file < 7 && rank + forward >= 0 && rank + forward < 8) {
        Bitboards::set(attacks, square + forward * 8 + 1);
    }
    
    return attacks;
}

bool is_square_attacked(const Board& board, int square, int color) {
    uint64_t enemy_pawns = board.pieces[PAWN] & board.colors[color];
    uint64_t enemy_knights = board.pieces[KNIGHT] & board.colors[color];
    uint64_t enemy_bishops = board.pieces[BISHOP] & board.colors[color];
    uint64_t enemy_rooks = board.pieces[ROOK] & board.colors[color];
    uint64_t enemy_queens = board.pieces[QUEEN] & board.colors[color];
    uint64_t enemy_king = board.pieces[KING] & board.colors[color];
    
    // Pawn attacks
    if (Bitboards::pawn_attacks(square, 1 - color) & enemy_pawns) return true;
    
    // Knight attacks
    if (Bitboards::knight_attacks(square) & enemy_knights) return true;
    
    // King attacks
    if (Bitboards::king_attacks(square) & enemy_king) return true;
    
    // Sliding attacks
    uint64_t all = board.all_pieces();
    if (Bitboards::bishop_attacks(square, all) & (enemy_bishops | enemy_queens)) return true;
    if (Bitboards::rook_attacks(square, all) & (enemy_rooks | enemy_queens)) return true;
    
    return false;
}

uint64_t all_attacks(const Board& board, int color) {
    uint64_t attacks = 0;
    
    // Pawn attacks
    uint64_t pawns = board.pieces[PAWN] & board.colors[color];
    while (pawns) {
        int sq = Bitboards::pop_lsb(pawns);
        attacks |= Bitboards::pawn_attacks(sq, color);
    }
    
    // Knight attacks
    uint64_t knights = board.pieces[KNIGHT] & board.colors[color];
    while (knights) {
        int sq = Bitboards::pop_lsb(knights);
        attacks |= Bitboards::knight_attacks(sq);
    }
    
    // King attacks
    uint64_t king = board.pieces[KING] & board.colors[color];
    while (king) {
        int sq = Bitboards::pop_lsb(king);
        attacks |= Bitboards::king_attacks(sq);
    }
    
    // Sliding attacks
    uint64_t all = board.all_pieces();
    uint64_t bishops = board.pieces[BISHOP] & board.colors[color];
    while (bishops) {
        int sq = Bitboards::pop_lsb(bishops);
        attacks |= Bitboards::bishop_attacks(sq, all);
    }
    
    uint64_t rooks = board.pieces[ROOK] & board.colors[color];
    while (rooks) {
        int sq = Bitboards::pop_lsb(rooks);
        attacks |= Bitboards::rook_attacks(sq, all);
    }
    
    uint64_t queens = board.pieces[QUEEN] & board.colors[color];
    while (queens) {
        int sq = Bitboards::pop_lsb(queens);
        attacks |= Bitboards::queen_attacks(sq, all);
    }
    
    return attacks;
}

} // namespace Bitboards
