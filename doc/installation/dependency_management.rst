..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

.. _installation/dependency_management:

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
http://exflctrl01.desy.de/karabo/karaboDevelopmentDeps/. They will be unpacked
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

``<platform-directory>`` is usually something like ``GNU-Linux-x86``.
It's the directory where all built externs are staged before
being copied into a karabo build directory.

Note that dependencies which include executable binaries (programs or shared
libraries) will need to have their ``RUNPATH`` metadata set so that runtime
linking works appropriately. This is handled by the ``extern/relocate_deps.sh``
script, and should be made there when adding/removing/updating dependencies.

In order to test the built result, you can create a feature branch with name
format of ``deps-mr-<package>`` and push it to the framework repository. The
CI runner will pick up this branch and create the a new dependency package at
http://exflctrl01.desy.de/karabo/karaboDevelopmentDeps

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


==================== ================= =========================================================== ===================== =========================
**library**          **version**       **license**                                                 **Karabo depends**    **KaraboGUI depends**
==================== ================= =========================================================== ===================== =========================
aio-pika             6.8.0             Apache-2.0                                                  yes                   no
aioredis             1.3.1             MIT                                                         yes                   no
aiormq               3.3.1             Apache-2.0                                                  yes                   no
AMQP-CPP             4.3.12            Apache-2.0                                                  yes                   no
asyncio-mqtt         0.8.1             BSD-3                                                       yes                   yes
atomicwrites         1.4.0             MIT                                                         yes                   yes
attrs                20.3.0            MIT                                                         yes                   yes
backcall             0.2.0             BSD-3                                                       yes                   yes
backports-abc        0.4               PSFL                                                        no                    no
backports.ssl-match  3.5.0.1           PSFL                                                        no                    no
boost                1.68              Boost License                                               yes                   no
bzip2                1.0.6             BSD                                                         yes                   yes
certifi              2018.4.16         MPL2.0                                                      no                    no
chardet              3.0.4             LGPL                                                        yes                   no
colorama             0.4.4             BSD                                                         yes                   yes
conan                1.57.0            MIT                                                         yes                   no
coverage             4.5.1             Apache-2.0                                                  no                    no
cppunit              1.14.0            LGPL                                                        no                    no
cycler               0.10.0            BSD-3                                                       no                    yes
cython               0.29.24           Apache-2.0                                                  no                    no
daemontools-encore   1.10-karabo3      MIT                                                         no                    no
dateutil             2.8.1             apache/BSD                                                  no                    yes
decorator            4.4.2             BSDv2                                                       yes                   yes
dill                 0.2.5             BSD-3                                                       yes                   yes
eulexistdb           0.21.1            Apache-2.0                                                  no                    no
eulxml               1.1.3             Apache-2.0                                                  no                    no
eXistDB              2.2               LGPL                                                        no                    no
flake8               3.8.4             MIT                                                         no                    no
flaky                3.7.0             Apache-2.0                                                  no                    no
freetype             2.5.2             FTL/GPLv2                                                   no                    yes
gmock                1.7.0             BSD                                                         no                    no
hdf5                 1.8.12            BSD                                                         yes                   no
h5py                 2.7.1             BSD                                                         no                    no
httplib2             0.9.1             MIT                                                         yes                   yes
idna                 2.7               PSFL                                                        yes                   no
importlib-metadata   3.3.0             apache                                                      yes                   no
iniconfig [pyt]      1.1.1             MIT                                                         yes                   yes
ipcluster-tools      0.0.11            BSD-3                                                       yes                   no
ipykernel            4.3.1             BSD-3-Clause                                                yes                   yes
ipyparallel          5.1.1             BSD-3-Clause                                                yes                   no
ipython              7.19.0            BSD-3-Clause                                                yes                   yes
ipython-genutils     0.2.0             BSD-3-Clause                                                yes                   yes
jedi                 0.17.2            MIT                                                         yes                   yes
jpeg                 9a                Ack                                                         yes                   yes
Jinja2               2.7.2             BSD                                                         no                    no
jsonschema           2.3.0             MIT                                                         yes                   yes
jupyter-client       6.1.6             BSD                                                         yes                   no
jupyter-core         4.6.3             BSD                                                         yes                   no
lapack               3.6.0             BSD                                                         yes                   no
libev-git            4.33dev           BSD/GPLv2                                                   yes                   no
libpng               1.6.8             libpng (MIT like)                                           yes                   yes
libxml2              2.9.10            MIT                                                         yes                   yes
libxslt              1.1.38            MIT                                                         yes                   yes
libzmq               4.2.5             LGPLv3                                                      yes                   yes
log4cpp              1.1.3             LGPLv2.1                                                    yes                   no
lxml                 3.6.4             BSD                                                         yes                   no
MarkupSafe           0.18              BSD                                                         no                    no
matplotlib           2.1.1             PSFL                                                        no                    no
more-itertools       8.6.0             MIT                                                         yes                   no
mqtt_cpp             10.0.0            Boost SL 1                                                  yes                   no
msgpack              0.5.6             APL2                                                        no                    no
msgpack-numpy        0.4.3             BSD                                                         no                    no
multidict            1.5.0             Apache-2.0                                                  yes                   no
nbformat             4.1.0             BSD                                                         yes                   yes
nose                 1.3.0             LGPL                                                        no                    no
notebook             4.2.2             BSD                                                         yes                   yes
nss                  ?                 MPL                                                         yes                   no
numpy                1.21.4            BSD                                                         yes                   yes
openmq               5.0.1             EPL/GPLv2                                                   yes                   yes
paho.mqtt.python     1.5.1             EPLv1/EDLv1                                                 yes                   no
packaging            20.8              apache/BSD                                                  yes                   no
pamqp                2.3.0             BSD-3-Clause                                                yes                   no
parse                1.6.3             BSD                                                         no                    no
parso                0.7.1             MIT                                                         no                    no
patchelf             0.8               GPLv3                                                       no                    no
pexpect              4.8.0             ISC license (BSD like)                                      yes                   yes
pg8000               1.21.2            BSD                                                         yes                   no
pickleshare          0.7.5             MIT                                                         yes                   yes
Pillow               2.5.3             PIL (MIT like)                                              no                    yes
Pint                 0.17              BSD-3-Clause                                                yes                   yes
pip                  7.1               MIT                                                         yes                   yes
pkgconfig            1.2.2             MIT                                                         yes                   yes
pluggy               0.13.1            MIT                                                         yes                   no
ply                  3.11              BSD                                                         yes                   no
prompt-toolkit       3.0.10            BSD-3-Clause                                                yes                   yes
ptyprocess           0.7.0             ISCL                                                        yes                   no
psutil               4.3.1             BSD                                                         no                    no
pugixml              1.2               MIT                                                         yes                   no
py                   1.10.0            MIT                                                         yes                   no
pybind11             2.6.1             MIT                                                         yes                   no
pycodestyle          2.6.0             MIT                                                         no                    no
pyelftools           0.24              Public Domain                                               no                    no
pyflakes             2.2.0             MIT                                                         no                    no
Pygments             2.7.4             BSD                                                         yes                   yes
pyparsing            2.4.7             MIT                                                         no                    yes
pyqt                 5.9.2             GPLv3/Commercial                                            no                    yes
pyqtgraph            0.11.0            MIT                                                         no                    yes
pytest               6.2.1             MIT                                                         no                    no
pytest-runner        2.11.1            MIT                                                         no                    no
pytz                 2013.9            MIT                                                         no                    yes
PyYAML               3.12              MIT                                                         no                    no
pyzmq                22.3.0            LGPL+BSD                                                    yes                   yes
qtconsole            4.2.1             BSD                                                         yes                   yes
qt                   5.9.7             GPLv3/Commercial                                            no                    yes
qtpy                 1.9               MIT                                                         no                    yes
redisclient          1.0.2dev          MIT                                                         yes                   no
requests             2.19.1            APLv2                                                       no                    no
rpathology           0.0.1             MIT                                                         no                    no
scikit-learn         0.14.1            BSD                                                         no                    no
scipy                1.6.3             BSD                                                         no                    no
setuptools           39.1.0            MIT                                                         yes                   yes
setuptools-scm       1.15.6            MIT                                                         yes                   yes
simplegeneric        0.8.1             ZPLv2.1 (BSD plus trademark)                                yes                   yes
six                  1.15.0            MIT                                                         yes                   yes
tiff                 4.4.1             libtiff license (BSD like)                                  no                    no
tornado              6.0.4             APLv2                                                       yes                   no
toml                 0.10.2            MIT                                                         yes                   no
traitlets            5.0.5             BSD                                                         yes                   yes
traits               4.6.0             BSD                                                         yes                   yes
tzlocal              1.1.1             MIT                                                         yes                   yes
urllib3              1.23              MIT                                                         yes                   no
wcwidth              0.2.5             MIT                                                         yes                   yes
wheel                0.24.0            MIT                                                         yes                   yes
yarl                 1.6.3             Apache-2.0                                                  yes                   no
zipp                 1.0.0             MIT                                                         yes                   no
==================== ================= =========================================================== ===================== =========================


