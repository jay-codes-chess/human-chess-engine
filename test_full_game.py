#!/usr/bin/env python3
"""Full game test - 40 moves to verify engine stability"""

import subprocess
import sys

ENGINE = "./human-chess-engine"

def run_engine(cmd):
    result = subprocess.run([ENGINE], input=cmd, capture_output=True, text=True, timeout=60)
    return result.stdout

def test_full_game():
    print("=" * 60)
    print("FULL GAME TEST - 40 moves")
    print("=" * 60)
    
    moves_white = []
    moves_black = []
    
    # Start position
    fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    
    for move_num in range(1, 41):
        # White's move
        cmd = f"uci\nposition fen {fen}\ngo depth 3\nquit\n"
        output = run_engine(cmd)
        
        for line in output.split('\n'):
            if line.startswith('bestmove'):
                parts = line.split()
                if len(parts) >= 2:
                    move = parts[1]
                    if move == "0000" or len(move) < 4:
                        print(f"Move {move_num}. White: ERROR - no valid move")
                        return False
                    print(f"Move {move_num}. White: {move}")
                    moves_white.append(move)
                    fen = apply_move(fen, move)
                    break
        else:
            print(f"Move {move_num}. White: ERROR - no bestmove")
            return False
        
        # Black's move
        cmd = f"uci\nposition fen {fen}\ngo depth 3\nquit\n"
        output = run_engine(cmd)
        
        for line in output.split('\n'):
            if line.startswith('bestmove'):
                parts = line.split()
                if len(parts) >= 2:
                    move = parts[1]
                    if move == "0000" or len(move) < 4:
                        print(f"Move {move_num}... Black: ERROR - no valid move")
                        return False
                    print(f"Move {move_num}... Black: {move}")
                    moves_black.append(move)
                    fen = apply_move(fen, move)
                    break
        else:
            print(f"Move {move_num}... Black: ERROR - no bestmove")
            return False
    
    print()
    print("=" * 60)
    print(f"GAME COMPLETED: {len(moves_white)} moves each")
    print("=" * 60)
    return True

def apply_move(fen, move):
    """Apply UCI move to FEN"""
    parts = fen.split()
    board = parts[0]
    side = parts[1]
    castling = parts[2]
    ep = parts[3]
    halfmove = int(parts[4])
    fullmove = int(parts[5])
    
    # Parse board to 2D
    board2d = []
    for row in board.split('/'):
        new_row = []
        for c in row:
            if c.isdigit():
                new_row.extend(['.'] * int(c))
            else:
                new_row.append(c)
        board2d.append(new_row)
    
    # Parse move
    from_file = ord(move[0]) - ord('a')
    from_rank = int(move[1]) - 1
    to_file = ord(move[2]) - ord('a')
    to_rank = int(move[3]) - 1
    
    # Convert UCI rank (0=rank1) to board index (7=rank1)
    from_idx = 7 - from_rank
    to_idx = 7 - to_rank
    
    piece = board2d[from_idx][from_file]
    
    # Move piece
    board2d[to_idx][to_file] = piece
    board2d[from_idx][from_file] = '.'
    
    # Switch side
    new_side = 'b' if side == 'w' else 'w'
    new_fullmove = fullmove + (1 if side == 'b' else 0)
    
    # Rebuild board string
    new_board = ''
    for row in board2d:
        empty = 0
        row_str = ''
        for c in row:
            if c == '.':
                empty += 1
            else:
                if empty > 0:
                    row_str += str(empty)
                    empty = 0
                row_str += c
        if empty > 0:
            row_str += str(empty)
        new_board += row_str + '/'
    new_board = new_board.rstrip('/')
    
    return f"{new_board} {new_side} {castling} {ep} {halfmove} {new_fullmove}"

if __name__ == "__main__":
    success = test_full_game()
    sys.exit(0 if success else 1)
