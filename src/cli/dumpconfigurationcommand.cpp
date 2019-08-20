#include <ostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "private/cli/commandmap.hpp"
#include "private/configurationpaths.hpp"
#include "private/configurationreader.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::cli;

namespace
{
    bool parseConfiguration(const std::string& file)
    {
        boost::property_tree::ptree propertyTree;

        try
        {
            boost::property_tree::read_json(file, propertyTree);
        }
        catch (const boost::property_tree::json_parser::json_parser_error& e)
        {
            std::ostringstream os;
            os << "error in SDL configuration " << file << " at line " << e.line() << ": ";
            os << e.message();
            std::cerr << os.str().c_str() << std::endl;
            return false;
        }
        return true;
    }


    int dumpConfigurationCommand(std::ostream& out)
    {
        std::string line;
        bool status(true);

        for (const auto& i : findConfigurationFiles( getDefaultConfDirectories() ))
        {
            std::ifstream file(i);

            out << "File: " << i << std::endl;

            unsigned int lineNum = 1;

            if(file.is_open())
            {
                bool parseStatus = parseConfiguration(i);

                if(status && !parseStatus)
                    status = false;

                while(getline(file, line))
                    out << lineNum++ << ": " << line << std::endl;
                file.close();
            }
        }

        const auto var(DB_HOST_ENV_VAR_NAME);
        const auto conf(getenv(var));
        if (conf == nullptr)
            out << var << " not set." << std::endl;
        else
            out << var  << ": " << conf << std::endl;

        if(!status)
        {
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
}

AUTO_REGISTER_COMMAND(std::bind(dumpConfigurationCommand, std::placeholders::_1),
                      "dump-configuration",
                      "Dump configuration",
                      "Find, parse and dump all shareddatalayer configuration files contents.",
                      CommandMap::Category::UTIL, 30000);

