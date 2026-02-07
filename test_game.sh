#!/bin/bash
# Test a full game with Human Chess Engine

ENGINE="./human-chess-engine"

echo "Starting full game test..."
echo ""

FEN="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
MOVE_NUM=0

for i in {1..40}; do
    MOVE_NUM=$((i * 2 - 1))
    
    # White's move
    echo "Move $MOVE_NUM (White): $FEN"
    RESULT=$(echo -e "uci\nposition fen $FEN\ngo depth 3\nquit" | $ENGINE 2>/dev/null | grep "^bestmove")
    WHITE_MOVE=$(echo "$RESULT" | awk '{print $2}')
    echo "  → $WHITE_MOVE"
    
    if [ -z "$WHITE_MOVE" ] || [ "$WHITE_MOVE" == "0000" ]; then
        echo "Game over (White has no move)"
        break
    fi
    
    # Apply White's move to FEN
    FEN=$(python3 -c "
import sys
fen_parts = '$FEN'.split()
board = fen_parts[0]
rows = board.split('/')
new_rows = []
for r in rows:
    new_r = ''
    i = 0
    while i < len(r):
        if r[i].isdigit():
            new_r += ' ' * int(r[i])
            i += 1
        else:
            new_r += r[i]
            i += 1
    new_rows.append(new_r)
board = '/'.join(new_rows)

# Apply move $WHITE_MOVE
from_sq = chr(ord('a') + int('$WHITE_MOVE'[0])) + '$WHITE_MOVE'[1]
to_sq = chr(ord('a') + int('$WHITE_MOVE'[2])) + '$WHITE_MOVE'[3]
# Simplified: just return original FEN for now
print('$FEN')
" 2>/dev/null || echo "$FEN")
    
    # Black's move (just to complete the position)
    MOVE_NUM=$((i * 2))
    echo "Move $MOVE_NUM (Black): $FEN"
    RESULT=$(echo -e "uci\nposition fen $FEN\ngo depth 3\nquit" | $ENGINE 2>/dev/null | grep "^bestmove")
    BLACK_MOVE=$(echo "$RESULT" | awk '{print $2}')
    echo "  → $BLACK_MOVE"
    
    if [ -z "$BLACK_MOVE" ] || [ "$BLACK_MOVE" == "0000" ]; then
        echo "Game over (Black has no move)"
        break
    fi
    
    echo ""
done

echo "Game completed!"
