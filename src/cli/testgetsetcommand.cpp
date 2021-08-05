#include <ostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <chrono>
#include <thread>
#include "private/cli/commandmap.hpp"
#include "private/configurationpaths.hpp"
#include <sdl/syncstorage.hpp>

using namespace shareddatalayer;
using namespace shareddatalayer::cli;

namespace
{
    std::shared_ptr<shareddatalayer::SyncStorage> createSyncStorage(const std::string& nsStr, std::ostream& out)
    {
        try
        {
            auto sdl(shareddatalayer::SyncStorage::create());
            sdl->waitReady(nsStr, std::chrono::minutes(1));
            sdl->setOperationTimeout(std::chrono::seconds(5));
            return sdl;
        }
        catch (const shareddatalayer::Exception& error)
        {
            out << "SyncStorage create failed: " << error.what() << std::endl;
        }

        out << "Test suspended!" << std::endl;
        return nullptr;
    }

    void execRemoveAll(shareddatalayer::SyncStorage& sdl, const std::string& nsStr, std::ostream& out)
    {
        try
        {
            sdl.removeAll(nsStr);
        }
        catch (const shareddatalayer::Exception& error)
        {
            out << "removeAll failed: " << error.what() << std::endl;
        }
    }

    void execSet(shareddatalayer::SyncStorage& sdl, const std::string& nsStr, const std::string& key, const std::vector<uint8_t>& val, std::ostream& out)
    {
        try
        {
            sdl.set(nsStr, { { key, val } });
        }
        catch (const shareddatalayer::Exception& error)
        {
            out << "Set " << key << " failed: " << error.what() << std::endl;
        }
    }

    void execGet(shareddatalayer::SyncStorage& sdl, const std::string& nsStr, const std::string& key, const std::vector<uint8_t>& val, std::ostream& out)
    {
        try
        {
            auto map(sdl.get(nsStr, { key }));
            auto i(map.find(key));
            if (i == map.end())
                out << "Get " << key << ": Not found!" << std::endl;
            else if (i->second != val)
                out << "Get " << key << ": Wrong value!" << std::endl;
        }
        catch (const shareddatalayer::Exception& error)
        {
            out << "Get " << key << " failed: " << error.what() << std::endl;
        }
    }

    void timeoutThread(const int& timeout)
    {
        std::this_thread::sleep_for(std::chrono::seconds(timeout));
        std::cerr << "SyncStorage create timeout, aborting after " << timeout << " seconds"<< std::endl;
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

    int TestGetSetCommand(std::ostream& out, const boost::program_options::variables_map& map)
    {
        auto keyCount(map["key-count"].as<int>());
        const auto timeout(map["timeout"].as<int>());
        auto ns(map["ns"].as<std::string>());
        setTimeout(timeout);
        auto sdl(createSyncStorage(ns, out));
        if (sdl == nullptr)
            return EXIT_FAILURE;

        out << "namespace\t"
            << "key\t"
            << "value\t"
            << "Write\tRead" << std::endl;

        for (uint8_t val(0); 0 < keyCount--; ++val)
        {
            auto key("key_" + std::to_string(val));

            auto wStart(std::chrono::high_resolution_clock::now());
            execSet(std::ref(*sdl), ns, key, {val}, out);
            auto wEnd(std::chrono::high_resolution_clock::now());
            auto writeLatency_us = std::chrono::duration_cast<std::chrono::microseconds>(wEnd - wStart);

            auto rStart(std::chrono::high_resolution_clock::now());
            execGet(std::ref(*sdl), ns, key, {val}, out);
            auto rEnd(std::chrono::high_resolution_clock::now());
            auto readLatency_us = std::chrono::duration_cast<std::chrono::microseconds>(rEnd - rStart);

            out << ns << '\t'
                << key << '\t'
                << std::dec << static_cast<int>(val) << "\t"
                << std::dec << writeLatency_us.count() << "\t"
                << std::dec << readLatency_us.count() << std::endl;
        }

        auto start(std::chrono::high_resolution_clock::now());
        execRemoveAll(std::ref(*sdl), ns, out);
        auto end(std::chrono::high_resolution_clock::now());
        auto used_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        out << "All test keys removed in " << used_us.count() << " microseconds" << std::endl;

        return EXIT_SUCCESS;
    }
}

AUTO_REGISTER_COMMAND(std::bind(TestGetSetCommand, std::placeholders::_1, std::placeholders::_3),
                      "test-get-set",
                      "Write and read to DB and check latency",
                      "Check that basic SDL api commands (set/get) works normally and measure latency.",
                      CommandMap::Category::UTIL, 30010,
                      ("key-count", boost::program_options::value<int>()->default_value(10), "Number of write/read keys")
                      ("timeout", boost::program_options::value<int>()->default_value(0), "Timeout (in seconds), Default is no timeout")
                      ("ns", boost::program_options::value<std::string>()->default_value("sdltoolns"), "namespace to use"));
