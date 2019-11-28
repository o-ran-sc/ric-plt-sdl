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

#include "private/namespacevalidator.hpp"
#include <sdl/emptynamespace.hpp>
#include <sdl/invalidnamespace.hpp>
#include "private/asyncconnection.hpp"


namespace shareddatalayer
{
    const std::string& getDisallowedCharactersInNamespace()
    {
        static const std::string disallowedCharacters{AsyncConnection::SEPARATOR, '{', '}'};
        return disallowedCharacters;
    }

    bool isValidNamespaceSyntax(const Namespace& ns)
    {
        return (ns.find_first_of(getDisallowedCharactersInNamespace()) == std::string::npos);
    }

    void validateNamespace(const Namespace& ns)
    {
        if (ns.empty())
            throw EmptyNamespace();
        else if (!isValidNamespaceSyntax(ns))
            throw InvalidNamespace(ns);
    }

    bool isValidNamespace(const Namespace& ns)
    {
        return (!ns.empty() && isValidNamespaceSyntax(ns));
    }
}
