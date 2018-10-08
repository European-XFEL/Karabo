#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on September 16, 2018
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import defaultdict

from PyQt4.QtCore import QObject, QSettings


class Item:
    __slots__ = ["name", "default", "q_set", "group", "path"]

    def __init__(self, name, default=None, q_set=False, group=None):
        # XXX: Once we have the new descriptor protocol, name gets removed!
        self.name = name

        self.q_set = q_set
        self.group = group
        self.path = "{group}/{name}".format(group=group, name=name)
        if self.q_set:
            self.default = QSettings().value(self.path) or default
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
            QSettings().setValue(self.path, value)

    def __str__(self):
        return self.name


NETWORK = "network"
PROJECT = "project"
ECOSYSTEM = "ecosystem"
DIRECTORIES = "dir"


class Configuration(QObject):
    """The main dispatch point for Karabo Configurations and QSettings.

    An Item is constructed via the descriptor protocol.

    pool = Item('pool', defaultValue='fish', q_set=False, group=POOL)

    - The first parameter is the name, which must correspond to the
      property name.
    - The second parameter defaultValue provides - as the name says - the
      default value. This value might be overwritten on initialization if
      there is a presetting in QSettings.
    - The qset parameter activates the QSetting functionality. Items with this
      parameter set to True, store their value in a QSetting file and load
      this on the next GUI startup and overwrite an eventual default value.
    - The group parameter defines an additional string for sorting and grabbing
      the Items for a later model view.

    NOTE: Every parameter has a cache principle, meaning that if an additional
          client is opened on the same machine, both clients do not interact
          via QSettings on runtime.

    NOTE: Not every parameter is supposed to have a QSetting
          - db tokens or other sensible data which should not leave the GUI.
          - parameters, such as broker topic, which might not be send always
            as they will take a default value from QSettings next time, which
            might be wrong.
    """

    broker_topic = Item('broker_topic', q_set=False, group=ECOSYSTEM)
    macro_server = Item('macro_server', default='karabo/macroServer',
                        q_set=True, group=ECOSYSTEM)
    project_manager = Item('project_manager', default='KaraboProjectDB',
                           q_set=False, group=ECOSYSTEM)

    # ----------------------------------------------
    # Last directories stored and used

    config_dir = Item('config_dir', q_set=True, group=DIRECTORIES)
    macro_dir = Item('macro_dir', q_set=True, group=DIRECTORIES)
    scene_dir = Item('scene_dir', q_set=True, group=DIRECTORIES)

    # ----------------------------------------------
    # Project db interface

    db_token = Item('db_token', default='admin', group=PROJECT)
    domain = Item('domain', default='CAS_INTERNAL', q_set=True,
                  group=PROJECT)

    # ----------------------------------------------
    # GUI Server network connection

    username = Item('username', default='operator', q_set=True,
                    group=NETWORK)
    gui_servers = Item('gui_servers', default=(), q_set=True,
                       group=NETWORK)

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

    def groups(self):
        """Return a dictionary with groups as keys and config keys as values
        """
        ret = defaultdict(set)
        for key in self._memory:
            group = getattr(self.__class__, key).group
            ret[group].add(key)
        return ret

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
