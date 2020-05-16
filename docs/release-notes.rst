..
..  Copyright (c) 2019 Nokia.
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

This document provides the release notes for O-RAN SC Amber release of
ric-plt/sdl.

.. contents::
   :depth: 3
   :local:



Version history
---------------

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
