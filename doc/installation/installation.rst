************
Installation
************

Before you start installing, make sure you know how you want to use
Karabo.

Just want to run Karabo and use existing devices
================================================

In this case you can simply download Karabo and the devices you are interested in from our servers and directly start.

Let's go to the procedure step by step:


1. Get and install Karabo Framework
-----------------------------------

A self extracting shell script is available on `ftp://karabo:framework@ftp.desy.de/karaboFramework/tags/1.3.0/`_

Select the correct installer for your operating system and download it to some local folder.

Change permissions and extract it using the following commands:

    chmod +x karabo-*.sh
    ./karabo-*.sh

When asked for path, simply press Enter to use the default path or just
enter a new path where you would like to install the Karabo Framework.

Your installation of the Karabo Framework is now complete.


2. Get and install all devices you need
---------------------------------------

Each device 



Write new or update existing devices
------------------------------------

If you are interested in implementing for example control-, or
compute-devices (plugins) for the Karabo system, you can download a
recent binary release of the core framework and directly start after its
installation.

Please read here for [[Getting Started - Karabo Packages|more
information]].

 

Karabo core framework development (compile from sources)
--------------------------------------------------------

If you want to contribute to the core framework developmnent, you will
have to build Karabo from sources.

**Participation in the core-framework development needs expert
knowledge.**

That is why you should have talked to Burkhard Heisen before you start.
Afterwards you can [[Getting Started - Karabo Framework| find more
details here]].

 

 

Developing code - tips, tricks and policies
===========================================

NetBeans
--------

Although technically not required, we strongly recommend the NetBeans
IDE for developing code associated to Karabo.

**NOTE: If you are working in the context of the European XFEL GmbH,
usage of NetBeans is mandatory!**

[[Getting Started - NetBeans|Read here for more information]].**\
**

Message Broker
--------------

Karabo is a distributed system and needs a so-called message-broker for
communication aspects.

[[Getting Started - Message Broker|Read here for more information]].

 

Good to know - a link collection**\
**
===================================

**SVN Repository (Versioning)**

[https://svnsrv.desy.de/desy/EuXFEL/WP76](https://svnsrv.desy.de/desy/EuXFEL/WP76 "https://svnsrv.desy.de/desy/EuXFEL/WP76") 
with kerberos authentication

[https://svnsrv.desy.de/basic/EuXFEL/WP76](https://svnsrv.desy.de/basic/EuXFEL/WP76) 
with basic authentication

** **

**WebSVN Link (Versioning)**

[https://svnsrv.desy.de/k5websvn/wsvn/EuXFEL.WP76](https://svnsrv.desy.de/k5websvn/wsvn/EuXFEL.WP76 "https://svnsrv.desy.de/k5websvn/wsvn/EuXFEL.WP76")

**[SVN Repository
How-to](/share/proxy/alfresco/api/node/content/workspace/SpacesStore/b24e732a-1eac-49a3-8830-406f07f7d71a/Subversion.pdf "SVN How-to")\
**

** **

**Issue-Tracking (Redmine)\
**

[https://in.xfel.eu/redmine](https://in.xfel.eu/redmine "https://in.xfel.eu/redmine")
(accessible also from outside European XFEL GmbH)

** **

**Continous Integration (Jenkins)**

[http://131.169.247.46:8080](http://131.169.247.46:8080 "http://131.169.247.46:8080")
(currently only accessible from within European XFEL GmbH)

** **

**Download of binary packages**

[https://docs.xfel.eu/share/page/site/KaraboFramework/wiki-page?title=Downloads](/share/page/site/KaraboFramework/wiki-page?title=Downloads "https://docs.xfel.eu/share/page/site/KaraboFramework/wiki-page?title=Downloads")**\
**

 

 

 

Wiki Page Preview
-----------------

![Alfresco
Enterprise](/share/components/images/alfresco-share-logo-enterprise.png)
Alfresco Software, Inc. © 2005-2012 All rights reserved.
