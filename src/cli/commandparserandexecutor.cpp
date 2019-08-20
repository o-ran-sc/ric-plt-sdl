#include "config.h"
#include "private/cli/commandparserandexecutor.hpp"
#include <ostream>
#include <set>
#include <string>
#include <exception>
#include <boost/program_options.hpp>
#include "private/cli/commandmap.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::cli;
namespace po = boost::program_options;

namespace
{
    void outputBashCompletion(std::ostream& os,
                              const po::options_description& commonOpts,
                              CommandMap& commandMap)
    {
        const auto commandNames(commandMap.getCommandNames());
        std::set<std::string> optionNames;
        for (const auto& i : commonOpts.options())
            optionNames.insert(i->long_name());
        for (const auto& i : commandNames)
            for (const auto& j : commandMap.getCommandOptions(i).options())
                optionNames.insert(j->long_name());

        os << "_sdltool()\n{\n    COMPREPLY=()\n    local commands=\"";
        for (const auto& i : commandNames)
            os << i << ' ';
        os << "\"\n    local options=\"";
        for (const auto& i : optionNames)
            os << "--" << i << ' ';
        os <<
R"bash("
    local cur="${COMP_WORDS[COMP_CWORD]}"

    if [[ ${cur} == -* ]] ; then
        COMPREPLY=( $(compgen -W "${options}" -- ${cur}) )
        return 0
    else
        COMPREPLY=( $(compgen -W "${commands}" -- ${cur}) )
        return 0
    fi
}
complete -F _sdltool sdltool
)bash"
           << std::flush;
    }

    void printShortHelp(std::ostream& os,
                        const po::options_description& commonOpts,
                        CommandMap& commandMap)
    {
        os << "Usage: sdltool [OPTIONS] COMMAND [COMMAND SPECIFIC OPTIONS]\n\n";
        commonOpts.print(os);
        os << "\nAvailable commands:\n";
        commandMap.shortHelp(os);
    }

    int privateParseAndExecute(int argc,
                               char** argv,
                               std::ostream& out,
                               std::ostream& err,
                               CommandMap& commandMap)
    {
        po::options_description commonHiddenOptsDescription("Common hidden options");
        commonHiddenOptsDescription.add_options()
            ("pos-command", po::value<std::string>(), "sdltool command to run")
            ("pos-subargs", po::value<std::vector<std::string>>(), "Arguments for command");

        po::options_description commonVisibleOptsDescription("Common options");
        commonVisibleOptsDescription.add_options()
            ("help", "Show help for COMMAND")
            ("repeat", po::value<size_t>()->default_value(1U), "Times to repeat the command")
            ("bash-completion", "Generate bash completion script")
            ("version", "Show version information");

        po::options_description commonOptsDescription("All common options");
        commonOptsDescription.add(commonHiddenOptsDescription);
        commonOptsDescription.add(commonVisibleOptsDescription);

        po::positional_options_description commonPosOptsDescription;
        commonPosOptsDescription
            .add("pos-command", 1)
            .add("pos-subargs", -1); // all positional arguments after 'command'

        po::variables_map commonVarsMap;
        po::parsed_options commonParsed(po::command_line_parser(argc, argv)
                                        .options(commonOptsDescription)
                                        .positional(commonPosOptsDescription)
                                        .allow_unregistered()
                                        .run());
        po::store(commonParsed, commonVarsMap);
        po::notify(commonVarsMap);

        if (!commonVarsMap.count("pos-command"))
        {
            if (commonVarsMap.count("help"))
            {
                printShortHelp(out, commonVisibleOptsDescription, commandMap);
                return EXIT_SUCCESS;
            }
            if (commonVarsMap.count("bash-completion"))
            {
                outputBashCompletion(out, commonVisibleOptsDescription, commandMap);
                return EXIT_SUCCESS;
            }
            if (commonVarsMap.count("version"))
            {
                out << PACKAGE_STRING << std::endl;
                return EXIT_SUCCESS;
            }
            err << "missing command\n\n";
            printShortHelp(err, commonVisibleOptsDescription, commandMap);
            return EXIT_FAILURE;
        }

        auto leftOverOpts = po::collect_unrecognized(commonParsed.options, po::include_positional);
        if (!leftOverOpts.empty())
            leftOverOpts.erase(leftOverOpts.begin());

        const auto commandName(commonVarsMap["pos-command"].as<std::string>());

        if (commonVarsMap.count("help"))
        {
            bool found(false);
            try
            {
                commandMap.longHelp(out, commandName);
                out << '\n';
                found = true;
            }
            catch (const CommandMap::UnknownCommandName&)
            {
            }
            if (!found)
            {
                err << "unknown command: \"" << commandName << "\"\n";
                return EXIT_FAILURE;
            }
            return EXIT_SUCCESS;
        }

        const auto& commandOptions(commandMap.getCommandOptions(commandName));

        po::variables_map subcmdVarsMap;
        auto subcmdParsed(po::command_line_parser(leftOverOpts).options(commandOptions).run());
        po::store(subcmdParsed, subcmdVarsMap);
        po::notify(subcmdVarsMap);

        return commandMap.execute(commandName,
                                  out,
                                  err,
                                  subcmdVarsMap,
                                  commonVarsMap["repeat"].as<size_t>());
    }
}

int shareddatalayer::cli::parseAndExecute(int argc,
                                         char** argv,
                                         std::ostream& out,
                                         std::ostream& err,
                                         CommandMap& commandMap)
{
    try
    {
        return privateParseAndExecute(argc, argv, out, err, commandMap);
    }
    catch (const std::exception& e)
    {
        err << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

