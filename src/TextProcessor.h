#ifndef TEXTPROCESSOR_H
#define TEXTPROCESSOR_H

#include <string>
#include <vector>

#include "TermPositions.h"

class TextProcessor
{
public:
    std::vector<std::string> tokenize(const std::string& text) const;
    TermPositions collectTermPositions(const std::string& text) const;
};

#endif