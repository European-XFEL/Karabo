***************************
Getting Started - PyCharms
***************************

Simply follow the installation instructions for the community edition at
`https://www.jetbrains.com/pycharm/`_

Use Karabo's Python as Interpreter
++++++++++++++++++++++++++++++++++

Navigate to File->Default Settings. Click on "Default Project", "Project
Interpreter". Then next to the top drop-down menu, click on the little gear
symbol and "Add Local". Now navigate to your Karabo installation folder, and
there into "extern/bin" and select python3.4.

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
sections in `python.bound` should be formatted like this, regardless of flake8
complaints::

    def expectedParameters(expected):
        (
            STRING_ELEMENT(expected).key("_serverId_")
                .displayedName("_ServerID_")
                .description("Do not set this property, it will be"
                             " set by the device-server")
                .expertAccess().assignmentInternal().noDefaultValue().init()
                .commit(),

            STRING_ELEMENT(expected).key("_deviceId_")
                .displayedName("_DeviceID_")
                .description("Do not set this property, it will be set"
                             " by the device-server")
                .expertAccess().assignmentInternal().noDefaultValue().init()
                .commit(),

            NODE_ELEMENT(expected).key("_connection_")
                .displayedName("Connection")
                .description("The connection to the communication layer"
                             " of the distributed system")
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

       `super(className, self)`

 * Do not use names of builtins for variables (e.g. `object`)