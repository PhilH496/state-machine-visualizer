#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include "../include/NFA.h"
#include "../include/DFA.h"
#include "converter.hpp"
#include <nlohmann/json.hpp>
#include <cctype>

using json = nlohmann::json;
using namespace std;

// Helper function to convert state numbers to q{state}
auto stateToString(int state) {
    return "q" + to_string(state);
}

// Helper function to convert NFA to JSON
json nfaToJson(const NFA& nfa) {
    json result;
    
    // Get all states
    set<int> allStates;
    map<int, map<char, set<int>>> transitions = nfa.getNFATransitions();
    
    // Collect all states from transitions
    for (const auto& row : transitions) {
        allStates.insert(row.first);
        for (const auto& trans : row.second) {
            for (int state : trans.second) {
                allStates.insert(state);
            }
        }
    }
    // Add initial and final states
    allStates.insert(nfa.getInitState());
    for (int state : nfa.getFinalStates()) {
        allStates.insert(state);
    }
    
    // Convert states to string array
    vector<int> sortedStates(allStates.begin(), allStates.end());

    vector<string> stateStrings;
    for (int state : sortedStates) {
        stateStrings.push_back(stateToString(state));
    }
    result["states"] = stateStrings;
    
    // Convert transitions
    json transObj;
    for (const auto& row : transitions) {
        string srcState = stateToString(row.first);
        json srcTrans;
        for (const auto& trans : row.second) {
            char sym = trans.first;
            string symStr = (sym == '_') ? "ε" : string(1, sym);
            if (trans.second.size() == 1) {
                srcTrans[symStr] = stateToString(*trans.second.begin());
            } else {
                // Multiple destinations - represent as array
                vector<string> dests;
                for (int dest : trans.second) {
                    dests.push_back(stateToString(dest));
                }
                srcTrans[symStr] = dests;
            }
        }
        if (!srcTrans.empty()) {
            transObj[srcState] = srcTrans;
        }
    }
    result["transitions"] = transObj;
    
    // Start state
    result["start"] = stateToString(nfa.getInitState());
    
    // Accept states
    vector<string> acceptStates;
    for (int state : nfa.getFinalStates()) {
        acceptStates.push_back(stateToString(state));
    }
    result["accept"] = acceptStates;
    
    return result;
}

// Helper function to convert DFA to JSON
json dfaToJson(const DFA& dfa) {
    json result;
    
    // Get all states
    set<int> allStates;
    map<int, map<char, int>> transitions = dfa.getDFATransitions();
    
    // Collect all states from transitions
    for (const auto& row : transitions) {
        allStates.insert(row.first);
        for (const auto& trans : row.second) {
            allStates.insert(trans.second);
        }
    }
    // Add initial and final states
    set<int> initStates = dfa.getInitStates();
    for (int state : initStates) {
        allStates.insert(state);
    }
    set<int> finalStates = dfa.getFinalStates();
    for (int state : finalStates) {
        allStates.insert(state);
    }

    // Sort states for consistent mapping
    vector<int> sortedStates(allStates.begin(), allStates.end());
    
    vector<string> stateStrings;
    for (int state : sortedStates) {
        stateStrings.push_back(stateToString(state));
    }
    result["states"] = stateStrings;
    
    // Convert transitions
    json transObj;
    for (const auto& row : transitions) {
        string srcState = stateToString(row.first);
        json srcTrans;
        for (const auto& trans : row.second) {
            char sym = trans.first;
            string symStr = string(1, sym);
            string dstState = stateToString(trans.second);
            srcTrans[symStr] = dstState;
        }
        if (!srcTrans.empty()) {
            transObj[srcState] = srcTrans;
        }
    }
    result["transitions"] = transObj;
    
    // Start state
    if (!initStates.empty()) {
        result["start"] = stateToString(*initStates.begin());
    } else {
        result["start"] = "";
    }
    
    // Accept states
    vector<string> acceptStates;
    for (int state : finalStates) {
        acceptStates.push_back(stateToString(state));
    }
    result["accept"] = acceptStates;
    
    return result;
}

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " ../inputs/input.txt" << endl;
        return 1;
    }
    
    string inputFile = argv[1];
    
    // Read regex from input file
    ifstream file(inputFile);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << inputFile << endl;
        return 1;
    }
    
    string regex;
    getline(file, regex);
    file.close();
    
    // trim whitespace
    regex.erase(std::remove_if(regex.begin(), regex.end(), ::isspace), regex.end());
    
    if (regex.empty()) {
        cerr << "Error: Empty regex in input file" << endl;
        return 1;
    }
    
    try {
        // convert regex to DFA
        string postfix = InfixToPostfix(regex);
        NFA nfa = PostfixToNFA(postfix);
        DFA dfa = NFAtoDFA(nfa);
        
        // create JSON output
        json output;
        output["nfa"] = nfaToJson(nfa);
        output["dfa"] = dfaToJson(dfa);
        
        ofstream outFile;
        string outputPath = "resources/output.json";
        outFile.open(outputPath);
        if (!outFile.is_open()) {
            outputPath = "../resources/output.json";
            outFile.open(outputPath);
            if (!outFile.is_open()) {
                cerr << "Error: Could not create output.json" << endl;
                return 1;
            }
        }
        outFile << output.dump(2);
        cout << "Regex converted. Results saved to " << outputPath << endl;
    } catch (exception e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}