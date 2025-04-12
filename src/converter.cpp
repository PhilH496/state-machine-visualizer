#include "converter.hpp"
#include <stack>
#include <queue>
#include <set>
#include <map>
#include <cctype>

using namespace std;

//---------------------------------------------------------------------
// define precedence and associativity of operators
//---------------------------------------------------------------------
int Precedence(char op)
{
    if (op == '*')
        return 3; // highest precedence for Kleene star (*)
    if (op == '.')
        return 2; // concatenation (.) has lower precedence than *
    if (op == '|')
        return 1; // alternation (|) has the lowest precedence
    return 0;
}

//---------------------------------------------------------------------
// check if a character is an operand (alpha)
//---------------------------------------------------------------------
bool IsOperand(char c)
{
    return isalpha(c);
}

//---------------------------------------------------------------------
// convert an infix regular expression to postfix
//---------------------------------------------------------------------
string InfixToPostfix(const string& infix)
{
    stack<char> ops;     // stack for operators
    string postfix = ""; // resulting postfix expression 'queue'
    for (size_t i = 0; i < infix.size(); i++)
    {
        char c = infix[i];
        if (IsOperand(c))
        {
            // if c is an operand, put it on the output queue
            postfix += c;
        }
        else if (c == '(')
        {
            // if it's an opening parenthesis, push it onto the stack
            ops.push(c);
        }
        else if (c == ')')
        {
            // pop off the stack until matching '(' is found
            while (!ops.empty() && ops.top() != '(')
            {
                postfix += ops.top();
                ops.pop();
            }
            ops.pop(); // pop the '('
        }
        else if (c == '|')
        {
            // if it's an alternation operator, pop operators of higher precedence
            while (!ops.empty() && ops.top() != '(' && Precedence(ops.top()) >= Precedence(c))
            {
                postfix += ops.top();
                ops.pop();
            }
            ops.push(c); // push the alternation operator onto the stack
        }
        else if (c == '.')
        {
            // do same for concatenation (.)
            while (!ops.empty() && ops.top() != '(' && Precedence(ops.top()) >= Precedence(c))
            {
                postfix += ops.top();
                ops.pop();
            }
            ops.push(c); // push the . operator onto the stack
        }
        else if (c == '*')
        {
            // and for (*)
            ops.push(c); // push the * operator onto the stack
        }
    }
    // pop all remaining operators off the stack
    while (!ops.empty())
    {
        postfix += ops.top();
        ops.pop();
    }
    return postfix;
}

//---------------------------------------------------------------------
// convert a regular expression in postfix to an NFA using Thompson's construction
//---------------------------------------------------------------------
NFA PostfixToNFA(const string& postfix)
{
    stack<NFA> nfa_stack;
    int stateCount = 0;
    for (size_t i = 0; i < postfix.size(); i++)
    {
        char c = postfix[i];
        // Creates a transition on the input character to a final state
        if (IsOperand(c)) 
        {
            int start = stateCount++;
            int end = stateCount++;
            NFA myNFA({c}, start, {end});
            myNFA.AddTransition(start, {end}, c);

            nfa_stack.push(myNFA);
        }
        else if (c == '*')
        {
            NFA myNFA = nfa_stack.top();
            nfa_stack.pop();
        
            // Create new states
            int newStart = stateCount++;
            int newFinal = stateCount++;
        
            // Create a new NFA with the same alphabet
            NFA newNFA(myNFA.getAlpha(), newStart, {newFinal});
        
            // From new start state, add ε-transitions to:
            // 1. myNFA's initial state 
            // 2. New final state 
            newNFA.AddTransition(newStart, {myNFA.getInitState(), newFinal}, '_');
        
            // Merge the NFA into the new NFA
            newNFA.Merge(myNFA);
        
            // From myNFA's final states, add ε-transitions to:
            // 1. myNFA's initial state 
            // 2. New final state 
            for (int finalState : myNFA.getFinalStates())
            {
                newNFA.AddTransition(finalState, {myNFA.getInitState(), newFinal}, '_');
            }
        
            nfa_stack.push(newNFA);
        }
        else if (c == '.')
        {
        NFA nfa2 = nfa_stack.top();
        nfa_stack.pop();
        NFA nfa1 = nfa_stack.top();
        nfa_stack.pop();

        // Add ε-transitions from nfa1's final states to nfa2's initial state
        for (int finalState : nfa1.getFinalStates())
        {
            nfa1.AddTransition(finalState, {nfa2.getInitState()}, '_');
        }

        // Merge nfa2 into nfa1
        nfa1.Merge(nfa2);

        nfa1.setFinalStates(nfa2.getFinalStates());

        nfa_stack.push(nfa1);
    }
        else if (c == '|')
        {
            NFA nfa2 = nfa_stack.top();
            nfa_stack.pop();
            NFA nfa1 = nfa_stack.top();
            nfa_stack.pop();

            nfa1.ShiftStates(1);
            nfa2.ShiftStates(stateCount - 3);

            int newStart = 0;
            int newFinal = stateCount + 1;
            stateCount = newFinal + 1;

            // Create the new union NFA.
            NFA newNFA(nfa1.getAlpha(), newStart, {newFinal});

            // From the new start state, add ε-transitions to both NFAs initial states.
            newNFA.AddTransition(newStart, {nfa1.getInitState(), nfa2.getInitState()}, '_');

            // Merge the two NFAs into newNFA.
            newNFA.Merge(nfa1);
            newNFA.Merge(nfa2);

            // For each final state in nfa1/2, add an ε-transition to the new final state.
            for (int finalState : nfa1.getFinalStates())
            {
                newNFA.AddTransition(finalState, {newFinal}, '_');
            }
            for (int finalState : nfa2.getFinalStates())
            {
                newNFA.AddTransition(finalState, {newFinal}, '_');
            }

            nfa_stack.push(newNFA);
        }
    }
    return nfa_stack.top();
}

