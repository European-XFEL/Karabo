[![Alfresco Share](/share/res/themes/xfel/images/app-logo.png)](#)   [My
Dashboard]( "My Dashboard")

Sites

[People]( "People") [Repository]( "Repository")

More...

###### My... {.first-of-type}

-   [My Tasks]( "My Tasks")
-   [Workflows I've Started]( "Workflows I've Started")
-   [My Content]( "My Content")
-   [My Sites]( "My Sites")
-   [My Profile]( "My Profile")

Burkhard Heisen

-   [![avatar](/share/proxy/alfresco/slingshot/profile/avatar/heisenb)]()
    What are you doing?
    Share
-   [My Profile]( "My Profile")
-   [Help]( "Help")
-   [Logout]( "Logout")

  [Help]( "Help")  
[Tutorial]( "Alfresco Tutorial for XFEL Users (Power Point Presentation)")
  [User Guide]( "Alfresco User Guide for XFEL Users (PDF)")  
[FAQ]( "Alfresco Frequently Asked Questions")   [Imprint]( "Imprint")  
[Support]( "Problems? Email support")  
[Home]( "European XFEL Home Directory")

-   [Advanced
    Search...](/share/page/site/KaraboFramework/advsearch "Advanced Search...")

Karabo Framework {.theme-color-3}
================

[Invite](/share/page/site/KaraboFramework/invite) Edit Site Details
Customize Site Leave Site

[Site Dashboard](/share/page/site/KaraboFramework/dashboard)  
[Wiki](/share/page/site/KaraboFramework/wiki-page?title=Main_Page)  
[Blog](/share/page/site/KaraboFramework/blog-postlist)   [Document
Library](/share/page/site/KaraboFramework/documentlibrary)  
[Calendar](/share/page/site/KaraboFramework/calendar)  
[Discussions](/share/page/site/KaraboFramework/discussions-topiclist)  
[Members](/share/page/site/KaraboFramework/site-members)

[Wiki Page List](/share/page/site/KaraboFramework/wiki) [Main
Page](/share/page/site/KaraboFramework/wiki-page?filter=main&title=Main_Page)

New Page

 

Delete

 

Rename

[RSS
Feed](/share/proxy/alfresco-feed/slingshot/wiki/pages/KaraboFramework?format=rss)

Rename

Enter the new name for the page

Print Wiki Page

Getting Started - Karabo Packages

View Page | [Edit
Page](?title=Getting_Started_-_Karabo_Packages&action=edit) |
[Details](?title=Getting_Started_-_Karabo_Packages&action=details)

Back to [[Getting Started]]

1. Install dependencies
=======================

Before you install the Karabo Framework, you need to first install some
dependencies.

-   Ubuntu type system:

        sudo apt-get install subversion build-essential
        sudo apt-get install krb5-user 

    For krb5-user use DESY.DE as default REALM

-   SL6:

        yum install make gcc gcc-c++ subversion
        yum install krb5-workstation

    For krb5-workstation copy /etc/krb5.conf from DESY server

-   MacOS system - see [[Getting Started - Karabo
    Framework|instructions]] for Karabo Framework.

2. Get and install Karabo Framework
===================================

A self extracting shell script is available on [[Downloads]].

Download it to a local folder.

Change permissions and extract it using the following commands:

    chmod +x karabo-*.sh
    ./karabo-*.sh

When asked for path, simply press Enter to use the default path or just
enter a new path where you would like to install the Karabo Framework.

Your installation of the Karabo Framework is now complete.

3. Get and install Karabo package development
=============================================

In addition to the Karabo Framework, if you need to install the package
development, you can do it like this:

-   Create the folder where you want to check out the
    karaboPackageDevelopment and go there, e.g.

        mkdir karaboPackageDevelopment 
        cd karaboPackageDevelopment

-   Now you need to checkout the code from the svn. But before the check
    out, you need to get a valid kerberos ticket via:

        kinit username@DESY.DE

    (Note: this is NOT your DESY email address.)

    Provide your password when prompted.

-   Now check out:

        svn co https://svnsrv.desy.de/desy/EuXFEL/WP76/karabo/karaboPackageDevelopment/trunk .

(If you have never used our SVN repository before,
[here](/share/proxy/alfresco/api/node/content/workspace/SpacesStore/b24e732a-1eac-49a3-8830-406f07f7d71a/Subversion.pdf "SVN How-to")
is the documentation on how to get an account.)

The installation of your Karabo package development is now complete.

4. Testing the installation
===========================

To test if you installed everything correctly, please do the following:

Within the karaboPackageDevelopment you will find the "karabo" script.
It is a tool like a package manager which helps to set up existing
karabo packages or create new ones.

For testing your Karabo installation simply type:

    ./karabo install karabo-self-test

Enter your email address when prompted.

This will download and build some sample-devices for self-testing and
automatically deploy them to the plugin folder of the deviceServer in
the central run directory.

Now you can navigate to run folder and first start all device servers
and gui:

    cd run./allStart
    ./startCli

This will open a few xterms - one for each device server (data logger,
gui server, cpp device server, python device server) and one for gui. Go
to gui and click connect button, leave the defaults entries in popped-up
window and click connect. Once connected, you should see on the
navigation panel all devices loaded. On the left bottom side, there is
project panel. Click on 'Open project button' navigate to your
...../run/project folder and open Karabo\_Self\_Test.krb project. Right
click on devices and select 'Initiate all'. You will see a few scenes
loaded in the middle panel. Now you can go to different scenes,
start/stop devices and so on.

In future also python macro will be available to do some automatic
checks.

Read
[here](/share/page/site/KaraboFramework/wiki-page?title=Device-Developer_Documentation)
for more details.

5. Testing an individual package
================================

-   Let's say you wanted to test the conveyor device, simply type

        ./karabo checkout conveyor testDevices trunk

-   Go to the directory of the package, e.g.

        cd packages/testDevices/conveyor

-   Compile the package:

        make

-   You will need one running instance of the gui device server. If it
    is not running, within your package go to the guiServer directory
    and start the script startGuiServer\

        cd karaboPackageDevelopment/run/guiServer./startGuiServer

-   Next, the plugin needs to be deployed, e.g.:

        cd karaboPackageDevelopment/packages/testDevices/conveyor/run/deviceServer/plugins
        ln -s ../../../dist/Debug/GNU-Linux-x86/libconveyor.socd .. 

-   Finally the deviceServer can be started:

        ./startDeviceServer

-   You may use the GUI in order to instantiate and interact with your
    device. In the run folder simply execute:

        ./startGui

 

Wiki Page Preview
-----------------

![Alfresco
Enterprise](/share/components/images/alfresco-share-logo-enterprise.png)
Alfresco Software, Inc. © 2005-2012 All rights reserved.
