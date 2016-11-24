#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 23, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import importlib

from PyQt4.QtGui import QApplication

# module global for storing singleton instances
__singletons = {}


def _build_getter(key, modulename, klassname):
    """A factory function for building singleton getter functions

    This is an internal detail, but some explanation would be helpful.
    Basically, the pattern for instantiating a singleton is to import a class
    from some module, then instantiate and store the result in a module global
    which is then accessible from some module-level getter function.

    The generalization of that is this: We have a single module-level dict
    which contains our singleton instances named ``__singletons``. For each
    singleton we define a getter function which checks for the existence of a
    specific singleton instance in the dict and returns it, or creates and adds
    the instance to the dict and then returns it.

    To create a getter function, we need three pieces of information:
    a key for the singletons dict, a module where the singleton's class can
    be imported, and the name of that class. The module is given relative to
    the ``karabo_gui`` package. A getter function will also get a custom
    creator function which uses the above information to import and instantiate
    the singleton. This uses ``importlib.import_module`` and ``getattr`` to
    resolve the class object.

    Finally, each singleton's class is expected to be derived from QObject and
    take a single keyword argument ``parent`` which in our case will always be
    the global ``QApplication`` instance.
    """
    def creator():
        module = importlib.import_module(modulename, 'karabo_gui')
        klass = getattr(module, klassname)

        app = QApplication.instance()
        instance = klass(parent=app)
        __singletons[key] = instance
        return instance

    def getter():
        if key in __singletons:
            return __singletons[key]
        return creator()

    return getter

# This is where all the singletons are defined
get_manager = _build_getter('manager', '.singletons.manager', 'Manager')
get_meditator = _build_getter('mediator', '.singletons.mediator', 'Mediator')
get_network = _build_getter('network', '.singletons.network', 'Network')

# XXX: To add: MainWindow instance, Topology object

# Hide our implementation detail
del _build_getter
