#!/usr/bin/env python3
"""Test a full game with Human Chess Engine"""

import subprocess
import sys

ENGINE = "./human-chess-engine"

def run_uci(fen, depth=3):
    """Get best move from engine"""
    cmd = f"uci\nposition fen {fen}\ngo depth {depth}\nquit"
    result = subprocess.run([ENGINE], input=cmd, capture_output=True, text=True)
    for line in result.stdout.split('\n'):
        if line.startswith('bestmove'):
            parts = line.split()
            if len(parts) >= 2:
                return parts[1]
    return None

def parse_fen(fen):
    """Parse FEN to 2D board (rank 8 at index 0, rank 1 at index 7)"""
    parts = fen.split()
    board_str = parts[0]
    side = parts[1]
    
    board = []
    for row in board_str.split('/'):
        new_row = []
        for c in row:
            if c.isdigit():
                new_row.extend(['.'] * int(c))
            else:
                new_row.append(c)
        board.append(new_row)
    return board, side  # board[0] = rank 8, board[7] = rank 1

def board_to_fen(board, side):
    """Convert 2D board back to FEN (board[0]=rank 8, board[7]=rank 1)"""
    rows = []
    for row in board:
        fen_row = ''
        empty = 0
        for c in row:
            if c == '.':
                empty += 1
            else:
                if empty > 0:
                    fen_row += str(empty)
                    empty = 0
                fen_row += c
        if empty > 0:
            fen_row += str(empty)
        rows.append(fen_row)
    return '/'.join(rows) + f" {side} KQkq - 0 1"

def apply_move(fen, move):
    """Apply a UCI move to FEN and return new FEN"""
    board, side = parse_fen(fen)
    
    # FEN: board[0] = rank 8, board[7] = rank 1
    # UCI: a1 = file 0, rank 0 (white's perspective)
    # So UCI rank 1 = board[7], UCI rank 8 = board[0]
    
    from_file = ord(move[0]) - ord('a')
    from_rank = int(move[1]) - 1  # 0 = rank 1 (white's perspective)
    to_rank = int(move[3]) - 1
    
    # Convert from UCI rank (0=rank1) to board index (7=rank1)
    from_idx = 7 - from_rank
    to_idx = 7 - to_rank
    
    to_file = ord(move[2]) - ord('a')
    
    piece = board[from_idx][from_file]
    
    # Move piece
    board[to_idx][to_file] = piece
    board[from_idx][from_file] = '.'
    
    # Handle promotion (assume queen)
    if len(move) == 5:
        board[to_idx][to_file] = move[4].upper() if piece.isupper() else move[4].lower()
    
    # Switch side
    new_side = 'b' if side == 'w' else 'w'
    
    return board_to_fen(board, new_side)

def main():
    print("=" * 50)
    print("Human Chess Engine - Full Game Test")
    print("=" * 50)
    print()
    
    fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    
    for move_num in range(1, 21):
        # White's move
        print(f"Move {move_num}. White: ", end="", flush=True)
        move = run_uci(fen, depth=3)
        if not move:
            print("ERROR - no move returned")
            break
        print(move)
        fen = apply_move(fen, move)
        
        if move == "0000":
            print("Checkmate or stalemate!")
            break
        
        # Black's move
        print(f"Move {move_num}... Black: ", end="", flush=True)
        move = run_uci(fen, depth=3)
        if not move:
            print("ERROR - no move returned")
            break
        print(move)
        fen = apply_move(fen, move)
        
        if move == "0000":
            print("Checkmate or stalemate!")
            break
    
    print()
    print("=" * 50)
    print("Game completed!")
    print("=" * 50)

if __name__ == "__main__":
    main()
