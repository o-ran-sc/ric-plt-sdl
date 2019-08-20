#include <iostream>
#include <exception>
#include <cstdlib>
#include "private/cli/commandmap.hpp"
#include "private/cli/commandparserandexecutor.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::cli;

int main(int argc, char** argv)
{
    try
    {
        return parseAndExecute(argc, argv, std::cout, std::cerr, CommandMap::getCommandMap());
    }
    catch (const std::exception& e)
    {
        std::cerr << "unexpected error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

