..
..  Copyright (c) 2019-2022 Nokia.
..
..  Licensed under the Creative Commons Attribution 4.0 International
..  Public License (the "License"); you may not use this file except
..  in compliance with the License. You may obtain a copy of the License at
..
..    https://creativecommons.org/licenses/by/4.0/
..
..  Unless required by applicable law or agreed to in writing, documentation
..  distributed under the License is distributed on an "AS IS" BASIS,
..  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
..
..  See the License for the specific language governing permissions and
..  limitations under the License.
..

Release Notes
=============

This document provides the release notes of the sdl library.

.. contents::
   :depth: 3
   :local:



Version history
---------------

[1.6.0] - 2022-03-10

* Enable redis/sentinel port and sentinel master name configuration

[1.5.1] - 2021-09-17

* Add set, get and listKeys -sdltool CLI commands

[1.5.0] - 2021-09-17

* New listKeys API to support glob-style search pattern
* Deprecated old findKeys and findKeysAsync API

[1.4.0] - 2021-08-11

* Add synchronous readiness check API

[1.3.0] - 2021-08-05

* Definable timeout for DB backend readiness in synchronous SDL API
* Fix SDL configuration file path Valgrind errors

[1.2.1] - 2021-05-31

* Multiple DBAAS Redis standalone groups

[1.2.0] - 2021-05-26

* Multiple DBAAS Redis Sentinel groups
* New namespace (--ns) option in sdltool test-get-set -command

[1.1.3] - 2020-05-16

* Rename rpm and Debian makefile targets to rpm-pkg and deb-pkg.
* Update CI Dockerfile to utilize rpm-pkg and deb-pkg makefile targets.

[1.1.2] - 2020-05-15

* Add makefile targets to build rpm and Debian packages.

[1.1.1] - 2020-05-11

* Add unit test code coverage (gcov) make target.

[1.1.0] - 2020-01-09

* Add public helper classes for UT mocking.

[1.0.4] - 2019-11-13

* Add PackageCloud.io publishing to CI scripts.

[1.0.3] - 2019-11-08

* Add CI Dockerfile to compile SDL library and run unit tests.
* Remove AX_PTHREAD autoconf macro due to incompatible license.

[1.0.2] - 2019-10-02

* Take standard stream logger into use.

[1.0.1] - 2019-10-01

* Add support for Redis Sentinel based database discovery, which usage can be
  activated via environment variables.
* Add Sentinel change notification handling logic.
* Unit test fix for a false positive warning, when'EXPECT_EQ' macro is used
  to validate boolean value.

[1.0.0] - 2019-08-20

* Initial version.
* Take Google's C++ unit test framework into use.
* Implement basic storage operations to create, read, update, and delete
  entries.
