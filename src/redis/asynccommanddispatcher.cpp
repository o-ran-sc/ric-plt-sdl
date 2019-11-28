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

#include "private/redis/asynccommanddispatcher.hpp"
#include <cstdlib>
#include "config.h"
#include "private/redis/databaseinfo.hpp"
#if HAVE_HIREDIS_VIP
#include "private/redis/asynchiredisclustercommanddispatcher.hpp"
#include "private/redis/asynchirediscommanddispatcher.hpp"
#elif HAVE_HIREDIS
#include "private/redis/asynchirediscommanddispatcher.hpp"
#endif
#include "private/abort.hpp"
#include "private/engine.hpp"

using namespace shareddatalayer::redis;

std::shared_ptr<AsyncCommandDispatcher> AsyncCommandDispatcher::create(Engine& engine,
                                                                       const DatabaseInfo& databaseInfo,
                                                                       std::shared_ptr<ContentsBuilder> contentsBuilder,
                                                                       bool usePermanentCommandCallbacks,
                                                                       std::shared_ptr<Logger> logger,
                                                                       bool usedForSentinel)
{
#if HAVE_HIREDIS_VIP
    static_cast<void>(usedForSentinel);
    if (databaseInfo.type == DatabaseInfo::Type::CLUSTER)
    {
        return std::make_shared<AsyncHiredisClusterCommandDispatcher>(engine,
                                                                      databaseInfo.ns,
                                                                      databaseInfo.hosts,
                                                                      contentsBuilder,
                                                                      usePermanentCommandCallbacks,
                                                                      logger);
    }
    else
        return std::make_shared<AsyncHiredisCommandDispatcher>(engine,
                                                               databaseInfo.hosts.at(0).getHost(),
                                                               databaseInfo.hosts.at(0).getPort(),
                                                               contentsBuilder,
                                                               usePermanentCommandCallbacks,
                                                               logger);
#elif HAVE_HIREDIS
    if (databaseInfo.type == DatabaseInfo::Type::CLUSTER)
        SHAREDDATALAYER_ABORT("Not implemented.");
    return std::make_shared<AsyncHiredisCommandDispatcher>(engine,
                                                           databaseInfo.hosts.at(0).getHost(),
                                                           databaseInfo.hosts.at(0).getPort(),
                                                           contentsBuilder,
                                                           usePermanentCommandCallbacks,
                                                           logger,
                                                           usedForSentinel);
#else
    SHAREDDATALAYER_ABORT("Not implemented.");
#endif
}
