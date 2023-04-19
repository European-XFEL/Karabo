#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 23, 2016
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
import importlib

from qtpy.QtWidgets import QApplication

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
    the ``karabogui`` package. A getter function will also get a custom
    creator function which uses the above information to import and instantiate
    the singleton. This uses ``importlib.import_module`` and ``getattr`` to
    resolve the class object.

    Finally, each singleton's class is expected to have an __init__ method
    which takes a single keyword argument ``parent`` which in our case will
    always be the global ``QApplication`` instance.
    """
    def creator():
        module = importlib.import_module(modulename, 'karabogui')
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
get_alarm_model = _build_getter('alarm_model', '.alarms.api',
                                'AlarmModel')
get_db_conn = _build_getter('db_conn', '.singletons.db_connection',
                            'ProjectDatabaseConnection')
get_manager = _build_getter('manager', '.singletons.manager', 'Manager')
get_mediator = _build_getter('mediator', '.singletons.mediator', 'Mediator')
get_network = _build_getter('network', '.singletons.network', 'Network')
get_panel_wrangler = _build_getter('panel_wrangler',
                                   '.singletons.panel_wrangler',
                                   'PanelWrangler')
get_selection_tracker = _build_getter('selection_tracker',
                                      '.singletons.selection_tracker',
                                      'SelectionTracker')
get_project_model = _build_getter('project_model', '.singletons.project_model',
                                  'ProjectViewItemModel')
get_topology = _build_getter('topology', '.topology.api', 'SystemTopology')
get_config = _build_getter('configuration', '.singletons.configuration',
                           'Configuration')

# Hide our implementation detail
del _build_getter
