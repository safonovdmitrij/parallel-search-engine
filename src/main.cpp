#include <iostream>
#include <string>

#include "OllamaClient.h"

#include <iostream>

int main()
{
    OllamaClient client;

    std::string response = client.generate(
            "Explain why the sky is blue in one paragraph"
    );

    std::cout << response << std::endl;

    return 0;
}
