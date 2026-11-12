#include "TextProcessor.h"

#include <cctype>
#include <limits>
#include <stdexcept>

namespace
{
    template<typename Consumer>
    void processTokens(const std::string& text, Consumer consume)
    {
        std::string token;

        for (unsigned char character : text)
        {
            if (std::isalnum(character))
            {
                token += static_cast<char>(std::tolower(character));
            }
            else if (!token.empty())
            {
                consume(token);
                token.clear();
            }
        }

        if (!token.empty())
        {
            consume(token);
        }
    }
}

std::vector<std::string> TextProcessor::tokenize(const std::string& text) const
{
    std::vector<std::string> tokens;

    processTokens(text, [&tokens](const std::string& token)
    {
        tokens.push_back(token);
    });

    return tokens;
}

TermPositions TextProcessor::collectTermPositions(const std::string& text) const
{
    TermPositions positions;
    std::uint64_t position = 0;

    processTokens(text, [&positions, &position](const std::string& token)
    {
        if (position > std::numeric_limits<TokenPosition>::max())
        {
            throw std::overflow_error("Document contains too many tokens for 32-bit positions.");
        }

        positions[token].push_back(static_cast<TokenPosition>(position));
        ++position;
    });

    return positions;
}