.. _toolsNetbeans:

**************************
Getting Started - NetBeans
**************************

.. _installingJava:

Prerequisites - Java
====================

Netbeans is a Java based product. So you should have a proper JDK
installed on your development system before you can start using
Netbeans.

**Note: Version that must be used: Java 7 (also Java 8 if possible)**

* Ubuntu 12.04 - use openjdk 7 or oracle java 7. (If you don't have a jdk installed already, it is recommended to use openjdk).

  * for openjdk:

    .. code-block:: bash

      sudo apt-get install openjdk-7-jdk

  * for oracle:

    .. code-block:: bash

      sudo add-apt-repository ppa:webupd8team/java
      sudo apt-get update
      sudo apt-get install oracle-java7-installer

* Ubuntu 10.04 - use oracle java as above

* SL6:

  .. code-block:: bash

    yum install java-1.7.0-openjdk-devel


Installing NetBeans
===================

**NOTE: Current version that must be used for karaboFramework: 8.0**

* Download installation script: http://netbeans.org/downloads/

* Make it executable:

  .. code-block:: bash

    chmod u+x netbeans-8.0-linux.sh

* Execute it

  .. code-block:: bash

    sh netbeans-8.0-linux.sh
 
(**NOTE:** use default settings, but "Do not install JUnit.")

(**NOTE:** Unselect the ckeckbox "Allow Automatic updates" in the last
screen of the installation process, otherwise your Netbeans will jump
to development version)


Tuning NetBeans
===============


Subversion
----------

The netbeans 8.0 svnkit client supports subversion 1.6 and 1.8. The
default command line client in Ubuntu 12.04 is 1.6. If you have your
local working directory already checked out with svn CLI then it knows
which client version was used for checkout (.svn directories contain
this info). When you open this project in Netbeans, svnkit will
recognize that previous version and will ask you to upgrade your local
working directory to 1.8. If you do so, then you will not be able to
use svn CLI in this directory - only through Netbeans. So the
preferred way would be to say no - still Netbeans svn kit will work.

On the other hand you can upgrade subversion client in Ubuntu 12.04,
for instance from WANdisco:

.. code-block:: bash

  sudo sh -c 'echo "# WANdisco Open Source Repo" >> /etc/apt/sources.list.d/WANdisco.list'
  sudo sh -c 'echo "deb http://opensource.wandisco.com/ubuntu precise svn18" >> /etc/apt/sources.list.d/WANdisco.list'
  wget -q http://opensource.wandisco.com/wandisco-debian.gpg -O- | sudo apt-key add -
  sudo apt-get update
  sudo apt-get install subversion

or wait until Ubuntu 14.04 is released.

Similar problem exist on Mac OSX with Xcode 5, where default svn CLI
is 1.7. You can either use explicitly svn CLI 1.6 from
/Applications/Xcode.app/Contents/Developer/usr/subversion-1.6/bin/svn,
or install subversion 1.8 from MacPorts (there you need to to a
workaround to enable krb5 authentication in serf library used by
subversion for http/https protocol):

.. code-block:: bash

  # fetch the package
  sudo port fetch serf1
  # Edit Portfile
  sudo port edit serf1
  #patch it with the following changes
  #...... ask JS
  #rebuild and install
  sudo port -s install serf1
  # you may check if library needs any krb5 libs
  otool -L /opt/local/lib/libserf-1.1.3.4.dylib
  # install subversion
  sudo port install subversion

You can also try to use precompiled subversion 1.8 from WANdisco
http://www.wandisco.com/subversion/download#osx (not tested)

MacOSX
------

Netbeans under MacOSX does not have proper support for setting up the runtime environment. By default you will encounter linker errors if you are running for example the Karabo unit tests. The solution is to edit this file:

.. code-block:: bash

  /Applications/NetBeans/NetBeans 7.3.app/Contents/Resources/NetBeans/etc/netbeans.conf

and add to the end of this file the following line:

.. code-block:: bash

  export DYLD_LIBRARY_PATH=$(cat ~/.karabo/karaboFramework)/extern/lib

You have to restart netbeans to get this work-around into shape.


Adding Python support to Netbeans
---------------------------------

**Important:** Do not install versions 8.0.1 if you need to use Python
projects.


Netbeans versions till version 8.0
++++++++++++++++++++++++++++++++++

If you want to run python projects on Netbeans, please follow these
steps:

* Start NetBeans
* Click on Tools -> Plugins
* When the Plugins window opens click the Settings Tab
* In the Settings tab click the Add button
* Type whatever Name you want for this, e.g. 'Developer Updates'
* Under URL put http://deadlock.netbeans.org/hudson/job/nbms-and-javadoc/lastStableBuild/artifact/nbbuild/nbms/updates.xml.gz and click OK
* The plugin list should refresh and may take a moment, when it does click on the 'Available Plugins' tab.
* In the search box there type 'python' to find the python plugin, check the box for that plugin and click Install.
* Don't update any other plugins, otherwise you will switch to Netbeans development version.
* (NOTE: In order not to be disturbed by updates every time Netbeans is started, you can go to Settings and uncheck all the plugins)
* If netbeans crashes after python plugin installation, try to restart it like this:

  .. code-block:: bash

    $NETBEANS_INSTALL_DIR/bin/netbeans $NETBEANS_INSTALL_DIR/platform/modules/ext/org.eclipse.osgi_3.8.0.v20120529-1548.jar
    #substitute appropriate directory and jar file name above


Netbeans versions after 8.0.2
+++++++++++++++++++++++++++++

If you want more details on why this had to be changed visit this
Netbeans `issue <https://netbeans.org/bugzilla/show_bug.cgi?id=248986>`_.

If you want to run python projects on Netbeans, please follow these
steps (proposed on the Netbeans `issue
<https://netbeans.org/bugzilla/show_bug.cgi?id=248986>`_ and described
on this `blog entry
<https://blogs.oracle.com/geertjan/entry/python_in_netbeans_ide_81>`_):

* **Get the Python Plugin for NetBeans IDE 8.0.2**, by downloading it from http://plugins.netbeans.org/plugin/56795, and then unzip the file.

  * **Import Python Plugin to NetBeans IDE.**
  * In NetBeans IDE, go to Tools | Plugins.
  * Click the "Downloaded" tab.
  * Click Add Plugins and browse to the folder where you unzipped the
    files in the previous step.
  * Select them all.
  * Press "ok" to import them all.
  * Click Install.
  * Click Next.
  * Put a checkmark in "I accept the terms in all of the license
    agreements."
  * Click Install.
  * Click Continue.
  * Click Finish.
  * Click Close.

* Enjoy developing Python in Netbeans!


.. _netbeansCodeAssistance:

Code-Assistance
---------------

Currently Netbeans has a shortcoming, that no variables can be used
for the code-assistance configuration. As a consequence absolute paths
to include folders must be set.

The code-assistance configuration can be found under::

  Tools -> Options -> C/C++ -> Code Assistance -> C++ Compiler

If you are working as a package developer you should add the following two lines:

.. code-block:: bash

  /yourPathToKaraboInstallation/karabo-XXXX/include
  /yourPathToKaraboInstallation/karabo-XXXX/extern/include

If you are working as a framework developer you should use:

.. code-block:: bash

  /yourPathToKaraboFramework/karaboFramework/package/Debug/OS/Version/Platform/karabo/include
  /yourPathToKaraboFramework/karaboFramework/package/Debug/OS/Version/Platform/karabo/extern/include



Code formatting options
-----------------------

You can import netbeans options from the file
`netbeans-8.0-defaults.zip`_.

.. _netbeans-8.0-defaults.zip: https://docs.xfel.eu/share/page/site/KaraboFramework/document-details?nodeRef=workspace://SpacesStore/d8a56017-6269-4006-993c-0704bd1f31da


Running Python code from Netbeans
---------------------------------

To be able to execute python code from Netbeans it's necessary to
execute a few more steps:

* Open a python project (i.e. pythonKarabo)
* Right click and select properties
* Select Python in the left menu
* Select the button "Manage..." near to the option "Python Platform"
* In the new window remove the "Default" platform
* Add a new Python platform using the link
  $PATH_TO_KARABO_FRAMEWORK/package/Debug/Ubuntu/12.04/x86_64/karabo/extern/bin/python
  (this can be slightly different depending on your system)
* Go to tab "Python Path" and remove all the links with
  ".../site-packages/..." and ".netbeans/8.0" (after doing that you
  should only have 7 paths in the list)
* Select "close" to save changes and go back to the Project Properties window
* Select "OK" to save all your changes

After doing these operations you successfully configured your Python
to all your Python projects.

Now you can execute Python projects and their tests in Netbeans.


Heap size
---------

To change the heap size for NetBeans IDE:

* Copy the netbeans.conf from the etc folder in the NetBeans installation directory into the etc directory in your NetBeans user directory ($HOME/.netbeans/7.3/etc). You might need to create the $HOME/.netbeans/7.3/etc directory first.
* In the netbeans.conf file in your user directory, add the -J-Xmx command line Java startup switch (bolded below) in the netbeans.conf file. In this example, the heap is set to 2 Gb.

  .. code-block:: bash

    # command line switches
    netbeans_default_options="-J-Xms32m -J-Xmx2g -J-XX:PermSize=32m -J-XX:MaxPermSize=96m -J-Xverify:none -J-Dapple.laf.useScreenMenuBar=true"

* Restart the IDE.

To insure that you do not run out of memory while the built-in parser
is running, you can also add the garbage collector switches
-J-XX:+UseConcMarkSweepGC (concurrent collector) and
-J-XX:+UseParNewGC (parallel collector) to the netbeans.conf file.


Local Broker Service
--------------------

If Netbeans was installed with GlassFish support, you have the
possiblity to start a local JMS-Broker via Netbeans. For that you
should go to the navigation panel and navigate to::

  Services -> Servers -> GlassFish Server

right-click and select Start from the context menu.


If you need to change default port from 7676 to 7777 (as used by
karabo 1.1.3 and greater) then, start GlassFish server, right-click
again and select View Domain Admin Console, select from Menu on the
left server (Admin server), then Properties tab in the main
window. Override current value of JMS_PROVIDER_PORT to 7777 and
save. Afterwards restart the server.


Continuous Integration
----------------------

Mainly important for Karabo framework developers: You may access the
continous integration system from Netbeans, this allows to quickly
build and test code changes for all supported platforms on our
(remote) building farm.

For setting up click the Services tab in the navigation panel, then
right-click on Hudson Builders and select the Add Hudson
Instance... option.

In the pop-up dialog enter the following information::

  Name: Karabo Continuous Integration
  URL: http://exfl-jenkins:8080/


You need to log in, i.e. right-click Karabo Continuous Integration and
chose the Log In... option. (Login details are available from JS)


Cuda Development
----------------


