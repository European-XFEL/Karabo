Karabo SCADA Framework
======================

The Supervisory Control and Data Acquisition (SCADA) framework Karabo. Please
see our [contributing guidelines](CONTRIBUTING.md) before reporting an issue.

## License

This work is multi-licensed under Mozilla Public License 2.0 (Core) and GPL
3.0 (pythonGui).
The applicable license depends on the location of a file in the repository tree.
The MPL2.0 license applies for all files except those located in src/pythonGui
and subtrees therein. Accordingly, the GPLv3.0 license applies to any files
location in src/pythonGui and its subtrees.

# Copyright notice:

    Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

    Karabo is free software: you can redistribute it and/or modify it under
    the terms of the appropriate licenses mentioned above.

    Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE."""

# Available Operating Systems

The supported OS are:

> Almalinux 8/9
> Ubuntu 20.04/22.04 LTS
> Centos 7
> RedHat 9

The GUI application is available under:

> Ubuntu 20.04/22.04 LTS
> Windows 7/10
> MacOSX

# Building

## Building from command-line ###

The quickest way to build the Karabo framework is by using
the `auto_build_all.sh` script. Simply execute:

    ./auto_build_all.sh

and get informed how to use this script. The build of the C++ components of the
Karabo framework is controlled by the CMake project at `src/CMakeLists.txt`.
Tests have shown that the builds are performed faster when `ninja` is used as
the CMake generator. If `ninja` is available on your Linux installation, the
build uses it instead of the default `Unix Makefiles` generator.


# Documentation

An extensive documentation is available in the `doc` folder. Build the
documentation using

    cd doc
    pip install -r requirements.txt
    make html

Or use the documentation available at
this [link](https://karabo.pages.xfel.eu/Framework/index.html)

Thank you for using Karabo!
