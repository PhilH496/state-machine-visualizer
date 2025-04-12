#ifndef CONVERTER_HPP
#define CONVERTER_HPP

#include <string>
#include "../include/NFA.h"
#include "../include/DFA.h"

std::string InfixToPostfix(const std::string& infix);
NFA PostfixToNFA(const std::string& postfix);
DFA NFAtoDFA(const NFA& nfa);
DFA regexToDFA(const std::string& infix);

#endif