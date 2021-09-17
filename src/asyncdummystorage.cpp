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

#include "private/asyncdummystorage.hpp"
#include <sys/eventfd.h>
#include "private/engine.hpp"

using namespace shareddatalayer;

AsyncDummyStorage::AsyncDummyStorage(std::shared_ptr<Engine> engine):
    engine(engine)
{
}

int AsyncDummyStorage::fd() const
{
    return engine->fd();
}

void AsyncDummyStorage::handleEvents()
{
    engine->handleEvents();
}

void AsyncDummyStorage::postCallback(const Callback& callback)
{
    engine->postCallback(callback);
}

void AsyncDummyStorage::waitReadyAsync(const Namespace&, const ReadyAck& readyAck)
{
    postCallback(std::bind(readyAck, std::error_code()));
}

void AsyncDummyStorage::setAsync(const Namespace&, const DataMap&, const ModifyAck& modifyAck)
{
    postCallback(std::bind(modifyAck, std::error_code()));
}

void AsyncDummyStorage::setIfAsync(const Namespace&, const Key&, const Data&, const Data&, const ModifyIfAck& modifyIfAck)
{
    postCallback(std::bind(modifyIfAck, std::error_code(), true));
}

void AsyncDummyStorage::setIfNotExistsAsync(const Namespace&, const Key&, const Data&, const ModifyIfAck& modifyIfAck)
{
    postCallback(std::bind(modifyIfAck, std::error_code(), true));
}

void AsyncDummyStorage::getAsync(const Namespace&, const Keys&, const GetAck& getAck)
{
    postCallback(std::bind(getAck, std::error_code(), DataMap()));
}

void AsyncDummyStorage::removeAsync(const Namespace&, const Keys&, const ModifyAck& modifyAck)
{
    postCallback(std::bind(modifyAck, std::error_code()));
}

void AsyncDummyStorage::removeIfAsync(const Namespace&, const Key&, const Data&, const ModifyIfAck& modifyIfAck)
{
    postCallback(std::bind(modifyIfAck, std::error_code(), true));
}

void AsyncDummyStorage::findKeysAsync(const Namespace&, const std::string&, const FindKeysAck& findKeysAck)
{
    postCallback(std::bind(findKeysAck, std::error_code(), Keys()));
}

void AsyncDummyStorage::listKeys(const Namespace&, const std::string&, const FindKeysAck& findKeysAck)
{
    postCallback(std::bind(findKeysAck, std::error_code(), Keys()));
}

void AsyncDummyStorage::removeAllAsync(const Namespace&, const ModifyAck& modifyAck)
{
    postCallback(std::bind(modifyAck, std::error_code()));
}
