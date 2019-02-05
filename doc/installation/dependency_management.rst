External Dependency Management in Karabo
========================================

The Karabo Framework bundles a large number of third party packages upon which
it is dependent. This includes things like boost, Qt, and HDF5.

Because building these dependencies has become a time consuming and sensitive
process, they are now being managed by an automated system. Using the
CI (continuous integration) features of GitLab, the dependencies are
automatically built into a redistributable bundle whenever one or more of those
dependencies changes.


Installing the Dependencies into a Clean Source Tree
----------------------------------------------------

After cloning the framework from git, or doing a clean rebuild, the external
dependencies should first be installed. This is handled automatically by the
``auto_build_all.sh`` script.

The already-built dependencies will be downloaded from
http://exflserv05.desy.de/karabo/karaboDevelopmentDeps/. They will be unpacked
and certain file paths within will be rewritten to match the environment where
the framework is being built.

**NOTE**: The build requirements include the ``curl`` and ``file`` commands. If
you do not have these installed, the download will fail and you will have to
wait for the dependencies to be built locally.


Mechanism for Determining Correct Dependencies to Install
---------------------------------------------------------

You might now be wondering: "How does the script know *which* dependencies to
download?". That's a very good question. The short answer is that the correct
version is found by querying the git repository. This information, combined
with the ID of the user's OS is used to determine the correct URL for the
dependencies.

The detailed answer is that each "version" of the dependencies recieves a
specially formatted tag on the master branch of the framework git repository.
These tags can be thought of as markers in the linear history which denote safe,
usuable versions of the dependencies. It is important that tags are not added to
"broken" commits.

So, starting from the current HEAD commit in a local git repository, the history
is examined one commit at a time until the most recent dependency tag is located.
This tag is then combined with the OS identifier and a URL is generated. See
below for a description of the naming convention used for these dependency tags.

**NOTE:** Please observe that this mechanism can only work if the dependency
tags are only added to the master branch. **NEVER NEVER NEVER** add a dependency
tag to a different branch; but especially not to a long lived branch which might
later be merged into the master branch. **It is crucially important** that
history can be walked directly back to the nearest dependency tag. If there is
any ambiguity about what that tag is, then this mechanism will cease to work in
a reliable fashion.

All that said, this is a robust solution for people developing the framework.
It does not affect users who install from a pre-built version of the framework.


Adding/Removing/Updating a Dependency
-------------------------------------

To add, remove, or update a dependency in Karabo, create a new feature branch
and make your changes. Build and install the dependency (or dependencies)
that you are currently concerned with.

Building all of Karabo is explained in :ref:`installation/sources`. But if you
only want to know how to build a single extern. Here is the command:

.. code-block:: bash

    cd extern
    ./build.sh <platform-directory> externPackageName

``<platform-directory>`` is usually something like ``GNU-Linux-x86`` or
``Darwin-x86``. It's the directory where all built externs are staged before
being copied into a karabo build directory.

Note that dependencies which include executable binaries (programs or shared
libraries) will need to have their ``RUNPATH`` metadata set so that runtime
linking works appropriately. This is handled by the ``extern/relocate_deps.sh``
script, and should be made there when adding/removing/updating dependencies.

In order to test the built result, you can create a feature branch with name
format of ``deps-mr-<package>`` and push it to the framework repository. The
CI runner will pick up this branch and create the a new dependency package at
http://exflserv05.desy.de/karabo/karaboDevelopmentDeps

Once the dependency package is created, you can try verify it by clean building
the framework locally using this package:

.. code-block:: bash

    ./auto_build_all.sh Clean-All
    FORCED_DEPS_TAG=deps-mr-<package> ./auto_build_all.sh Debug --pyDevelop

This will build the framework using the updated dependency packages created
from your feature branch. Once finished, you can try to use the framework to
verify if the new/updated pakcage plays nicely with the framework.

Once you are reasonably sure that everything works, open a merge request for
the branch. After the merge request is approved and merged into the master
branch, add a tag to the merge commit:

.. code-block:: bash

    git log  # to find the commit hash for the merge commit
    git tag deps-<name> <commit hash>  # See below for naming convention
    git push origin --tags

This will cause an automated build to begin and rebuild the dependencies. If
this build fails, then shame on you. You should be more careful when working
with the dependencies. You'll need to fix the problem with a new merge
request and tag. It's not the end of the world, of course, but it might
cause grief for anyone who performs a clean rebuild before you've fixed the
master branch.


The Naming Convention for Dependency tags
-----------------------------------------

All dependency tags need to begin with "deps-". This is hardcoded into the
build infrastructure. The rest is only defined by convention.

