#include "private/cli/commandmap.hpp"
#include <sstream>
#include <iomanip>
#include <memory>
#include <cstdlib>
#include <boost/io/ios_state.hpp>

using namespace shareddatalayer::cli;

namespace
{
    std::string buildCommandNameAlreadyRegisteredError(const std::string& commandName)
    {
        std::ostringstream os;
        os << "command name \"" << commandName << "\" already registered";
        return os.str();
    }

    std::string buildUnknownCommandNameError(const std::string& commandName)
    {
        std::ostringstream os;
        os << "unknown command: \"" << commandName << '\"';
        return os.str();
    }

    std::string buildCategoryOffsetAlreadyRegisteredError(const std::string& commandName, int offset)
    {
        std::ostringstream os;
        os << commandName << ": Offset " << offset << " already registered";
        return os.str();
    }

    std::string getCategoryName(CommandMap::Category category)
    {
        switch (category)
        {
            case CommandMap::Category::UTIL:
                return "Utility";
            default:
                return "Unknown";
        }
    }
}

CommandMap::CommandNameAlreadyRegistered::CommandNameAlreadyRegistered(const std::string& commandName):
    Exception(buildCommandNameAlreadyRegisteredError(commandName))
{
}

CommandMap::UnknownCommandName::UnknownCommandName(const std::string& commandName):
    Exception(buildUnknownCommandNameError(commandName))
{
}

CommandMap::CategoryOffsetAlreadyRegistered::CategoryOffsetAlreadyRegistered(const std::string& commandName, int offset):
    Exception(buildCategoryOffsetAlreadyRegisteredError(commandName, offset))
{
}

struct CommandMap::Info
{
    CommandFunction function;
    std::string shortHelp;
    std::string longHelp;
    boost::program_options::options_description options;

    Info(const CommandFunction& function,
         const std::string& shortHelp,
         const std::string& longHelp);
};

CommandMap::Info::Info(const CommandFunction& function,
                       const std::string& shortHelp,
                       const std::string& longHelp):
    function(function),
    shortHelp(shortHelp),
    longHelp(longHelp)
{
}

CommandMap::CommandMap()
{
}

CommandMap::~CommandMap()
{
}

boost::program_options::options_description&
CommandMap::registerCommand(const std::string& commandName,
                            const std::string& shortHelp,
                            const std::string& longHelp,
                            const CommandFunction& commandFunction,
                            Category category,
                            int categoryOffset)
{
    const auto ret(map.insert(std::make_pair(commandName, Info(commandFunction, shortHelp, longHelp))));
    if (!ret.second)
        throw CommandNameAlreadyRegistered(commandName);
    const auto retCat(categoryMap.insert(std::make_pair(CategoryKey(category, categoryOffset), commandName)));
    if (!retCat.second)
        throw CategoryOffsetAlreadyRegistered(commandName, categoryOffset);
    return ret.first->second.options;
}

std::vector<std::string> CommandMap::getCommandNames() const
{
    std::vector<std::string> ret;
    for (const auto& i : map)
        ret.push_back(i.first);
    return ret;
}

const boost::program_options::options_description&
CommandMap::getCommandOptions(const std::string& commandName) const
{
    const auto i(map.find(commandName));
    if (i == map.end())
        throw UnknownCommandName(commandName);
    return i->second.options;
}

void CommandMap::shortHelp(std::ostream& out) const
{
    boost::io::ios_all_saver guard(out);
    size_t maxWidth(0U);
    Category currentCategory(Category::UNDEFINED);
    for (const auto& i : map)
        if (maxWidth < i.first.size())
            maxWidth = i.first.size();
    for (const auto& i : categoryMap)
    {
        if (currentCategory != i.first.first)
        {
            currentCategory = i.first.first;
            out << std::endl;
            out << getCategoryName(currentCategory) << " commands:" << std::endl;
        }
        out << std::left << std::setw(maxWidth + 2U) << i.second << map.at(i.second).shortHelp << '\n';
    }
}

void CommandMap::longHelp(std::ostream& out, const std::string& commandName) const
{
    const auto i(map.find(commandName));
    if (i == map.end())
        throw UnknownCommandName(commandName);
    out << i->first;
    if (!i->second.options.options().empty())
    {
        out << " OPTIONS\n\n" << i->second.longHelp << "\n\nOptions:\n";
        i->second.options.print(out);
    }
    else
    {
        out << "\n\n" << i->second.longHelp << '\n';
    }
}

int CommandMap::execute(const std::string& commandName,
                        std::ostream& out,
                        std::ostream& err,
                        const boost::program_options::variables_map& params,
                        size_t count)
{
    const auto i(map.find(commandName));
    if (i == map.end())
    {
        err << "unknown command: \"" << commandName << '\"' << std::endl;
        return EXIT_FAILURE;
    }
    const auto& function(i->second.function);
    int ret(EXIT_SUCCESS);
    for (size_t i = 0U; i < count; ++i)
        if ((ret = function(out, err, params)) != EXIT_SUCCESS)
            break;
    return ret;
}

CommandMap& CommandMap::getCommandMap() noexcept
{
    static CommandMap instance;
    return instance;
}

