#include "Interpreter.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [file.aul]\n";
    std::cout << "       " << programName << " (interactive mode)\n";
}

int main(int argc, char* argv[]) {
    Interpreter interp;

    try {
        if (argc > 1) {
            // Load and execute AUL file
            std::string filename = argv[1];
            std::cout << "Loading: " << filename << std::endl;
            interp.evaluateFile(filename);
        } else {
            // Interactive mode / REPL
            std::cout << "AUL - A Universal Language\n";
            std::cout << "Type 'exit' or Ctrl+C to quit\n\n";
            
            std::string line;
            std::stringstream buffer;
            bool inMultilineBlock = false;
            int braceCount = 0;
            
            while (true) {
                std::cout << (inMultilineBlock ? "  > " : "> ");
                if (!std::getline(std::cin, line)) break;
                
                if (line == "exit") break;
                
                // Count braces to detect multi-line blocks
                for (char c : line) {
                    if (c == '{') braceCount++;
                    else if (c == '}') braceCount--;
                }
                
                buffer << line << "\n";
                
                if (braceCount == 0 && !line.empty()) {
                    // Try to evaluate
                    try {
                        interp.evaluate(buffer.str());
                        buffer.str("");
                        buffer.clear();
                        inMultilineBlock = false;
                    } catch (const std::exception& e) {
                        std::cerr << "Error: " << e.what() << std::endl;
                        buffer.str("");
                        buffer.clear();
                        braceCount = 0;
                        inMultilineBlock = false;
                    }
                } else if (braceCount > 0) {
                    inMultilineBlock = true;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