In order to disentangle the dependencies' structure, it is convenient to split the structure as follow:
The graph below represents the karabo libraries (please note that the graph below represents the goal
of a refactoring that is in progress):

.. digraph:: karabo_libraries

    "karathon" -> "karabo-cpp"
    "karabogui" -> "karabo.common"
    "karabogui" -> "karabo.native"
    "karabo.middlelayer" -> "karabo.native"
    "karabo.middlelayer" -> "karabo.common"
    "karabo.middlelayer_devices" -> "karabo.middlelayer"
    "karabo.middlelayer_devices" -> "karabo.project_db"
    "karabo.bound" -> "karabo.common"
    "karabo.bound" -> "karathon"
    "karabo.bound_devices" -> "karabo.project_db"
    "karabo.bound_devices" -> "karabo.bound"

Here are the dependencies of the ``karabo-cpp`` python module:

.. digraph:: karabocpp_dependencies

    "karabo-cpp" -> "openmq"
    "karabo-cpp" -> "boost"
    "karabo-cpp" -> "hdf5"
    "karabo-cpp" -> "redisclient"
    "karabo-cpp" -> "amqpcpp"
    "karabo-cpp" -> "mqtt_cpp"
    "mqtt_cpp" -> "boost"
    "redisclient" -> "boost"
    "boost" -> "libxml2"
    "boost" -> "libxslt"
    "libxml2" -> "bzip2"
    "libxslt" -> "bzip2"
    "amqpcpp" -> "libev"

