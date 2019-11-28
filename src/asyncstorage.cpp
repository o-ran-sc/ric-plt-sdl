/*
   Copyright (c) 2018-2019 Nokia.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

/*
 * This source code is part of the near-RT RIC (RAN Intelligent Controller)
 * platform project (RICP).
*/

#include <boost/optional.hpp>
#include "config.h"
#include "private/abort.hpp"
#include "private/asyncconnection.hpp"
#include "private/asyncstorageimpl.hpp"
#include "private/createlogger.hpp"
#include "private/engineimpl.hpp"
#include "private/configurationreader.hpp"
#include "private/databaseconfigurationimpl.hpp"
#include "private/logger.hpp"
#if HAVE_REDIS
#include "private/redis/asyncredisstorage.hpp"
#include "private/redis/asyncdatabasediscovery.hpp"
#endif

using namespace shareddatalayer;

namespace
{

std::unique_ptr<shareddatalayer::AsyncStorage> createInstance(const boost::optional<shareddatalayer::AsyncConnection::PublisherId>& pId)
{
    auto engine(std::make_shared<EngineImpl>());
    auto logger(createLogger(SDL_LOG_PREFIX));
    /* clang compilation does not support make_unique */
    return std::unique_ptr<AsyncStorageImpl>(new AsyncStorageImpl(engine, pId, logger));
}

}

/* clang compilation produces undefined reference linker error without this */
constexpr char AsyncStorage::SEPARATOR;

std::unique_ptr<AsyncStorage> AsyncStorage::create()
{
    return createInstance(boost::none);
}
