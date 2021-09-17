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

    void set(shareddatalayer::SyncStorage& sdl,
             const SyncStorage::Namespace& ns,
             const SyncStorage::DataMap& dataMap,
             std::ostream& out)
    {
        try
        {
            sdl.set(ns, dataMap);
            out << "set {" << ns << "}," << dataMap << std::endl;
        }
        catch (const shareddatalayer::Exception& error)
        {
            out << "Error in set(" << ns << ", " << dataMap << ") failed: "
                << error.what() << std::endl;
        }
    }

    int setCommand(std::ostream& out, const boost::program_options::variables_map& map)
    {
        auto ns(map["ns"].as<std::string>());
        auto key(map["key"].as<std::string>());
        auto data(map["data"].as<std::string>());
        auto debug(map["debug"].as<bool>());
        SyncStorage::Data dataVector(data.begin(), data.end());

        auto sdl(createSyncStorage(ns, out));
        if (nullptr == sdl)
            return EXIT_FAILURE;
        sdl->setOperationTimeout(std::chrono::seconds(5));

        if (debug)
        {
            out << "DEBUG setCommand ns: " << ns
                << " key: " << key << " data: " << data << std::endl;
        }

        SyncStorage::DataMap datamap({{key, dataVector}});
        set(std::ref(*sdl), ns, datamap, out);

        return EXIT_SUCCESS;
    }
}

const char *longHelpSetCmd =
    "Use set SDL API to write data to storage under the namespace.\n\n"
    "Example: sdltool set --ns 'sdltool' --key 'key1' --data '1'";

AUTO_REGISTER_COMMAND(std::bind(setCommand, std::placeholders::_1, std::placeholders::_3),
                      "set",
                      "set key value with SDL API",
                      longHelpSetCmd,
                      CommandMap::Category::UTIL,
                      30040,
                      ("ns,n", boost::program_options::value<std::string>()->default_value("sdltoolns"), "namespace to use")
                      ("key,k", boost::program_options::value<std::string>()->default_value("key1"), "key value")
                      ("data,d", boost::program_options::value<std::string>()->default_value("1"), "data")
                      ("debug", boost::program_options::bool_switch()->default_value(false), "Enable debug logs")
                     );

