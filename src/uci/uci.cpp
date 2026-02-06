/**
 * UCI Protocol Implementation
 * Handles communication with chess GUIs
 */

#include "uci.hpp"
#include "../evaluation/evaluation.hpp"
#include "../search/search.hpp"
#include "../utils/board.hpp"
#include <iostream>
#include <sstream>
#include <vector>

namespace UCI {

Options options;
static std::string current_position = "";

// Forward declarations
void cmd_display();
void cmd_evaluate();

// Main UCI loop
void loop(int argc, char* argv[]) {
    std::string cmd;
    
    std::cout << "Human Chess Engine v0.1" << std::endl;
    std::cout << "Type 'uci' to enter UCI mode, 'quit' to exit." << std::endl;
    
    while (std::getline(std::cin, cmd)) {
        std::istringstream ss(cmd);
        std::string token;
        ss >> token;
        
        if (token == "uci") {
            cmd_uci();
        } else if (token == "isready") {
            cmd_is_ready();
        } else if (token == "quit") {
            break;
        } else if (token == "position") {
            std::vector<std::string> tokens;
            while (ss >> token) tokens.push_back(token);
            cmd_position(tokens);
        } else if (token == "go") {
            std::vector<std::string> tokens;
            while (ss >> token) tokens.push_back(token);
            cmd_go(tokens);
        } else if (token == "setoption") {
            std::vector<std::string> tokens;
            while (ss >> token) tokens.push_back(token);
            cmd_setoption(tokens);
        } else if (token == "stop") {
            cmd_stop();
        } else if (token == "d") {
            cmd_display();
        } else if (token == "eval") {
            cmd_evaluate();
        }
    }
}

// Display position for debugging
void cmd_display() {
    Board b;
    if (!current_position.empty()) {
        b.set_from_fen(current_position);
    } else {
        b.set_start_position();
    }
    std::cout << "FEN: " << b.get_fen() << std::endl;
    std::cout << "Side to move: " << (b.side_to_move == 0 ? "White" : "Black") << std::endl;
    auto moves = b.generate_moves();
    std::cout << "Legal moves: " << moves.size() << std::endl;
}

// Evaluate position
void cmd_evaluate() {
    int score = Evaluation::evaluate(current_position);
    std::cout << "Evaluation: " << score << " cp" << std::endl;
    
    auto exp = Evaluation::explain(score, current_position);
    std::cout << "Notes:" << std::endl;
    for (const auto& note : exp.move_reasons) {
        std::cout << "  - " << note << std::endl;
    }
    for (const auto& note : exp.imbalance_notes) {
        std::cout << "  - " << note << std::endl;
    }
}

// UCI protocol commands
void cmd_uci() {
    std::cout << "id name Human Chess Engine v0.1" << std::endl;
    std::cout << "id author Brendan & Jay" << std::endl;
    
    // UCI options
    std::cout << "option name PlayingStyle type combo default classical " <<
                 "var classical var attacking var tactical var positional var technical" << std::endl;
    std::cout << "option name SkillLevel type spin default 10 min 0 max 20" << std::endl;
    std::cout << "option name Hash type spin default 64 min 1 max 1024" << std::endl;
    std::cout << "option name Threads type spin default 1 min 1 max 32" << std::endl;
    std::cout << "option name UseMCTS type check default true" << std::endl;
    std::cout << "option name VerbalPV type check default false" << std::endl;
    std::cout << "option name ShowImbalances type check default false" << std::endl;
    
    std::cout << "uciok" << std::endl;
}

void cmd_is_ready() {
    std::cout << "readyok" << std::endl;
}

void cmd_position(const std::vector<std::string>& tokens) {
    std::string fen;
    
    // Handle different position commands
    if (tokens.size() >= 2 && tokens[1] == "startpos") {
        // Standard starting position
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    } else if (tokens.size() >= 2 && tokens[1] == "fen") {
        // Custom FEN - assemble from tokens[2-7]
        std::string fen_parts[6] = {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "KQkq", "-", "0", "1"};
        for (int i = 2; i < 8 && i < (int)tokens.size(); i++) {
            fen_parts[i-2] = tokens[i];
        }
        fen = fen_parts[0] + " " + fen_parts[1] + " " + fen_parts[2] + 
              " " + fen_parts[3] + " " + fen_parts[4] + " " + fen_parts[5];
    } else {
        // Default to starting position
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    }
    
    current_position = fen;
    
    // Parse moves if present (simplified - just ignore for now)
}

void cmd_go(const std::vector<std::string>& tokens) {
    int depth = 4;
    int movetime = 30000;
    bool infinite = false;
    
    // Parse go parameters
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == "depth" && i + 1 < tokens.size()) {
            depth = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "movetime" && i + 1 < tokens.size()) {
            movetime = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "infinite") {
            infinite = true;
            movetime = 3600000;  // 1 hour max for "infinite"
        }
    }
    
    // Perform search
    auto result = Search::search(current_position, movetime, depth);
    
    // Convert move to UCI notation
    std::string best_move_uci;
    if (result.best_move != 0) {
        best_move_uci = Bitboards::move_to_uci(result.best_move);
    } else {
        // Fallback: generate first legal move
        Board b;
        b.set_from_fen(current_position);
        auto moves = b.generate_moves();
        if (!moves.empty()) {
            best_move_uci = Bitboards::move_to_uci(moves[0]);
        } else {
            best_move_uci = "0000";  // No moves (stalemate/checkmate)
        }
    }
    
    // Output result
    std::cout << "info depth " << result.depth;
    std::cout << " score cp " << (result.score / 100);
    std::cout << " nodes " << result.nodes;
    std::cout << " time " << result.time_ms;
    std::cout << " pv " << best_move_uci << std::endl;
    
    std::cout << "bestmove " << best_move_uci << std::endl;
}

void cmd_setoption(const std::vector<std::string>& tokens) {
    if (tokens.size() < 4) return;
    
    std::string name, value;
    bool in_name = false, in_value = false;
    
    for (size_t i = 1; i < tokens.size(); i++) {
        if (tokens[i] == "name") {
            in_name = true; in_value = false;
        } else if (tokens[i] == "value") {
            in_name = false; in_value = true;
        } else if (in_name) {
            name += tokens[i] + " ";
        } else if (in_value) {
            value += tokens[i] + " ";
        }
    }
    
    if (!name.empty() && name.back() == ' ') name.pop_back();
    if (!value.empty() && value.back() == ' ') value.pop_back();
    
    if (name == "PlayingStyle") {
        Evaluation::set_style(value);
        options.playing_style = value;
    } else if (name == "SkillLevel") {
        options.skill_level = std::stoi(value);
    } else if (name == "Hash") {
        options.hash_size = std::stoi(value);
    } else if (name == "Threads") {
        options.threads = std::stoi(value);
        Search::set_threads(options.threads);
    } else if (name == "UseMCTS") {
        options.use_mcts = (value == "true");
        Search::set_use_mcts(options.use_mcts);
    } else if (name == "VerbalPV") {
        options.verbal_pv = (value == "true");
    } else if (name == "ShowImbalances") {
        options.show_imbalances = (value == "true");
    }
}

void cmd_stop() {
    Search::stop();
}

void cmd_quit() {}

} // namespace UCI
