#ifndef SHAREDDATALAYER_CLI_COMMON_HPP
#define SHAREDDATALAYER_CLI_COMMON_HPP

#include <functional>
#include <iostream>
#include <sdl/syncstorage.hpp>

namespace shareddatalayer
{
    namespace cli
    {
        inline std::ostream& operator<<(std::ostream &out, const SyncStorage::Data &data)
        {
            for (const auto& d : data)
                out << d;
            return out;
        }

        inline std::ostream& operator<<(std::ostream& out, const SyncStorage::DataMap& dataMap)
        {
            for (const auto& d : dataMap)
                out << d.first << " " << d.second;
            return out;
        }
    }
}
#endif
