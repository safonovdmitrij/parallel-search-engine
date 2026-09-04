#ifndef TEXTPROCESSOR_H
#define TEXTPROCESSOR_H

#include <string>
#include <vector>

class TextProcessor
{
public:
    std::vector<std::string> tokenize(const std::string& text) const;
};

#endif //TEXTPROCESSOR_H
