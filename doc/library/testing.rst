..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

*********
 Testing
*********

Karabo extensively uses two technologies, C++ and Python. Code for both
technologies is tested using unit tests. Because of the distributed nature of
Karabo also more complex functional tests need to be executed.
For that reason we distinguish *unit*, *integration* and *long* tests.
During development, the *unit* tests are run for each branch update pushed to
the GitLab repository. The *integration* tests are run when a branch is merged
to the master. Currently (October 2019), the *long* tests are only run on
demand, but the idea is that they run for master every night or at least
weekly.

Whereas the unit tests are only within an API, the integration and long tests
are intended to provide also cross API tests. All these cross API tests run the
tests in Python.

If an integration/long test or code that may have a high impact on this kind
of tests is touched, at least the concerned integration tests should be run
when pushing to the GitLab repository.
This is achieved by temporarily committing changes to the *auto_build_all.sh*
script. The *if [ "$RUNTESTS" = "y" ]; then* section at its end has to be
extended by also calling *runIntegrationTests* (i.e. C++),
*runPythonIntegrationTests*, *runCppLongTests*, and/or *runPythonLongTests*.
Best practice is to do this in a single commit that is removed via
*git rebase -i* before merging.


Unit tests for the C++ core
===========================

We are using `CppUnit <http://sourceforge.net/projects/cppunit/>`_ as
unit testing framework.

All unit tests are placed in *src/karabo/tests*. Within the *tests*
folder the file structure of Karabo's sources is repeated and the
corresponding test classes are finally placed there.

Every sub-folder in *tests* implements an own ``main()`` function which runs all
registered classes of the folder. By convention all test classes should end with
<className>_Test.cpp or <className>_Test.hh, respectively.


Creating a whole new test
--------------------------

UPDATE: Use of CppUnit/Netbeans for creating new C++ tests is discouraged.
Supporting C++ unit and integration tests using Google Test
(https://github.com/google/googletest) in an IDE agnostic way is
the path the Karabo Framework is moving towards. Documentation on that will
come through the year of 2022.

HINT: You have do this only in the unlikely case that Karabo gets an new sources sub-folder!

1. Create a new folder under *src/karabo/tests*, corresponding to the new folder created in *src/karabo*

2. Right-click the folder *TestFiles* in NetBeans and click *New CppUnit Test...*

3. In the dialog use::

     Test Name:        <folderName>_test
     Folder:           tests/<folderName>
     Test Class Name:  <className>_Test
     Source Extension: cc and hh
     Test Name:        <folderName>TestRunner


Creating a new test class
-------------------------

1. Navigate to the corresponding test in NetBeans (e.g. *util_test*) right-click and select *New->Other...*

2. In the dialog choose *C++* as category and select *C++ CppUnit Test* as file type

3. In the next dialog use::

     Class Name:  <className>_Test
     For both source and header:
     Folder:      tests/<subfolder>
     Extension:   cc and hh

4. In case the new test class

 * does not appear in NetBeans' project view
 * and/or the tests are not executed using the recipes does not run (see :ref:`running-tests-label`)
 * and/or compiling the file using F9 fails

 it may help to close the Karabo project, re-open it, right click on the test subfolder and add header and source files using *Add existing Item...*.


Creating a new test function
----------------------------

Simply add a new function into an existing test class and register it in the header using the *CPPUNIT_TEST* macro. (Look at other functions as example!)


.. _running-tests-label:

Running C++ unit tests
-----------------------

* Method 1: In NetBeans, navigate to the *Test Files* or to any sub-folder of it,
  and select *Test* in the context menu.
* Method 2: Use the *auto build* script: *./auto_build_all.sh Debug --runTests* - but be aware
  that this will also run the Python tests.
* Method 3: From command line (in *build/netbeans/karabo*): ``make -j test``
* Method 4: Just compile test (in *build/netbeans/karabo*): ``make -j build-tests``


Integration and long tests for the C++ core
============================================

They use the same test framework as the unit tests, but are not organised in
the NetBeans project *karabo*, but in their own projects *integrationTests*
and *cppLongTests*, respectively.

Otherwise, they are organised in a similar way as the unit tests.

Running C++ integration tests
-------------------------------

* Method 1: In NetBeans' *integrationTests* project, navigate to the
  *Test Files* or to any sub-folder of it, and select *Test* in the context
  menu.
* Method 2: Use the *auto build* script:
  *./auto_build_all.sh Debug --runIntegrationTests* - but be aware that this
  will also run the Python tests.
* Method 3: From command line (in *build/netbeans/integrationTests*):
  ``make -j test``
* Method 4: Just compile test (in *build/netbeans/integrationTests*):
  ``make -j build-tests``

Running C++ long tests
-----------------------------

Similar as for the integration tests, but using project *cppLongTests*.
The *auto_build_all.sh* script has the option *--runLongTests* for these tests.


Unit tests for Python bindings (Bound API)
===========================================

*To be documented.*


Unit tests for native Python code (Middlelayer API)
====================================================

*To be documented.*


Integration/long tests for Python (both APIs)
================================================

The Python integration tests are the most complex ones since they need to
spawn extra processes, e.g. for bound Python devices.

They are organised as sub-directories of
*src/pythonKarabo/karabo/integration_tests*. The actual tests in there are
the *test_\*.py* files, using the test framework of either Bound or
Middlelayer Python. An empty *__init__.py* is required.
To integrate the new test with the continuous integration, it needs to be added
to the *runPythonIntegrationTests()* or *runPythonLongTests()* functions
in the *run_python_tests.sh* file.

If a test shall launch processes for different APIs, it is recommended to use
the *BoundDeviceTestCase* implemented in *karabo/integration_tests/utils.py*
as the test base class as in the *pipeline_cross_test* sub-directory.
This base provides *def start_server(self, api, server_id, ...)* to start
server processes of the *cpp*, *bound* or *mdl* API - and takes care to
properly terminate these processes after each test.

If the test case includes a new Bound Python device (like in the case of the
*device_comm_test*), the code for that class can be put into a file in the
test sub-directory.
To make it available as a plugin for the bound Python server, one has to add
this class as an entry point of the ``karabo.bound_device_test`` in the
*src/pythonKarabo/setup.py* file.
If one uses the recommended ``BoundDeviceTestCase.startServer`` to start the
bound Python server, this entry point has to be given as ``namespace`` argument.
Similarly, the same applies to middlelayer test devices with the corresponding
``karabo.middlelayer_device_test`` entry point.