The basic format is: ``deps-<action>-<package>``

``<action>`` is one of the following: add, update, remove

``<package>`` is the principle package which is being changed. The fact that
multiple packages are perhaps being modified is not terribly important. It is
also a good idea to add a little bit of version information after the package
name for disambiguation. As the number of "deps-" tags grows, some packages will
likely appear more than once (eg. deps-update-boostNNN or deps-update-numpy)


Current collection of dependencies
----------------------------------

Karabo is currently shipped with the a tree dependencies that enables
development directly from the distributed platform:

+--------------------+----------------+-------+------------------+-------------------+
|library             ||version        |license| needed by Karabo | needed by the GUI |
+====================+================+=======+==================+===================+
| alabaster          |0.7.7          | BSD   | no               | no                |
+--------------------+---------------+-------+------------------+-------------------+
| babel              ||0.7.7         |  BSD  | no               | no                |
+--------------------+---------------+-------+------------------+-------------------+
| backcall           |0.1.0          | PSFL  | yes              | yes               |
+--------------------+---------------+-------+------------------+-------------------+
| backports-abc      |0.4            | PSFL  | no               | no                |
+--------------------+---------------+-------+------------------+-------------------+
| backports.ssl-match|3.5.0.1        | PSFL  | no               | no                |
+--------------------+---------------+-------+------------------+-------------------+
| boost              |1.66           |Boost License| yes               | yes                |
+--------------------+---------------+-------+------------------+-------------------+
| bzip2              | 1.0.6         | BSD   | yes              | yes               |
+--------------------+---------------+-------+------------------+-------------------+
| certifi            |2018.4.16      | MPL2.0| no               | no                |
+--------------------+---------------+-------+------------------+-------------------+
| chardet            |3.0.4          | LGPL  | yes              | no                |
+--------------------+---------------+-------+------------------+-------------------+
| coverage           |4.5.1          |APL2.0 | no               | no                |
+--------------------+---------------+-------+------------------+-------------------+
| cppunit            |1.14.0          |LGPL | no               | no                |
+--------------------+---------------+-------+------------------+-------------------+
| cycler             |0.10.0         | BSD-3 | no               | yes               |
+--------------------+---------------+-------+------------------+-------------------+
| Cython             |0.27.3         |APL    | ?                | ?                 |
+--------------------+---------------+-------+------------------+-------------------+
| daemontools-encore | 1.10-karabo3  | MIT | no              | no               |
+--------------------+---------------+-------+------------------+-------------------+
| dateutil    | 2.2  | apache/BSD | no              | yes               |
+--------------------+---------------+-------+------------------+-------------------+
| decorator          |4.0.10         | BSDv2 | yes              | yes               |
+--------------------+---------------+-------+------------------+-------------------+
| dill               |0.2.5          | BSD-3 | yes              | yes               |
+--------------------+---------------+-------+------------------+-------------------+
| docker             |3.3.0          |APL2.0 | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| docker-pycreds     |0.3.0          |APL2.0 | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| docutils           |0.12           |~      | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| ecdsa              |0.11           |MIT    | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| eulexistdb         |0.21.1         |APL2.0 | yes              | yes               |
+--------------------+------------+---------------+-------+------------------+-------------------+
| eulxml             |1.1.3          |APL2.0 | yes              | yes               |
+--------------------+------------+---------------+-------+------------------+-------------------+
| eXistDB            | 2.2         |LGPL | yes/not derivative | no |
+--------------------+------------+---------------+-------+------------------+-------------------+
| fftw               |3.3.2       |GPLv2 or later    | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| flake8             |3.3.0       |MIT    | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| flaky              |3.4.0       |APL2.0 | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| freetype           |2.5.2       |FTL/GPLv2 | no               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| gmock              |1.7.0       |BSD | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| guidata            |1.7.6       |CeCLv2 | no               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| guiqwt             |2.3.1       |CeCLv2 | no               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| h5py               |2.7.1      | BSD | no               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| httplib2           |0.9.1      | MIT      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| idna               | 2.7        |PSFL     | yes               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| imagesize          |0.7.1       |MIT     | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| ipcluster-tools    |0.0.11      | BSD-3      | yes               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| ipykernel          |4.3.1       | BSD-3-Clause      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| ipyparallel        |5.1.1       | BSD-3-Clause      | yes               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| ipython            |5.0.0       | BSD-3-Clause      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| ipython-genutils   |0.1.0       | BSD-3-Clause      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| jedi               |0.13.2      | MIT   | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| jpeg               |9a          | Ack   | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| Jinja2             |2.7.2       | BSD      | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| jsonschema         |2.3.0       | MIT      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| jupyter-client     |4.3.0       | BSD      | yes               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| jupyter-core       |4.1.0       | BSD      | yes               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| lapack             |3.6.0     | BSD      | yes               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| libpng             |1.6.8     | libpng license (MIT like)      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| libxml             |2.9.4     | MIT      | yes               | ?                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| libxslt             |1.1.38     | MIT      | yes               | ?                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| libzmq            |4.2.5     | LGPLv3      | yes               | ?                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| log4cpp             |1.1.3     | LGPLv2.1      | yes               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| lxml               |3.6.4       | BSD      | yes               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| MarkupSafe         |0.18        | BSD      | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| matplotlib         |2.1.1       | PSFL      | no               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| mccabe             |0.6.1       | MIT      | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| msgpack            |0.5.6       | APL2      | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| msgpack-numpy      |0.4.3       | BSD      | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| nbformat           |4.1.0       | BSD      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| nose               |1.3.0       | LGPL     | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| notebook           |4.2.2       | BSD      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| nss           |?       | MPL      | yes               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| numpy              |1.11.1      | BSD      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| openmq              | 5.0.1     | EPL/GPLv2 | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| paramiko           |1.14.0      | LGPL      | no               | no                |
+--------------------+------------+-----------+------------------+-------------------+
| parse              |1.6.3       | BSD       | no               | no                |
+--------------------+------------+-----------+------------------+-------------------+
| patchelf           |0.8       | GPLv3       | no               | no                |
+--------------------+------------+-----------+------------------+-------------------+
| pexpect            |3.1      | ISC license (BSD like) | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| pickleshare          |0.7.3      | MIT      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| Pillow               |2.5.3      | PIL license (MIT-like)     | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| Pint                 |0.7.1      | BSD-3-Clause      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| pip                  | 7.10     |   MIT    | yes              | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| pkgconfig               |1.2.2      |   MIT    | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| ply               |3.11      | BSD      | yes               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| prompt-toolkit              |2.0.8      | BSD-3-Clause      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| pssh                |2.3.1      | BSD      | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| psutil             |4.3.1      | BSD      | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| py            |1.4.31      | MIT      | yes               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| pycodestyle               |2.3.1      | MIT       | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| pycrypto             |2.6.1      | Public Domain  | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| pyelftools               |0.24      | Public Domain      | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| pyflakes               |1.5.0      | MIT      | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| Pygments               |2.0.2      | BSD      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| pyparsing              |2.0.1      | MIT      | no               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| pyqtgraph              |0.10.0      | MIT       | no               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| pyqwt              |5.2.0      | GPLv2       | no               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| pytest              |2.9.2      | MIT      | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| pytest-runner             |2.11.1      | MIT      | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| pytz               |2013.9      | MIT      | no               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| pyusb            |1.0.0b1      | BSD       | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| PyYAML              |3.12      | MIT      | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| pyzmq              |17.0.0      | LGPL+BSD      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| qtconsole                  |4.2.1      | BSD      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| requests                  |2.19.1      | APLv2      | yes               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| rpathology                |0.0.1      |MIT       | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| scikit-learn              |0.14.1      | BSD      | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| scipy              |0.18.0      | BSD      | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| setuptools             |39.1.0      | MIT      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| setuptools-scm             |1.15.6      | MIT      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| simplegeneric              |0.8.1      |  ZPL 2.1 (GPL plus trademark)      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| six              |1.10.0      |  GPLv3     | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| snappy           | 1.1.2      | BSD             | ?      | ?  |                 
+--------------------+------------+---------------+-------+------------------+-------------------+
| snowballstemmer              |1.2.1      | BSD      | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| Sphinx               |1.4.5.dev20180920      | BSD      | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| sphinx-rtd-theme             |0.1.9      | MIT      | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| suds-jurko            |      |0.6      | LGPLv3      | no               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| tiff               |4.4.1      | libtiff license (BSD like)      | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| tornado               |4.4.1      | APLv2      | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| traitlets                |4.2.2      | BSD      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| traits                |4.6.0      | BSD      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| tzlocal               |1.1.1      | MIT      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| urllib3                |1.23      | MIT      | yes               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| wcwidth               |0.1.7      | MIT      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| websocket-client        |0.48.0      | LGPL v2.1      | no               | no                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| wheel                 |0.24.0      | MIT      | yes               | yes                |
+--------------------+------------+---------------+-------+------------------+-------------------+
| wxPython           | 2.9.5.0 | wxWindows Library License ( like LGPL but more permissive) | yes | yes |
+--------------------+------------+---------------+-------+------------------+-------------------+ 
