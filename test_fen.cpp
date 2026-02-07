#include <iostream>
#include <sstream>
#include "../src/utils/board.hpp"

int main() {
    Board b;
    std::string fen = "rnbqkbnr/pppppppp/8/8/8/5N2/PPPPPPPP/RNBQKB1R b KQkq - 0 1";
    std::cout << "Setting FEN: " << fen << std::endl;
    b.set_from_fen(fen);
    std::cout << "FEN after parsing: " << b.get_fen() << std::endl;
    return 0;
}
