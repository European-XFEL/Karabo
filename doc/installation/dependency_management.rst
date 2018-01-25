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
the framework locally using this pakcage:

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
