#include <ostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <chrono>
#include <arpa/inet.h>
#include <sdl/asyncstorage.hpp>
#include <boost/asio.hpp>
#include <thread>
#include "private/cli/commandmap.hpp"
#include "private/configurationpaths.hpp"
#include "private/createlogger.hpp"
#include "private/engineimpl.hpp"
#include "private/databaseconfigurationimpl.hpp"
#include "private/configurationreader.hpp"
#include "private/redis/databaseinfo.hpp"
#include "private/asyncstorageimpl.hpp"
#include "private/redis/asyncredisstorage.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::cli;
using namespace shareddatalayer::redis;

namespace
{
    void handler(std::shared_ptr<shareddatalayer::AsyncStorage> sdl, boost::asio::posix::stream_descriptor& sd)
    {
        sdl->handleEvents();
        sd.async_read_some(boost::asio::null_buffers(), std::bind(handler, sdl, std::ref(sd)));
    }

    std::shared_ptr<AsyncStorage> createStorage(const std::string& nsStr, std::ostream& out)
    {
        try
        {
        	std::shared_ptr<AsyncStorage> sdl(AsyncStorage::create());
            boost::asio::io_service ios;
            boost::asio::posix::stream_descriptor sd(ios);
            sd.assign(sdl->fd());
            sd.async_read_some(boost::asio::null_buffers(), std::bind(handler, sdl, std::ref(sd)));
            sdl->waitReadyAsync(nsStr, [&ios](const std::error_code& error)
                                {
                                    if (error)
                                        std::cerr << "SDL waitReadyAsync failed. Error:\n" << error.message() << std::endl;
                                    ios.stop();
                                });
            ios.run();
            sd.release();
            out << "Storage to namespace " << nsStr << " created." << std::endl;
            return sdl;
        }
        catch (const shareddatalayer::Exception& error)
        {
            out << "Storage create failed: " << error.what() << std::endl;
        }
        return nullptr;
    }

    std::string getHosts(const DatabaseConfiguration::Addresses& databaseAddresses)
    {
        std::string hosts("");
        for (auto i(databaseAddresses.begin()); i != databaseAddresses.end(); ++i)
            hosts = hosts + i->getHost() + " ";
        return hosts;
    }

    std::string getPorts(const DatabaseConfiguration::Addresses& databaseAddresses)
    {
        std::string ports("");
        for (auto i(databaseAddresses.begin()); i != databaseAddresses.end(); ++i)
            ports = ports + std::to_string(ntohs(i->getPort())) + " ";
        return ports;
    }

    void PrintEnvironmentVariable(std::ostream& out, std::string name)
    {
        const auto var(name.c_str());
        const auto conf(getenv(var));
        if (conf != nullptr)
            out << var  << ": " << conf << std::endl;
    }

    void PrintStaticConfiguration(std::ostream& out)
    {
        auto engine(std::make_shared<EngineImpl>());
        DatabaseConfigurationImpl databaseConfigurationImpl;
        ConfigurationReader configurationReader(createLogger(SDL_LOG_PREFIX));
        configurationReader.readDatabaseConfiguration(databaseConfigurationImpl);
        auto staticAddresses(databaseConfigurationImpl.getServerAddresses());
        auto defaultAddresses(databaseConfigurationImpl.getDefaultServerAddresses());
        auto staticDbType(databaseConfigurationImpl.getDbType());
        if (!staticAddresses.empty())
        {
            out << "\nStatic Server Addresses:" << std::endl;
            out << "Static Host: " << getHosts(staticAddresses) << std::endl;
            out << "Static Port: " << getPorts(staticAddresses) << std::endl;
            if (staticDbType == DatabaseConfiguration::DbType::REDIS_CLUSTER)
                out << "Static DB type: redis-cluster" << std::endl;
            else if (staticDbType == DatabaseConfiguration::DbType::REDIS_STANDALONE)
                out << "Static DB type: redis-standalone" << std::endl;
            else if (staticDbType == DatabaseConfiguration::DbType::REDIS_SENTINEL)
                out << "Static DB type: redis-sentinel" << std::endl;
            else
                out << "Static DB type not defined" << std::endl;
        }
        if (!defaultAddresses.empty() && staticAddresses.empty())
        {
            out << "\nDefault Server Addresses:" << std::endl;
            out << "Default Host: " << getHosts(defaultAddresses) << std::endl;
            out << "Default Port: " << getPorts(defaultAddresses) << std::endl;
        }
        PrintEnvironmentVariable(out, DB_HOST_ENV_VAR_NAME);
        PrintEnvironmentVariable(out, DB_PORT_ENV_VAR_NAME);
        PrintEnvironmentVariable(out, SENTINEL_PORT_ENV_VAR_NAME);
        PrintEnvironmentVariable(out, SENTINEL_MASTER_NAME_ENV_VAR_NAME);
    }