Here are the dependencies of the ``karabo.common`` python module:

.. digraph:: karabocommon_dependencies

    "karabo.common" -> "traits"

Here are the dependencies of the ``karabo.native`` python sub-module:

.. digraph:: karabonative_dependencies

    "karabo.native" -> "lxml"
    "lxml" -> "libxml2"
	"karabo.native" -> "Pint"
    "karabo.native" -> "numpy"
    "karabo.native" -> "python-dateutil"
	"python-dateutil" -> "six"

Here are the dependencies of the ``karabo.project_db`` python sub-module:

.. digraph:: karaboprojectdb_dependencies

	"karabo.project_db" -> "eulexistdb"
	"karabo.project_db" -> "psutil"
	"eulxml" -> "ply"
	"eulxml" -> "lxml"
	"eulxml" -> "six"
	"eulexistdb"
	"eulexistdb" -> "requests"
	"eulexistdb" -> "eulxml"
	"requests" -> "chardet"
	"requests" -> "idna"
	"requests" -> "urllib3"
	"requests" -> "certify"

Here are the dependencies of the ``karabo.middlelayer`` python sub-module, for the sake of clarity,
the ``ipython``, ``numpy`` and ``jupyter_client`` modules are not expanded in their dependencies:

.. digraph:: karabomiddlelayer_dependencies

    "karabo.middlelayer" -> "lxml"
    "karabo.middlelayer" -> "IPython"
    "karabo.middlelayer" -> "jupyter_client"
    "karabo.middlelayer" -> "aio-pika"
    "karabo.middlelayer" -> "paho.mqtt.python"
    "karabo.middlelayer" -> "aioredis"
    "aio-pika" -> "aiormq"
    "aiormq" -> "pamqp"
    "aiormq" -> "yarl"
    "yarl" -> "multidict"

Here are the dependencies of the ``karabogui`` python sub-module, for the sake of clarity,
the ``ipython``, ``numpy`` and ``jupyter_client`` modules are not expanded in their dependencies:

	"karabogui" -> "karabo.common"
	"karabogui" -> "karabo.native"
	"karabogui" -> "pyqt"
	"pyqt" -> "qt5"
	"karabogui" -> "pythonqwt"
	"karabogui" -> "guiqwt"
	"karabogui" -> "qtconsole"
	"karabogui" -> "matplotlib"
	"karabogui" -> "ipython"
	"matplotlib" -> "numpy"
	"matplotlib" -> "six"
	"matplotlib" -> "python-dateutil"
	"matplotlib" -> "pytz"
	"matplotlib" -> "cycler"
	"matplotlib" -> "pyparsing"
	"qtconsole" -> "jupyter_client"
	"qtconsole" -> "traitlets"
	"qtconsole" -> "pygments"
	"qtconsole" -> "jupyter_core"
	"qtconsole" -> "ipykernel"
	"karabogui" -> "pyzmq"
	"karabogui" -> "pyqtgraph"
	"pyqtgraph" -> "numpy "
	"cycler" -> "six"
	"karabogui" -> "requests"


