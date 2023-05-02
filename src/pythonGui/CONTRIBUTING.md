Contributing Code to KaraboGUI
==============================

Although mentioned in the `CONTRIBUTING.md` of the root folder, we kindly
mention again the necessity to ask for a signed `Contributor License Agreement` (CLA):

- people contributing as individuals should sign the Individual CLA
- people contributing on behalf of an organization should sign the Entity CLA.

The CLAs can be found in the `Contributor_License_Agreement.md` and
`Entity_Contributor_License_Agreement.md` documents located in the root folder
of this repository. Please send signed CLAs to opensource [at] xfel.eu. We'll
get in touch with you then.


Setting up development environment
----------------------------------

For developing in **KaraboGUI** you need to
have `miniconda3` `installed <https://docs.conda.io/en/latest/miniconda.html>`_
and added `conda-forge` to the channels::

    conda config --add channels conda-forge

After installing your `miniconda3` distribution, install the package
``conda-devenv`` from the conda-forge channel::

    conda install conda-devenv -c conda-forge

``conda-devenv`` is a tool for creating a development environment that always
follows the dependencies specified in your environment.devenv.yml, purging any
other dependencies left behind. Now run::

    conda devenv --file conda-recipes/karabogui/environment.devenv.yml

This will solve your environment dependencies and create an environment
called ``karabogui``. Call ``conda activate karabogui`` to activate it. Still,
the Karabo code has to be installed::

    cd src/pythonKarabo
    pip install -e . --no-deps
    cd ../../
    cd src/pythonGui pip
    install -e . --no-deps

Now all the code from ``karabogui``, ``common`` and ``native`` will be on
your ``PYTHONPATH``.

Finally, generate the version file using::

    python setup.py develop

inside the ``pythonGui`` directory. Repeat that step from time to time to keep
the version number up-to-date.


Workflow
--------

In general, it is advised to start on a new feature by initiating a discussion first, e.g. by creating a Github `issue` or
commenting on an existing one.
The KaraboGUI code development follows the **trunk based development**
approach.

1. Checkout the trunk (main) and pull the latest changes
2. Branch off per topic, e.g. `git checkout -b my-new-feature`
3. Commit changes to the topic branch
4. If you branch consists of multiple commits, please rebase interactively::

   git rebase -i main

5. Forward your branch to the repository with `git push origin my-new-feature`

Create a public merge request and await review with guidance on the changes.
Always try to back feature branches with test coverage.


Design
------

By design, in model-view-controller programming, we separate out functionality to
controllers based on the *traits* package for type checking, user interface library
independent testing and event signals.

- Python Coding Style:

  Code should be *isorted* and style-checked using *flake8*. An isort
  configuration file is located in the `pythonGui` folder. Double quotes ("")
  should be used around strings instead of single quotes('') and triple-double
  quotes (""") should be used instead of triple-single quotes ('''). We prefer
  hanging indentation.

- Naming:

  Classes should be named in PascalCase and methods and instance variables
  should be in snake_case, except when overriding a third-party
  method/variable (like PyQt). Qt signal names should be in camelCase, and
  should clearly indicate what the signal is associated with and what type of
  event it represents (e.g. buttonClicked, columnDataRequested). Always spell
  out the variable without truncating, e.g. `server` instead of `srv`. For user
  interface variables it is recommended to mangle the type of the QObject,
  e.g. `server_button` into the member name. Do not use names of builtins for
  variables (e.g. `object`)

- PyQt Specific:

  Always try to decorate the slot method with "@Slot" and declare appropriate
  argument types. Keep the .ui files in the 'ui' directory at the same level as
  the .py file.

- Testing:

  Always try to add at least simple unit tests for a new feature.
  Even only importing and creating objects further helps to maintain the code base
  significantly.

- Documentation:

  Always try to add a docstring for new code, including tests, unless the name
  of the class/method/function is self-explanatory. Document the pyqt signal,
  including when it is emitted and its arguments. Overridden methods should
  have a docstring to mention how they behave differently from the parent
  method.

- Dependencies:

  The Qt version of the `Karabo GUI` is provided by the community, e.g. we use
  packages like *qtpy* and the conda qt-feedstock to run. However, only the
  *PyQt* package is tested and supported. *PySide* is best-effort. For plotting
  features, the library *PyQtGraph* should be used. The `Karabo GUI` must run
  on different OS: MacOS, Windows and Linux. Hence, the *pathlib* module
  should be used whenever os path functionality has to be used.
