*********
 Testing
*********

Karabo extensively uses two technologies, C++ and Python. Code for both technologies is tested using unit tests. Because of the distributed nature of Karabo also more complex functional tests need to be executed.

Unit tests for the C++ core
===========================

We are using `CppUnit <http://sourceforge.net/projects/cppunit/>`_ as
unit testing framework, which is nicely supported by NetBeans.

All unit tests are placed in *src/karabo/tests*. Within the *tests*
folder the file structure of Karabo's sources is repeated and the
corresponding test classes are finally placed there.

Every sub-folder in *tests* implements an own ``main()`` function which runs all registered classes of the folder. By convention all test classes should end with <className>_Test.cpp or
<className>_Test.hh, respectively. 

Creating a whole new test 
--------------------------

HINT: You have do this only in the unlikely case that Karabo gets an new sources sub-folder!

1. Create a new folder under *src/karabo/tests*, corresponding to the new folder created in *src/karabo*

2. Right-click the folder *TestFiles* in Netbeans and click *New CppUnit Test...*

3. In the dialog use::

     Test Name:        <folderName>_test
     Folder:           tests/<folderName>
     Test Class Name:  <className>_Test
     Source Extension: cc and hh
     Test Name:        <folderName>TestRunner


Creating a new test class
-------------------------

1. Navigate to the corresponding test in Netbeans (e.g. *util_test*) right-click and select *New->Other...*

2. In the dialog choose *C++* as category and select *C++ CppUnit Test* as file type

3. In the next dialog use::

     Class Name:  <className>_Test
     For both source and header:
     Folder:      tests/<subfolder>
     Extension:   cc and hh


Creating a new test function
----------------------------

Simply add a new function into an existing test class and register it in the header using the *CPPUNIT_TEST* macro. (Look at other functions as example!)


Running tests
-------------

Method 1: Navigate to the *Test Files* or to any sub-folder of it, and select *Test* in the context menu.

Method 2: From command line (in *build/netbeans/karabo*): ``make test``

Method 3: Just compile test (in *build/netbeans/karabo*): ``make build-tests``



Unit tests for Python bindings (API 1)
======================================

*To be done by S. Esenov*


Unit tests for native Python code (API 2)
=========================================

*To be done by S. Esenov*
