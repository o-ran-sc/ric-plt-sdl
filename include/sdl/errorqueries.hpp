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

/** @file */

#ifndef SHAREDDATALAYER_ERRORQUERIES_HPP_
#define SHAREDDATALAYER_ERRORQUERIES_HPP_

#include <system_error>

namespace shareddatalayer
{
   /** @enum shareddatalayer::Error
     * @brief Error constants meant for client error handling implementation.
     *
     * Certain <code>std::error_code</code>s returned by shareddatalayer can
     * be compared against these enum values. If returned error can be compared
     * against these values, it is mentioned in the documentation of the
     * functionality which returns the <code>std::error_code</code>.
     *
     * Descriptions of the <code>shareddatalayer::Error</code> values and
     * instructions how to handle each value:
     *
     * <code>shareddatalayer::Error::SUCCESS</code>:<br>
     * Request was successfully handled, no error handling needed.
     *
     * <code>shareddatalayer::Error::NOT_CONNECTED</code>:<br>
     * shareddatalayer is not connected to the backend data storage and therefore could not
     * deliver the request to the backend data storage. Data in the backend data storage
     * has not been altered.<br>
     * Client is advised to try the operation again later. If shareddatalayer class,
     * from which the error originated, contains <code>waitReadyAsync</code> API, client
     * can use <code>waitReadyAsync</code> API to get a notification once shareddatalayer
     * is connected to the backend data storage.<br>
     * Note that if client receives this error for the first SDL API operation which client does,
     * it is advised to modify client implementation so that client waits for <code>waitReadyAsync</code>
     * notification before doing any SDL API operations.
     *
     * <code>shareddatalayer::Error::OPERATION_INTERRUPTED</code>:<br>
     * shareddatalayer sent the request to the backend data storage but did not receive a reply from
     * the backend data storage. In case of a write type request, data in the backend data storage
     * may or may not have been altered.<br>
     * Client is advised to try the operation again later. If shareddatalayer class, from which the error originated,
     * contains waitReadyAsync API, client can use waitReadyAsync API to get a notification once shareddatalayer is
     * connected to the backend data storage.<br>
     * When shareddatalayer operations work again, client can choose how to best address the possible loss of consistency.
     * Client can re-do all the interrupted writes or read the current data in the backend data storage and update it accordingly.
     *
     * <code>shareddatalayer::Error::BACKEND_FAILURE</code>:<br>
     * shareddatalayer delivered the request to the backend data storage but the backend data storage failed to process the request.
     * In case of a write type request, data in the backend data storage may or may not have been altered.<br>
     * Client is advised to try the operation again later. If also re-tries are failing client should re-create the used shareddatalayer instance.
     * It is possible that the system does not automatically recover from this type of error situations. Therefore client is advised
     * to escalate the problem to O&M if operation does not succeed after above mentioned recovery actions.<br>
     * When shareddatalayer operations work again, client can choose how to best address the possible loss of consistency
     * (see <code>shareddatalayer::Error::OPERATION_INTERRUPTED</code> for possible options).
     *
     * <code>shareddatalayer::Error::REJECTED_BY_BACKEND</code>:<br>
     * shareddatalayer delivered the request to the backend data storage but the backend data storage rejected the request.
     * In case of a write type request, data in the backend data storage may or may not have been altered.<br>
     * It is likely that same request will fail repeatedly. It is advised to investigate the exact reason for the failure from the logs.
     * Reason can be, for example, too many keys in the request or too large data in the request. It is backend specific, what kind of
     * request is considered invalid. Redis is the most common backend type, maximum supported request parameters with Redis backend
     * are currently by default following:
     * maximum amount of keys in request: 524287 keys, maximum key/value size: 512MB.
     * If the client request is invalid, client implementation needs to be changed. If the reason for invalid request seems to originate
     * from shareddatalayer, contact shareddatalayer support. <br>
     * When shareddatalayer operations work again, client can choose how to best address the possible loss of consistency
     * (see <code>shareddatalayer::Error::OPERATION_INTERRUPTED</code> for possible options).
     *
     * <code>shareddatalayer::Error::REJECTED_BY_SDL</code>:<br>
     * shareddatalayer could not process the request and therefore did not deliver the request to the backend data storage.
     * Data in the backend data storage has not been altered.<br>
     * It is likely that same request will fail repeatedly. Reason can be, for example, illegal parameter passed to SDL API function.
     * It is advised to investigate the exact reason for the failure, for example, by investigating logs and returned <code>std::error_code</code>.
     * If the client request is invalid, client implementation needs to be changed.<br>
     */
    enum class Error
    {
        SUCCESS = 0,
        NOT_CONNECTED,
        OPERATION_INTERRUPTED,
        BACKEND_FAILURE,
        REJECTED_BY_BACKEND,
        REJECTED_BY_SDL
    };

    /// @private
    std::error_condition make_error_condition(shareddatalayer::Error ec);
}

namespace std
{
    /// @private
    template <>
    struct is_error_condition_enum<shareddatalayer::Error> : public true_type { };
}

#endif
