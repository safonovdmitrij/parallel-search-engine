#ifndef TERMPOSITIONS_H
#define TERMPOSITIONS_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

using TokenPosition = std::uint32_t;

using TermPositions = std::unordered_map<std::string, std::vector<TokenPosition>>;

#endif