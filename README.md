# Copyright notice:

> Copyright (C) European XFEL GmbH Hamburg. All rights reserved.


# Available Operating Systems

The supported OS are:

> Ubuntu 20.04 LTS
> Ubuntu 18.04 LTS
> Centos 7

The GUI application is furthermore available under:

> Windows 7/10
> MacOSX

# Building

## Building from command-line ###

The quickest way to build the Karabo framework is by using the auto_build_all.sh script. Simply execute:

    ./auto_build_all.sh

and get informed how to use this script.

## Formatting compliance

In order to have your commits accepted by the CI's linting jobs, your modified Python source files must be compliant
with `flake8` and `isort` and your modified C++ source files must be compliant with
the `clang-format` settings in the `.clang-format` file at the root of the
repository.

The CI linting job currently use `clang-format 13.0.0`. Please make sure
that you have this version available on your development system.

There are instructions on how to setup a development system based on Visual Studio Code at [(https://rtd.xfel.eu/docs/karabo/en/latest/tools/vscode.html)](https://rtd.xfel.eu/docs/karabo/en/latest/tools/vscode.html) (which corresponds to
file `doc/tools/vscode.rst` in the repository). As a result of executing those instructions, you'll have
a Conda environment with `clang-format 13.0.0` installed and Visual Studio Code using that `clang-format`
instance to format all the C++ source files in the project at save time.

To manually run the CI linting job on your local system, please execute the following script:

    $REPO_ROOT/ci/lint

If you followed the setup instructions for Visual Studio Code, please activate the Conda environment
with `clang-format` installed before running the CI linting job:

    conda activate karabo-cpp

Another script, `src/devScripts/lint_modified.sh`, allows all modified
source files in the Framework that are not yet staged to be formatted with
`clang-format` (C++ files) or with `isort` (Python files). This script also checks the modified Python files for PEP8 compliance with `flake8`.

There is a pre-commit Git hook at `src/devScripts/pre-commit-hook` that you should enable if you want to be warned early about C++ source formatting that will be rejected by the CI lint job. To enable that pre-commit hook, please create a symbolic link named `pre-commit` at directory `.git/hooks` referencing `src/devScripts/pre-commit-hook`:

    ln -s $REPO_ROOT/src/devScripts/pre-commit-hook $REPO_ROOT/.git/hooks/pre-commit

where `REPO_ROOT` is the path of the root directory of your local Framework repository.

# Documentation

An extensive documentation is available in the `doc` folder.
Build the documentation using

    cd doc
    pip install -r requirements.txt
    make html

Or use the documentation available at this [link](https://rtd.xfel.eu/docs/karabo/en/latest/)
or the old [link](https://in.xfel.eu/readthedocs/docs/karabo/en/latest/)

Thank you for using Karabo!
