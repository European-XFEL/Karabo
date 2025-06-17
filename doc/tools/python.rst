..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

***************************
Getting Started - PyCharms
***************************

Simply follow the installation instructions for the community edition at
`https://www.jetbrains.com/pycharm/`_

Configuring PyCharms
++++++++++++++++++++

* Wrong margins

By default PyCharms comes with a right margin of 120 character. In
order to apply pep8 settings go to::

    File -> Settings -> Editor -> Code Style

and set the right margin to 79 characters (or "Hard wrap at <put 79 here> columns", depending on pycharm version).

* Trailing white spaces

Trailing white spaces are by default neither visible or removed. You
have two options to deal with them. You can go either to::

    File -> Settings -> Editor -> General

and look in *Other* to *Strip trailing on save* (choose "All" or "Modified lines" here).

.. note::

   You might encounter two issues related to 'Always keep trailing space on caret line' checkbox.

   - If you keep this checkbox ticked, white spaces won't be stripped from the current line.

   - If you uncheck it, annoying disappearing of your whitespaces might happen, when you type your text and make a pause.

     That is because of default autosaving.
     Possible solution: disable autosaving and save file in a traditional "Ctrl+S" way:

     Go to File > Settings (Ctrl+Alt+S).
     Go to Appearance & Behavior > System Settings > Synchronization.
     Make sure the two are unchecked:

        -Save files on frame deactivation

        -Save files automatically if application is idle for x sec

     After auto-saving is disabled, also it could be useful to apply the next settings:

     Go to Editor > General > Editor Tabs, put a checkmark on "Mark modified files with asterisk"

     (Optional but recommended)
     Under "Tab Closing Policy", select "Close unchanged" option of "When tabs exceed the limit" radiobox.

     You may also want to increase the number of allowed tabs.

     Click Apply > OK.

Or, as an alternative way, you can change your IDE appearance::

    File -> Settings -> Editor -> General -> Appearance

and select the *Show white spaces* option.


Use Karabo's Python as Interpreter
++++++++++++++++++++++++++++++++++

Navigate to File->Default Settings. Click on "Default Project", "Project
Interpreter". Then next to the top drop-down menu, click on the little gear
symbol and "Add Local". Now navigate to your Karabo installation folder, and
there into "extern/bin" and select ``python``.

************************
Getting Started - Flake8
************************

If you do not already have pip installed (a Python package manager) do so::

    sudo apt-get -y install python3-pip

Then install Flake8::

    sudo pip3 install flake8

.. note::

   Make sure you install the Python3 version

Using Flake8
++++++++++++

Navigate to the directory containing the python file you would like to check
against PEP8 compliance, then execute ::

    flake8 yourPythonFile.py

you will see an output similar to this::

    flake8 worker.py
    worker.py:1:1: F401 'sys' imported but unused
    worker.py:6:1: E302 expected 2 blank lines, found 1
    worker.py:7:1: W293 blank line contains whitespace
    worker.py:8:32: E251 unexpected spaces around keyword / parameter equals
    worker.py:8:34: E251 unexpected spaces around keyword / parameter equals
    worker.py:8:48: E251 unexpected spaces around keyword / parameter equals
    worker.py:8:50: E251 unexpected spaces around keyword / parameter equals
    worker.py:8:65: E251 unexpected spaces around keyword / parameter equals


Here the first number is the line number, the second the character number of
the problem's location. Simply go through these problems and reformat your code
as suggested and following the style guide provided here:
`https://www.python.org/dev/peps/pep-0008/`_. Note that expected parameter
sections in `python.bound` should be formatted like this::

    def expectedParameters(expected):
        (
            STRING_ELEMENT(expected).key("_deviceId_")
            .displayedName("_DeviceID_")
            .description("Do not set this property, it will be set "
                         "by the device-server")
            .expertAccess().assignmentInternal().noDefaultValue().init()
            .commit(),

            NODE_ELEMENT(expected).key("_connection_")
            .displayedName("Connection")
            .description("The connection to the communication layer "
                         "of the distributed system")
            .appendParametersOf(JmsConnection)
            .adminAccess()
            .commit(),
        )

Documentation
-------------

 * Use double backticks (\`\`name\`\`) to document variables inside a method

Python Classes
--------------

 * Per convention, deriving from a super class and call methods should be done
   like this:

       `super(ClassName, self)`

 * Do not use names of builtins for variables (e.g. `object`)
