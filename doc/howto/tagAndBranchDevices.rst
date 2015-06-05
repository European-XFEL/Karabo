*************************************************
How to tag and branch packages for Karabo devices
*************************************************
.. sectionauthor:: Gero Flucke <gero.flucke@xfel.eu>

This description applies for the development of Karabo devices as packages
using XFEL's subversion repository
`https://svnsrv.desy.de/desy/EuXFEL/karaboPackage <https://svnsrv.desy.de/k5websvn/wsvn/EuXFEL.karaboPackages>`_.
This repository holds many packages (e.g. ``dataGenerator``) grouped
in package categories (e.g. ``testDevices``).

Within each package there are the usual folders for code development with
subversion:

trunk
  for the main development

tags
  to mark specific versions

branches
  for bug fixes and back ports


Conventions for Tag and Branch Names
=====================================
The tag and branch names of Karabo packages consist of two parts that are
separated from each other by a hyphen (``-``). The part before the hyphen
indicates the version of the package functionality whereas the second
part tells to which Karabo version this package version fits.

The pattern for the package version is :math:`{\tt major.minor.revision}`.
For the Karabo version, it is sufficient to specify
:math:`{\tt major.minor}`.
So the combined version looks like

.. math::

   \underbrace{{\tt major.minor.revision}}_{\mbox{package}}-\underbrace{{\tt major.minor}}_{\mbox{karabo}}.

Branch names omit the :math:`{\tt .revision}` part, so example names for tags
and branches are

tags
   1.0.0-1.4, 1.1.0-1.4, 1.1.0-1.5
branches
   1.1-1.4, 2.0-1.4

Tagging and Branching Strategy
==============================
Since usually only one or very few developers write on the same device code, 
a rather linear development is expected. Therefore, branches are not
necessarily needed and a simple tagging and branching model is suggested:

* The **main development** takes place in the **trunk**.
* **Tags** are usually directly created **from the trunk**:
  trunk ⇒ tags/1.0.0-1.3
* Tag from another tag if a new Karabo version does not require
  package changes:
  tags/1.0.0-1.3 ⇒ tags/1.0.0-1.4
* **Branch for bug fixes or back ports**

  * for older (Karabo) versions that are incompatible with trunk
  * or if trunk contains unstable development.

* Create the **branch from a tag**:
  tags/1.1.0-1.4 ⇒ branches/1.1-1.4
* Commit fixes or back ports to this branch.
* Create a **tag from** this **branch**:
  branches/1.1-1.4 ⇒ tags/1.1.1-1.4

How to Tag and Branch
======================
The ``./karabo`` script in the ``karaboRun`` directory provides the 
`tag` and `branch` subcommands. Run ``./karabo help tag`` and  
``./karabo help branch`` for details.