    void PrintDatabaseInfo(const DatabaseInfo& databaseInfo, std::ostream& out)
    {
        out << "Used database configuration (databaseInfo):" << std::endl;
        out << "Host: " << getHosts(databaseInfo.hosts) << std::endl;
        out << "Port: " << getPorts(databaseInfo.hosts) << std::endl;
        switch (databaseInfo.type)
        {
            case DatabaseInfo::Type::SINGLE:
                out << "Database type: SINGLE" << std::endl;
                break;
            case DatabaseInfo::Type::REDUNDANT:
                out << "Database type: REDUNDANT" << std::endl;
                break;
            case DatabaseInfo::Type::CLUSTER:
                out << "Database type: CLUSTER" << std::endl;
                break;
        }
        switch (databaseInfo.discovery)
        {
            case DatabaseInfo::Discovery::HIREDIS:
                out << "Discovery type:: HIREDIS" << std::endl;
                PrintStaticConfiguration(out);
                break;
            case DatabaseInfo::Discovery::SENTINEL:
                out << "Discovery type:: SENTINEL" << std::endl;
                PrintStaticConfiguration(out);
                break;
        }
    }

    [[noreturn]] void timeoutThread(const int& timeout)
    {
        std::this_thread::sleep_for(std::chrono::seconds(timeout));
        std::cerr << "Storage create timeout, aborting after " << timeout << " seconds"<< std::endl;
        PrintStaticConfiguration(std::cerr);
        std::exit(EXIT_FAILURE);
    }

    void setTimeout(const int& timeout)
    {
        if (timeout)
        {
            std::thread t(timeoutThread, timeout);
            t.detach();
        }
    }

    int TestConnectivityCommand(std::ostream& out,
                                const boost::program_options::variables_map& map)
    {
        const auto ns(map["ns"].as<std::string>());
        const auto timeout(map["timeout"].as<int>());
        setTimeout(timeout);
        auto sdl(createStorage(ns, out));
        if (sdl != nullptr)
        {
            auto asyncStorageImpl(std::dynamic_pointer_cast<AsyncStorageImpl>(sdl));
            if (asyncStorageImpl != nullptr)
            {
            	AsyncStorage& operationalHandler(asyncStorageImpl->getOperationHandler(ns));
            	AsyncRedisStorage* redisStorage = dynamic_cast<AsyncRedisStorage*>(&operationalHandler);
                if (redisStorage != nullptr)
                {
                	auto databaseinfo (redisStorage->getDatabaseInfo());
                	PrintDatabaseInfo(databaseinfo, out);
                }
                else
                {
                	// @TODO Improve output for the case if dummy backend is used.
                    out << "Cannot get AsyncRedisStorage." << std::endl;
                    return EXIT_FAILURE;
                }
            }
            else
            {
                out << "Cannot get AsyncStorageImpl." << std::endl;
                return EXIT_FAILURE;
            }
        }
        return EXIT_SUCCESS;
    }
}

AUTO_REGISTER_COMMAND(std::bind(TestConnectivityCommand, std::placeholders::_1, std::placeholders::_3),
                      "test-connectivity",
                      "Test SDL backend connectivity",
                      "Check that SDL database backend is available and show discovered redis host address and port",
                      CommandMap::Category::UTIL, 30020,
                      ("ns", boost::program_options::value<std::string>()->default_value("sdltoolns"), "Used namespace")
                      ("timeout", boost::program_options::value<int>()->default_value(0), "Timeout (in seconds), Default is no timeout"));
