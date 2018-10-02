#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on September 16, 2018
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import QObject

from karabogui.enums import KaraboSettings
from karabogui.util import get_setting, set_setting


class Item:
    __slots__ = ["name", "default", "q_set"]

    def __init__(self, name, default=None, q_set=None):
        # XXX: Once we have the new descriptor protocol, name gets removed!
        self.name = name
        self.q_set = q_set
        if self.q_set is not None:
            self.default = get_setting(self.q_set) or default
        else:
            self.default = default

    def __get__(self, instance, owner):
        if instance is None:
            return self
        else:
            return instance.__dict__.get(self.name, self.default)

    def __set__(self, instance, value):
        instance.__dict__[self.name] = value
        if self.q_set:
            set_setting(self.q_set, value)

    def __str__(self):
        return self.name


class Configuration(QObject):
    """The main dispatch point for Karabo Configurations and QSettings.
    """

    broker_topic = Item('broker_topic', q_set=KaraboSettings.BROKER_TOPIC)
    macro_server = Item('macro_server', default='karabo/macroServer',
                        q_set=KaraboSettings.MACRO_SERVER)

    # ----------------------------------------------
    # Last directories stored and used

    config_dir = Item('config_dir', q_set=KaraboSettings.CONFIG_DIR)
    macro_dir = Item('macro_dir', q_set=KaraboSettings.MACRO_DIR)
    scene_dir = Item('scene_dir', q_set=KaraboSettings.SCENE_DIR)

    # ----------------------------------------------
    # Project db interface

    db_token = Item('db_token', default='admin')
    domain = Item('domain', default='CAS_INTERNAL',
                  q_set=KaraboSettings.PROJECT_DOMAIN)

    # ----------------------------------------------
    # GUI Server network connection

    username = Item('username', q_set=KaraboSettings.USERNAME)
    guiServers = Item('guiServers', q_set=KaraboSettings.GUI_SERVERS)

    def __new__(cls, *args, **kwargs):
        instance = super(Configuration, cls).__new__(cls, *args, **kwargs)
        # Memory filling avoiding __setattr__!
        memory = [getattr(cls, attr).name for attr in dir(cls)
                  if isinstance(getattr(cls, attr), Item)]
        instance.__dict__['_memory'] = memory
        return instance

    def __init__(self, parent=None):
        super(Configuration, self).__init__(parent)

    def __setitem__(self, key, value):
        if key not in self._memory:
            raise KeyError("Configuration key is not valid: {}".format(key))
        super(Configuration, self).__setattr__(key, value)

    def __getitem__(self, item):
        if item not in self._memory:
            raise KeyError("Configuration key is not valid: {}".format(item))
        return super(Configuration, self).__getattribute__(item)

    def __contains__(self, key):
        return key in self._memory

    def __iter__(self):
        for key in self._memory:
            yield key, super(Configuration, self).__getattribute__(key)

    def items(self):
        for key in self._memory:
            yield key, super(Configuration, self).__getattribute__(key)

    def values(self):
        for key in self._memory:
            yield super(Configuration, self).__getattribute__(key)

    def keys(self):
        for key in self._memory:
            yield key

    def __len__(self):
        return len(self._memory)

    def __setattr__(self, key, value):
        if key not in self._memory:
            raise KeyError("Configuration key is not valid: {}".format(key))
        super(Configuration, self).__setattr__(key, value)

    def __repr__(self):
        names = '\n'.join('\t' + attr + ': ' + repr(getattr(self, attr))
                          for attr in self._memory)
        return '{{\n{}\n}}'.format(names)
