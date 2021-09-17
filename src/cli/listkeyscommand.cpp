#include <ostream>
#include <cstdlib>
#include "private/cli/commandmap.hpp"
#include "private/cli/common.hpp"
#include <sdl/syncstorage.hpp>
#include <sdl/asyncstorage.hpp>

using namespace shareddatalayer;
using namespace shareddatalayer::cli;

namespace
{
    std::shared_ptr<shareddatalayer::SyncStorage> createSyncStorage(const SyncStorage::Namespace& ns,
                                                                    std::ostream& out)
    {
        try
        {
            auto sdl(shareddatalayer::SyncStorage::create());
            sdl->waitReady(ns, std::chrono::minutes(1));
            return sdl;
        }
        catch (const shareddatalayer::Exception& error)
        {
            out << "SyncStorage create failed: " << error.what() << std::endl;
        }

        return nullptr;
    }

    void listkeys(shareddatalayer::SyncStorage& sdl,
                  const SyncStorage::Namespace& ns,
                  const std::string& pattern,
                  std::ostream& out)
    {
        try
        {
            auto keys(sdl.listKeys(ns, pattern));
            for (auto key: keys)
                out << key << std::endl;
        }
        catch (const shareddatalayer::Exception& error)
        {
            out << "listKeys(" << ns << ", " << pattern << ") failed: "
                << error.what() << std::endl;
        }
    }

    int listKeysCommand(std::ostream& out, const boost::program_options::variables_map& map)
    {
        auto ns(map["ns"].as<std::string>());
        auto pattern(map["pattern"].as<std::string>());

        auto sdl(createSyncStorage(ns, out));
        if (nullptr == sdl)
            return EXIT_FAILURE;
        sdl->setOperationTimeout(std::chrono::seconds(5));

        listkeys(std::ref(*sdl), ns, pattern, out);

        return EXIT_SUCCESS;
    }
}

const char *longHelpListkeysCmd =
    "Use listKeys SDL API to list keys matching search pattern under the namespace.\n\n"
    "Example: sdltool listkeys --ns 'sdltool' --pattern 'foo*'";

AUTO_REGISTER_COMMAND(std::bind(listKeysCommand, std::placeholders::_1, std::placeholders::_3),
                      "listkeys",
                      "listKeys SDL API",
                      longHelpListkeysCmd,
                      CommandMap::Category::UTIL,
                      30030,
                      ("ns,n", boost::program_options::value<std::string>()->default_value("sdltoolns"), "namespace to use")
                      ("pattern,p", boost::program_options::value<std::string>()->default_value("*"), "key search pattern")
                     );

