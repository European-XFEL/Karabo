..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

.. _library/ciAtXFEL:

*******************************************
Continuous Integration at the European XFEL
*******************************************

The source management system of the Karabo Framework is currently hosted on the
internal Gitlab server of the European XFEL.

The CI/CD (Continuous Integration and Continuous Delivery) features of the
Gitlab server are used to implement automatic tests and deployment of binaries
to a web server.

Automated Framework Builds
==========================

The Karabo framework is currently distributed with a run-environment containing
all necessary dependencies to run in an isolated mode plus the dependencies
needed to provide a minimal development environment as well as some analysis
convenience features (e.g. matplotlib and scipy).

The automated building system will create and upload the following artefacts:

* Dependencies binaries

* Framework binaries

* The conda packages: currently uploading only the KaraboGUI package

Dependencies binaries
+++++++++++++++++++++

The dependency system of Karabo is described :ref:`here <installation/dependency_management>`.

For convenience, the dependencies are built for each supported Operating system
and uploaded to the XFEL internal server `here <http://exflctrl01.desy.de/karabo/karaboDevelopmentDeps/>`__.

Framework binaries
++++++++++++++++++

The Framework is built against all supported operating systems and bundled in
a binary file that can be found `here <http://exflctrl01.desy.de/karabo/karaboFramework/tags>`_ 
as discussed in :ref:`here <installation/binary>`

In addition to the url tag location, the CI will create a link to a standard
location that always points to the latest build.

The address http://exflctrl01.desy.de/karabo/karaboFramework/tags/latest_build
contains the latest release build matching the `N.N.N` format.

The address http://exflctrl01.desy.de/karabo/karaboFramework/tags/latest_prerelease_build
contains the latest prerelease build.
Alpha releases, and release candidates will be linked to this address.

In addition to these builds that are generated from a tag on the git repository,
a nightly build is generated every day and upoaded to this address
http://exflctrl01.desy.de/karabo/karaboFramework/nightly

Conda Packages
++++++++++++++

There are severe limitations to the current dependency system used by the Karabo
framework. The conda packaging system has been identified as the replacement
for the monolithic dependency system currently in use.

The first component that has been refactored to be deployed with the conda
packaging system is the Karabo GUI.

The CI system will upload the conda package to the XFEL channel of packages
as well as upload to a mirror all the necessary external packages.
The latter is needed since the deployment of the Karabo GUI in the European
XFEL is often performed on machines that have restricted network access and
cannot use the default conda package repositories.

Automated Framework Tests
=========================

The CI configuration manages the following test types on Merge Requests and on
merge events:

* Framework unit tests

* Framework integration tests

* Framework long tests


Unit tests
++++++++++

Unit test jobs are executed on merge requests. To reduce the load on the CI,
only the tests affected by code changes will be executed.

Framework integration tests
+++++++++++++++++++++++++++

This test job will run the C++ integration tests located in
`src/integrationTests` as well as the python integration tests located in
`src/pythonKarabo/karabo/integration_tests`.

This task is triggered by merge events and merge requests with changes in the
code paths that are not tested via simple unit tests.

Framework long tests
++++++++++++++++++++

This test job will run the C++ tests located in
`src/cppLongTests` as well as the python pipeline integration tests.
This task is executed weekly.

Automate Device Tests
=====================

Device packages can also benefit from the continuous integration features
of the Gitlab server. For this reason, the devices generated from template
by the command `karabo new` will be provided with an example `.gitlab-ci.yml`
file that provides basic testing functionality.

The provided files build against the current latest release as well as the
latest prerelease build. The development process aims to provide an overlap
between minor versions. It should always be possible to have a device version
that is compatible with the currently released version of the framework and the
upcoming version of the framework.

In case a device is stable, it is advisable to execute tests on a weekly or
fortnightly schedule.
