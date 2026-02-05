# Human Chess Engine ♟️

A human-like chess engine that thinks like a coach, not a calculator.

## Philosophy

Unlike Stockfish which plays perfect chess through brute-force search, this engine:
- **Evaluates positions like a human** using Silman's Imbalance Theory
- **Thinks strategically** using Russian School methodology  
- **Calculates selectively** using MCTS for human-like move selection
- **Explains its moves** with verbal PV output for teaching

## Key Features

### 🎯 Human-Like Evaluation
- Imbalance-based evaluation (material, piece activity, pawn structure, space, initiative)
- Style-tunable weights (Classical, Attacking, Tactical, Positional, etc.)
- Prophylactic thinking from Russian Chess School

### 🧠 Teaching Focus
- Verbal PV output explaining WHY each move is played
- Imbalance analysis in comments
- Pattern recognition annotations
- Skill levels for students

### ⚡ Modern Architecture
- C++ for performance
- MCTS or Alpha-Beta search (selectable)
- UCI protocol compatible
- Style profiles via UCI options

## Architecture

```
human-chess-engine/
├── src/
│   ├── evaluation/     # Custom Silman-based evaluation
│   ├── search/         # MCTS or alpha-beta
│   ├── uci/            # UCI protocol
│   └── utils/          # Board representation
├── data/
│   └── opening_book/   # Human-style opening prep
├── docs/
│   └── style_guide.md  # Evaluation philosophy
└── Makefile
```

## Knowledge Base

Built on concepts from:
- Jeremy Silman - "How to Reassess Your Chess"
- Konstantin Sakaev - "Russian Chess School Vol 1 & 2"
- M.I. Shereshevsky - "Endgame Strategy"
- Alexander Kotov - "Play Like a Grandmaster"
- Vladimir Vukovic - "The Art of Attack"
- Neil McDonald - "Positional Sacrifices"
- Ivan Sokolov - "Sacrifice and Initiative"

## Roadmap

```
Phase 1: Basic evaluation function (Silman imbalances)
Phase 2: Search implementation (MCTS or alpha-beta)
Phase 3: UCI protocol integration
Phase 4: Style profiles via UCI options
Phase 5: Verbal PV output
Phase 6: Self-play training (future NNUE)
```

## License

MIT License - Open source for the chess community!

## Author

Built with ❤️ by Brendan and Jay

---

*"Chess is a conversation with the board. Our engine teaches you how to listen."*
