#include "TextProcessor.h"

#include <cctype>

std::vector<std::string> TextProcessor::tokenize(const std::string& text) const
{
    std::vector<std::string> tokens;
    std::string currentToken;

    for (char character : text)
    {
        const unsigned char unsignedCharacter = static_cast<unsigned char>(character);

        if (std::isalnum(unsignedCharacter))
        {
            currentToken += static_cast<char>(std::tolower(unsignedCharacter));
        }
        else if (!currentToken.empty())
        {
            tokens.push_back(currentToken);
            currentToken.clear();
        }
    }

    if (!currentToken.empty())
    {
        tokens.push_back(currentToken);
    }

    return tokens;
}
