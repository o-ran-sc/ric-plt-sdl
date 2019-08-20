#ifndef SHAREDDATALAYER_CLI_COMMANDPARSERANDEXECUTOR_HPP
#define SHAREDDATALAYER_CLI_COMMANDPARSERANDEXECUTOR_HPP

#include <iosfwd>

namespace shareddatalayer
{
    namespace cli
    {
        class CommandMap;

        int parseAndExecute(int argc,
                            char** argv,
                            std::ostream& out,
                            std::ostream& err,
                            CommandMap& commandMap);
    }
}

#endif

