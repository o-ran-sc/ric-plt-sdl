#include <ostream>
#include <cstdlib>
#include "private/cli/commandmap.hpp"
#include "private/cli/common.hpp"
#include <sdl/syncstorage.hpp>

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

    void get(shareddatalayer::SyncStorage& sdl,
             const SyncStorage::Namespace& ns,
             const SyncStorage::Key& key,
             std::ostream& out)
    {
        try
        {
            auto data(sdl.get(ns, {key}));
            out << data << std::endl;
        }
        catch (const shareddatalayer::Exception& error)
        {
            out << "get(" << ns << ", " << key << ") failed: "
                << error.what() << std::endl;
        }
    }

    int getCommand(std::ostream& out, const boost::program_options::variables_map& map)
    {
        auto ns(map["ns"].as<std::string>());
        auto key(map["key"].as<std::string>());

        auto sdl(createSyncStorage(ns, out));
        if (nullptr == sdl)
            return EXIT_FAILURE;
        sdl->setOperationTimeout(std::chrono::seconds(5));

        get(std::ref(*sdl), ns, key, out);

        return EXIT_SUCCESS;
    }
}

const char *longHelpGetCmd =
        "Use get SDL API to read data from storage under the namespace.\n\n"
        "Example: sdltool get --ns 'sdltool' --key 'key1'";

AUTO_REGISTER_COMMAND(std::bind(getCommand, std::placeholders::_1, std::placeholders::_3),
                      "get",
                      "get data with SDL API under the namespace",
                      longHelpGetCmd,
                      CommandMap::Category::UTIL,
                      30050,
                      ("ns,n", boost::program_options::value<std::string>()->default_value("sdltoolns"), "namespace to use")
                      ("key,k", boost::program_options::value<std::string>()->default_value("key1"), "key value")
                     );

