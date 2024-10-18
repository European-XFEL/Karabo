..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

.. _installation/dependency_management:

External Dependency Management in Karabo
========================================

The Karabo Framework depends on a large number of third party packages.
The karabo C++ library dependencies are managed using the conan open
source package manager. This automatically provides things like Python,
boost, AMQP, etc.

Because building these dependencies has become a time consuming and sensitive
process, they are now being managed by an automated system. Together with the
CI (continuous integration) features of GitLab, the dependencies are
automatically built and cached whenever one or more of those
dependencies changes.


Installing the Dependencies into a Clean Source Tree
----------------------------------------------------

After cloning the framework from git, or doing a clean rebuild, the external
dependencies should first be installed. This is handled automatically by the
``auto_build_all.sh`` script.

The already-built dependencies will be downloaded from the public conan
repository when available. If pre-built binaries are not available, then the
source code will be download and compiled locally according to the script
in the conanfile.py of the dependency package.


Mechanism for Determining Correct Dependencies to Install
---------------------------------------------------------

The conan package manager enables building the karabo framework with a
deterministic build environment. All of the build options, compiler
and host system settings are used to compute a package ID for the dependency.
The package ID is a unique identifier that conan uses to determine package
compatibility with a host system and other dependencies.

If you are interested in a more detailed explanation of how this works, you
can read more about it at blog.conan.io:

https://blog.conan.io/2019/09/27/package-id-modes.html




Remarks on Windows CI
=====================

Our release process for Windows is now done on a shared Windows 10 runner.
This runner was configured manually by means of installing `miniforge`, `plink`
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
mitigate most of them by some cleaning process on `ci/conda/build.cmd`.

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
