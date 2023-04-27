..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

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

**Note: Version that must be used: Java 7 or 8**

* Ubuntu 14.04 - use openjdk 8 or oracle java 8. (If you don't have a jdk
installed already, it is recommended to use openjdk).

  * for openjdk:

    .. code-block:: bash

      sudo apt-get install openjdk-7-jdk or for java 8 follow
      `https://wiki.ubuntuusers.de/Java/Installation/OpenJDK/`_

* Ubuntu 16.04 - should have everything needed


Installing NetBeans
===================

**NOTE: Current version that must be used for karaboFramework: 8.0.2**

* Download installation script: http://netbeans.org/downloads/

* Make it executable:

  .. code-block:: bash

    chmod u+x netbeans-8.0.2-linux.sh

* Execute it

  .. code-block:: bash

    sh netbeans-8.0.2-linux.sh
 
(**NOTE:** use default settings, but "Do not install JUnit.")

(**NOTE:** Unselect the checkbox "Allow Automatic updates" in the last
screen of the installation process, otherwise your Netbeans will jump
to development version)


Tuning NetBeans
===============

Code-Assistance
---------------

Currently Netbeans has a shortcoming, that no variables can be used
for the code-assistance configuration. As a consequence absolute paths
to include folders must be set.

The code-assistance configuration can be found under::

  Tools -> Options -> C/C++ -> Code Assistance -> C++ Compiler

Add the following two lines:

.. code-block:: bash

  content_of_KARABO_variable/include
  content_of_KARABO_variable/extern/include



Code formatting options
-----------------------

Please import the NetBeans options from the file
`netbeans-8.0-settings.zip` in the git repository under the directory `build/netbeans`. This can be done via::

  Tools -> Options -> Import

Afterwards, take care that the lines that you edited (and not all the others!)
are adjusted according to the formatting rules. For this go to::

  Tools -> Options -> Editor -> On Save

For `All Languages` choose `Modified Lines Only` - for both `Reformat` and
`Remove Trailing Whitespace From`. This ensure proper format for new lines
without spamming code review with formatting noise.


Parallel Compilation
-----------------------

Unfortunately, you have to tell NetBeans by hand to make use of more than one
CPU core for compilation. For that you have to edit the C/C++ project options at::

  Tools -> Options -> C/C++ -> Project Options

In the `Make Options`, add e.g. `-j8` to compile up to 8 source files in parallel.


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



.. _netbeansCodeAssistance:


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