Here are the dependencies of the ``ipython``, ``numpy`` and ``jupyter_client``:

.. digraph:: ipythonnumpyjupyter_dependencies

	"ipython" -> "decorator"
    "ipython" -> "pickleshare"
    "ipython" -> "traitlets"
    "ipython" -> "prompt_toolkit"
    "ipython" -> "pygments"
    "ipython" -> "backcall"
    "ipython" -> "pexpect"
	"prompt_toolkit" -> "six"
	"prompt_toolkit" -> "wcwidth"
	"jupyter_client" -> "traitlets"
	"jupyter_client" -> "pyzmq"
	"jupyter_client" -> "jupyter_core"
	"jupyter_core" -> "traitlets"
	"ipykernel" -> "ipyparallel"
	"ipyparallel" -> "notebook"
    "ipykernel" -> "ipython"
    "ipykernel" -> "traitlets"
    "ipykernel" -> "jupyter_client"
    "ipykernel" -> "tornado"
    "ipykernel" -> "dill"
	"notebook" -> "jsonschema"
	"notebook" -> "nbformat"
	"numpy" -> "lapack"
	"numpy" -> "cython"
	"ipython_genutils" -> "ipython"

Here are the dependencies that are **not** needed by framework, but might be needed
during development:

.. digraph:: notderivative_dependencies

	"ipcluster-tools"
	"ipcluster-tools" -> "ipython"
	"ipcluster-tools" -> "pytest"
	"daemontools"
	"scipy"
	"parse"
	"backports.ssl-match-hostname"
	"backcall"
	"slumber" -> "requests"
	"msgpack-numpy" -> "numpy"
	"msgpack-numpy" -> "msgpack"
	"pyelftools"
	"pyusb"
	"PyYAML"
	"pycodestyle"
	"pyflakes"
	"flake8"
	"flake8" -> "pyflakes"
	"flake8" -> "pycodestyle"
	"msgpack"
	"flaky"
	"docker-pycreds"
	"docker-pycreds" -> "six"
	"websocket-client"
	"websocket-client" -> "six"
	"docker"
	"docker" -> "requests"
	"docker" -> "six"
	"docker" -> "websocket_client"
	"docker" -> "docker_pycreds"
	"coverage"
	"rpathology"
	"nose"
	"py"
	"pytest"
	"pytest" -> "py"
	"pytest-runner"
	"backports-abc"
	"jsonschema"
	"ipython"
	"ipyparallel"
	"ipykernel"
	"guiqwt"
	"graphviz"
	"setuptools"
	"setuptools-scm"
	"scipy"
	"h5py"
	"h5py" -> "numpy"
	"h5py" -> "six"

Remarks on miniconda3 Windows CI
================================

Our release process for Windows is now done on a shared Windows 10 runner.
This runner was configured manually by means of installing `miniconda3`, `plink`
and `cwrsync` on our home folder (*C:\Users\xkarabo*) and are all added on `xkarabo`
path. Currently the GitLab CI logs in as a system user, so we have to
manually add these environment variables each time the job is executed
(see **.gitlab-ci.yaml**). Also, `cwrsync's ssh` tool needs the **HOME** variable
set to be %USERPROFILE%.

Also, for the Windows CI as we don't have an easy-to-use tool like `sshpass`
we have created an RSA key and added it to our linux server (*exflctrl01*). The
key on Windows is located on `%USERPROFILE%\.ssh\win-cwrsync`.

Code used for building the recipe
---------------------------------

Our building process has three steps:

The first step for the release is to create (solve) the environment based on our
`environment.devenv.file`. This environment is used to generate our recipe's
`meta.yaml` based on a template called `meta_base.yaml` using a very well known
code generator called `cogapp`. After this file is generated, we can delegate
the build process to `conda-build`. When successful, we will have our package
inside `<conda_directory>/conda-bld/<platform>/`.