//---------------------------------------------------------------------
// convert a NFA to DFA using subset construction
//---------------------------------------------------------------------
DFA NFAtoDFA(const NFA& myNFA)
{
    map<int, map<char, set<int>>> Ntran = myNFA.getNFATransitions();
    int NFAStart = myNFA.getInitState();
    set<int> NFAFinals = myNFA.getFinalStates();
    set<char> alpha = myNFA.getAlpha();

    map<set<int>, map<char, set<int>>> Dtran; // DFA transition map
    map<set<int>, int> stateMapping;          // Maps NFA states to its corresponding DFA state
    queue<set<int>> unmarkedStates;           // States yet to be processed
    set<set<int>> Dstates;                    // DFA states

    // Create initial DFA state from NFA initial state 
    set<int> startSet = {NFAStart};
    set<int> startClosure = myNFA.EpsilonClosure(startSet);
    Dstates.insert(startClosure);
    unmarkedStates.push(startClosure);
    stateMapping[startClosure] = 0;

    int stateCount = 0;
    DFA myDFA(alpha, {0}, {});

    // Process all unmarked states
    while (!unmarkedStates.empty())
    {
        set<int> T = unmarkedStates.front();
        unmarkedStates.pop();

        // For each input symbol
        for (char a : alpha)
        {
            if (a == '_') { // Skip epsilon transitions
                continue;
            }

            set<int> U = myNFA.EpsilonClosure(myNFA.move(T, a)); // Get states reachable by the input symbol(using move(T,a)) then by epsilon
            if (!U.empty())
            {
                if (Dstates.count(U) == 0) // If the DFA set isn't in our set of DFAs, add it to be processed
                {
                    Dstates.insert(U);
                    unmarkedStates.push(U);
                    stateCount++;
                    stateMapping[U] = stateCount;
                }
                Dtran[T][a] = U;
            }
        }
    }

    // Convert state set transitions to DFA state numbers
    for (auto &row : Dtran)
    {
        int src = stateMapping[row.first];
        for (auto &trans : row.second)
        {
            int dst = stateMapping[trans.second];
            myDFA.AddTransition(src, dst, trans.first);
        }
    }

    set<int> DFAFinals;
    for (auto &entry : stateMapping)
    {
        const set<int> &stateSet = entry.first;
        int dfaStateID = entry.second;
        // For each final NFA state, make the DFA state final 
        for (int nfaState : stateSet)
        {
            if (NFAFinals.find(nfaState) != NFAFinals.end())
            {
                DFAFinals.insert(dfaStateID);
                break;
            }
        }
    }
    myDFA.setFinalStates(DFAFinals);
    return myDFA;
}

//---------------------------------------------------------------------
// helper method that calls the other methods to convert regex to DFA
//---------------------------------------------------------------------
DFA regexToDFA(const string& infix) {
    string postfix = InfixToPostfix(infix);
    NFA resultNFA = PostfixToNFA(postfix);
    DFA resultDFA = NFAtoDFA(resultNFA);
    return resultDFA;
}