#!/usr/bin/env python3
"""Comprehensive test suite for Human Chess Engine"""

import subprocess
import sys

ENGINE = "./human-chess-engine"

def run_engine(cmd):
    """Run engine with given input and return output"""
    result = subprocess.run([ENGINE], input=cmd, capture_output=True, text=True, timeout=10)
    return result.stdout

def test_basic_uci():
    """Test basic UCI handshake"""
    output = run_engine("uci\nquit\n")
    assert "id name Human Chess Engine v0.1" in output, "Missing engine name"
    assert "uciok" in output, "Missing uciok"
    print("✓ Basic UCI handshake works")

def test_starting_position():
    """Test engine responds from starting position"""
    output = run_engine("uci\nposition startpos\ngo depth 2\nquit\n")
    assert "bestmove" in output, "No bestmove from starting position"
    print("✓ Starting position works")

def test_fen_parsing():
    """Test custom FEN parsing"""
    # Test after e4 e5
    output = run_engine("uci\nposition startpos moves e2e4 e7e5\ngo depth 2\nquit\n")
    assert "bestmove" in output, "No bestmove after e4 e5"
    print("✓ FEN parsing works")

def test_custom_fen():
    """Test custom FEN string"""
    # Scholar's mate position
    fen = "r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5Q2/PPPP1PPP/RNB1K1NR w KQkq - 4 4"
    output = run_engine(f"uci\nposition fen {fen}\ngo depth 2\nquit\n")
    assert "bestmove" in output, "No bestmove from custom FEN"
    print("✓ Custom FEN works")

def test_move_sequence():
    """Test multiple moves in sequence"""
    cmds = """uci
position startpos
go depth 2
position startpos moves g1f3
go depth 2
position startpos moves g1f3 g8f6
go depth 2
quit
"""
    output = run_engine(cmds)
    lines = [l for l in output.split('\n') if l.startswith('bestmove')]
    assert len(lines) >= 3, f"Expected at least 3 bestmoves, got {len(lines)}"
    print("✓ Move sequence works")

def test_display():
    """Test display command"""
    output = run_engine("uci\nposition startpos\nd\nquit\n")
    assert "FEN:" in output, "Missing FEN in display"
    assert "Legal moves:" in output, "Missing legal moves count"
    print("✓ Display works")

def main():
    print("=" * 50)
    print("Human Chess Engine - Test Suite")
    print("=" * 50)
    print()
    
    tests = [
        test_basic_uci,
        test_starting_position,
        test_fen_parsing,
        test_custom_fen,
        test_move_sequence,
        test_display,
    ]
    
    passed = 0
    failed = 0
    
    for test in tests:
        try:
            test()
            passed += 1
        except AssertionError as e:
            print(f"✗ {test.__name__}: {e}")
            failed += 1
        except Exception as e:
            print(f"✗ {test.__name__}: {e}")
            failed += 1
    
    print()
    print("=" * 50)
    print(f"Results: {passed} passed, {failed} failed")
    print("=" * 50)
    
    return failed == 0

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)
