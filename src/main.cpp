#include <iostream>
#include <string>

#include "OllamaClient.h"
#include "DataLoader.h"

#include <iostream>

int main()
{
    OllamaClient client;

    std::string response = client.expandQuery("ocean pollution");
    std::cout << response << std::endl;

    return 0;
}