After the karabogui package is built, we also need to populate our mirror channel based on
the package's dependencies. For this we developed a script called
`create_mirror_channels.py` which decides which packages to download using the
`conda-mirror` tool. The advantage to have a mirror is that the deployment is much
faster and we have the safety of having our internal channel.

Possible Issues
===============

Differently from our Linux CI, the Windows CI is not started fresh at each run,
so it's possible that some issues arise during the release process. We try to
mitigate most of them by some cleaning process on `ci/miniconda/build.cmd`.

Some errors that were met were:


`Not a conda environment: <environment path>`
---------------------------------------------

The environment got corrupted somehow. Fix it by removing it manually:

    conda remove -n <environment_name> --all --yes

    or

    conda env remove -n <environment_name> --all

Package conflicts on test phase
-------------------------------

Usually it's an error like the following

    Found conflicts! Looking for incompatible packages.
    This can take several minutes.  Press CTRL-C to abort.
    failed

    Package libtiff conflicts for:
    pyqtgraph==0.11.0=py_1 -> pyqt -> qt=5.6 -> libtiff=4.0
    karabogui==2.7.0a5=py36_0 -> pillow==6.2.1=py36h5fcff3f_1 -> libtiff[version='>=4.1.0,<5.0a0']
    karabogui==2.7.0a5=py36_0 -> libtiff==4.1.0=h21b02b4_1
    libtiff==4.1.0=h21b02b4_1
    Package pygments conflicts for:
    karabogui==2.7.0a5=py36_0 -> pygments[version='2.4.2|2.5.0',build=py_0]
    karabogui==2.7.0a5=py36_0 -> ipython==7.2.0=py36h39e3cac_1000 -> pygments
    qtconsole==4.6.0=py_0 -> pygments
    ipykernel==5.1.3=py36h5ca1d4c_0 -> ipython[version='>=5.0'] -> pygments
    ipython==7.2.0=py36h39e3cac_1000 -> jedi[version='>=0.10']]


Either this means:

* An actual conflicting of dependencies
* One of the packages are not available on the desired platform
* A dirty conda build cache

On our scenario, as we always solve the environment before the build (in order
to decide which packages we use), the first two options are not viable. By cleaning
the conda build cache it usually works:

    conda build purge-all

If it doesn't, try cleaning everything in conda:

    conda clean --all --yes

If it doesn't, it might be a bug generated by an update on the conda package.
Try downgrading it:

    conda install -n base conda=<lower_version>

Remarks on Licensing
====================

For the Karabo Framework, excluding the GUI, we plan to use a Mozilla Public License
version 2.0 , which foresees as weak form of copy-left.

for more information:
https://www.mozilla.org/en-US/MPL/2.0/FAQ/

The GUI would be released initially as GPLv3, which is required by the PyQt5
library the GUI uses.
A future more permissive license is possible, but would require factoring out
the GPLv3 dependency, which we do not deem necessary as of now.

A note on the General Public License
------------------------------------

Licensing the framework with stricter copy-left licenses like the General Public
License (GPL) is not possible due to conflicts between the software license of
some of the dependencies used. The OpenMQ C library is dual licensed with the
Eclipse Public License version 2.0 (EPL-2.0) and the GPL version 2.0 (GPL-2.0).
GPL-2.0 is in conflict with the Apache Software License version 2 (Apache-2.0)
that is used by multiple libraries and this precludes
the use of GPL-2.0 and the strict definition of the GPL included in OpenMQ precludes
the use of other GPL versions.

For more information regarding the EPL-2.0 dual licensing:
https://www.eclipse.org/legal/epl-2.0/faq.php

License for the Karabo GUI
--------------------------

The Karabo GUI depends from a library that has a strong copyleft license.
The PyQT5 graphical user interface library is licensed under GPLv3 or Commercial.
This limits the licenses usable for the Karabo GUI to GPLv3.

The Karabo GUI is however currently using an abstraction layer that will allow
the use of a less strictly licensed library and if the need arises or we wish to,
the Karabo GUI code could be relicensed using a more permissive license.

A note regarding Karabo Plugins (Devices)
-----------------------------------------

The Mozilla Public License (MPL) extends to all files containing code licensed
under the MPL. As a consequence of this, all code using plugins can be licensed
and released with a license of the choice of the authors as long as the distribution
of such code and binaries complies with the license of Karabo and its dependencies.

A Note on Copyright
===================

The European XFEL GmbH is the copyright owner of the code. All contributions must
include this copyright notice.
