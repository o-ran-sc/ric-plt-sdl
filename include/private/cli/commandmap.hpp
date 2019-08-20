#ifndef SHAREDDATALAYER_CLI_COMMANDMAP_HPP
#define SHAREDDATALAYER_CLI_COMMANDMAP_HPP

#include <string>
#include <functional>
#include <iosfwd>
#include <map>
#include <vector>
#include <cstddef>
#include <boost/program_options.hpp>
#include <sdl/exception.hpp>

namespace shareddatalayer
{
    namespace cli
    {
        class CommandMap
        {
        public:
            class CommandNameAlreadyRegistered;

            class UnknownCommandName;

            class CategoryOffsetAlreadyRegistered;

            enum class Category
            {
                UNDEFINED,
                UTIL,
            };

            CommandMap();

            ~CommandMap();

            using CommandFunction = std::function<int(std::ostream& out,
                                                      std::ostream& err,
                                                      const boost::program_options::variables_map& params)>;

            boost::program_options::options_description&
            registerCommand(const std::string& commandName,
                            const std::string& shortHelp,
                            const std::string& longHelp,
                            const CommandFunction& commandFunction,
                            Category category,
                            int categoryOffset);

            std::vector<std::string> getCommandNames() const;

            const boost::program_options::options_description&
            getCommandOptions(const std::string& commandName) const;

            int execute(const std::string& commandName,
                        std::ostream& out,
                        std::ostream& err,
                        const boost::program_options::variables_map& params,
                        size_t count);

            void shortHelp(std::ostream& out) const;

            void longHelp(std::ostream& out, const std::string& commandName) const;

            static CommandMap& getCommandMap() noexcept;

            CommandMap(const CommandMap&) = delete;
            CommandMap(CommandMap&&) = delete;
            CommandMap& operator = (const CommandMap&) = delete;
            CommandMap& operator = (CommandMap&&) = delete;

        private:
            struct Info;

            std::map<std::string, Info> map;
            using CategoryKey = std::pair<Category, int>;
            std::map<CategoryKey, std::string> categoryMap;
        };

        class CommandMap::CommandNameAlreadyRegistered: public Exception
        {
        public:
            explicit CommandNameAlreadyRegistered(const std::string& commandName);
        };

        class CommandMap::UnknownCommandName: public Exception
        {
        public:
            explicit UnknownCommandName(const std::string& commandName);
        };

        class CommandMap::CategoryOffsetAlreadyRegistered: public Exception
        {
        public:
            CategoryOffsetAlreadyRegistered(const std::string& commandName, int offset);
        };
    }
}

#define AUTO_REGISTER_COMMAND_XTOKENPASTE(x, y) x ## y
#define AUTO_REGISTER_COMMAND_TOKENPASTE(x, y) AUTO_REGISTER_COMMAND_XTOKENPASTE(x, y)

#define AUTO_REGISTER_COMMAND(_function, _name, _shortHelp, _longHelp, _category, _categoryOffset, ...) \
namespace                                                               \
{                                                                       \
    struct AUTO_REGISTER_COMMAND_TOKENPASTE(AutoRegister, __LINE__)     \
    {                                                                   \
        AUTO_REGISTER_COMMAND_TOKENPASTE(AutoRegister, __LINE__)()      \
        {                                                               \
            ::shareddatalayer::cli::CommandMap::getCommandMap().registerCommand(_name, _shortHelp, _longHelp, _function, _category, _categoryOffset).add_options() \
                __VA_ARGS__;                                            \
        }                                                               \
        ~AUTO_REGISTER_COMMAND_TOKENPASTE(AutoRegister, __LINE__)() { } \
    };                                                                  \
    AUTO_REGISTER_COMMAND_TOKENPASTE(AutoRegister, __LINE__) AUTO_REGISTER_COMMAND_TOKENPASTE(autoRegister, __LINE__); \
}

#endif


