/**
 * UCI Protocol Implementation
 * Handles communication with chess GUIs
 */

#include "uci.hpp"
#include "../evaluation/evaluation.hpp"
#include "../search/search.hpp"
#include <iostream>
#include <sstream>
#include <thread>

namespace UCI {

Options options;
static std::string current_position = "";

// Main UCI loop
void loop(int argc, char* argv[]) {
    std::string cmd;
    
    std::cout << "Human Chess Engine v0.1" << std::endl;
    std::cout << "Type 'uci' to enter UCI mode, 'quit' to exit." << std::endl;
    
    while (std::getline(std::cin, cmd)) {
        // Parse command
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
            // Debug: display position
            std::cout << "Position: " << current_position << std::endl;
        } else if (token == "eval") {
            // Debug: evaluate position
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
    }
}

// Parse UCI commands
void parse_command(const std::string& cmd) {
    std::istringstream ss(cmd);
    std::string token;
    ss >> token;
    
    if (token == "position") {
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
    if (tokens.size() < 2) return;
    
    size_t idx = 1;
    std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    
    if (tokens[1] == "startpos") {
        // Standard starting position
    } else if (tokens[1] == "fen") {
        // Custom FEN
        std::string fen_parts[6];
        for (int i = 0; i < 6 && idx < tokens.size(); i++) {
            fen_parts[i] = tokens[idx++];
        }
        fen = fen_parts[0] + " " + fen_parts[1] + " " + fen_parts[2] + 
              " " + fen_parts[3] + " " + fen_parts[4] + " " + fen_parts[5];
    }
    
    current_position = fen;
    
    // Parse moves if present
    if (idx < tokens.size() && tokens[idx] == "moves") {
        // Store moves for future use
    }
}

void cmd_go(const std::vector<std::string>& tokens) {
    int depth = 10;
    int nodes = 0;
    int movetime = 30000; // 30 seconds default
    bool infinite = false;
    
    // Parse go parameters
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == "depth" && i + 1 < tokens.size()) {
            depth = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "nodes" && i + 1 < tokens.size()) {
            nodes = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "movetime" && i + 1 < tokens.size()) {
            movetime = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "infinite") {
            infinite = true;
        }
    }
    
    // Perform search
    auto result = Search::search(current_position, movetime);
    
    // Output result
    std::cout << "info depth " << result.depth;
    std::cout << " score cp " << result.score;
    std::cout << " nodes " << result.nodes;
    std::cout << " time " << result.time_ms;
    std::cout << " pv " << result.best_move << std::endl;
    
    // Verbal PV if enabled
    if (options.verbal_pv) {
        auto exp = Evaluation::explain(result.score, current_position);
        std::cout << "comment ";
        for (size_t i = 0; i < exp.move_reasons.size(); i++) {
            if (i > 0) std::cout << " | ";
            std::cout << exp.move_reasons[i];
        }
        std::cout << std::endl;
    }
}

void cmd_setoption(const std::vector<std::string>& tokens) {
    if (tokens.size() < 4) return;
    
    std::string name, value;
    bool in_name = false;
    bool in_value = false;
    
    for (size_t i = 1; i < tokens.size(); i++) {
        if (tokens[i] == "name") {
            in_name = true;
            in_value = false;
        } else if (tokens[i] == "value") {
            in_name = false;
            in_value = true;
        } else if (in_name) {
            name += tokens[i] + " ";
        } else if (in_value) {
            value += tokens[i] + " ";
        }
    }
    
    // Remove trailing spaces
    if (!name.empty() && name.back() == ' ') name.pop_back();
    if (!value.empty() && value.back() == ' ') value.pop_back();
    
    // Set options
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

void cmd_quit() {
    // Cleanup
}

} // namespace UCI
