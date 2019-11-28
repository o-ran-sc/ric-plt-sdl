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

#ifndef SHAREDDATALAYER_PUBLISHERID_HPP_
#define SHAREDDATALAYER_PUBLISHERID_HPP_

#include <string>

namespace shareddatalayer
{
    /**
     * Identification that can be set by a publisher to identify the source of
     * the modification. Process that is modifying data in a namespace and is
     * also subscribed to the same namespace can utilize the publisherId to
     * distinguish events that it published itself.
     */
    using PublisherId = std::string;

    /**
     * Special constant for when a notification is not associated with any particular
     * publisher, such as a notification after notification framework service discontinuity
     * event. When this is set, client is advised to refresh its data from shared data
     * layer storage as it *might* have changed.
     *
     * @todo add check-and-throw for this in AsyncConnection/Connection side.
     */
    extern const PublisherId NO_PUBLISHER;
}

#endif
