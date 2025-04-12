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