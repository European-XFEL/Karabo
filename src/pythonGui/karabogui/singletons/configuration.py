#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on September 16, 2018
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################
from collections import defaultdict
from typing import Any

from qtpy.QtCore import QObject, QSettings

from .util import decrypt, encrypt


def convert_value(value, dtype):
    """Convert a value to a QSettings value. This is mainly done for booleans
    """
    if dtype is bool:
        return str(value).lower()
    return value


class Item:
    __slots__ = (
        "name", "default", "q_set", "group", "path", "editable", "dtype")

    def __init__(self, default: Any = None,
                 q_set: bool = False,
                 group: str | None = None,
                 editable: bool = False,
                 dtype: int | float | bool | None = None) -> None:
        self.name: str = ""
        self.default = default
        self.q_set = q_set
        self.group = group
        self.path: str = ""
        self.editable = editable
        self.dtype = dtype

    def __get__(self, instance: "Item", owner: type["Item"]) -> Any:
        if instance is None:
            return self
        else:
            return instance.__dict__.get(self.name, self.default)

    def __set__(self, instance: "Item", value: Any) -> None:
        instance.__dict__[self.name] = value
        if self.q_set:
            QSettings().setValue(self.path, convert_value(value, self.dtype))

    def __set_name__(self, owner: type["Item"], name: str) -> None:
        self.name = name
        self.path = f"{self.group}/{self.name}"
        if self.q_set:
            self.default = self.get_shared_value()

    def get_shared_value(self) -> Any:
        """Initialize a value from `QSettings` file"""
        value = QSettings().value(self.path)
        if not value or self.dtype is None:
            return value or self.default
        # Actively stored values
        if self.dtype is bool:
            return str(value).lower() == "true"
        elif self.dtype is int:
            return int(value)
        elif self.dtype is float:
            return float(value)
        return value

    def toDict(self) -> dict:
        return {name: getattr(self, name) for name in
                ["editable", "name", "dtype"]}

    def erase(self) -> None:
        QSettings().remove(self.path)

    def __str__(self) -> str:
        return self.name


class SharedItem(Item):
    """A SharedItem in the Configuration is by default storing into QSettings
    """

    def __init__(self, group: str | None = None,
                 editable: bool = False,
                 dtype: int | float | bool | None = None) -> None:
        super().__init__(default=None, q_set=True, group=group,
                         editable=editable, dtype=dtype)

    def __get__(self, instance: "SharedItem",
                owner: type["SharedItem"]) -> Any:
        if instance is None:
            return self
        value = self.get_shared_value()
        return decrypt(value, self.group) if value is not None else value

    def __set__(self, instance: "SharedItem", value: Any) -> None:
        value = convert_value(value, self.dtype)
        if value is not None:
            value = encrypt(value, self.group)
        QSettings().setValue(self.path, value)

    def __set_name__(self, owner: type["SharedItem"], name: str) -> None:
        self.name = name
        self.path = f"{self.group}/{self.name}"
        self.default = None


AUTHENTICATION = "authentication"
NETWORK = "network"
PROJECT = "project"
BACKBONE = "backbone"
DIRECTORIES = "dir"
USER = "user"


class Configuration(QObject):
    """The main dispatch point for Karabo Configurations and QSettings.

    An Item is constructed via the descriptor protocol.

    pool = Item(default="fish", q_set=False, group="POOL")

    - The first parameter ``default`` provides - as the name says - the
      default value. This value might be overwritten on initialization if
      there is a presetting in QSettings.

    - The ``qset`` parameter activates the QSetting functionality. Items with
      this parameter set to True, store their value in a QSetting file and load
      this on the next GUI startup and overwrite an eventual default value.

    - The ``editable`` parameter defines if this item is user editable

    - The ``group`` parameter defines an additional string for sorting and
      grabbing the Items for a model view.

    - The ``dtype`` parameter defines the data dtype. If a dtype is specified,
      a default value must be provided.

    NOTE: Every parameter has a cache principle, meaning that if an additional
          client is opened on the same machine, both clients do not interact
          via QSettings on runtime.

    NOTE: Not every parameter is supposed to have a QSetting.

          - db tokens or other sensible data which should not leave the GUI.
          - parameters, such as broker topic, which might not be send always
            as they will take a default value from QSettings next time, which
            might be wrong.
    """
    broker_topic = Item(q_set=False, group=BACKBONE)

    # ----------------------------------------------
    # Last directories stored and used

    data_dir = Item(q_set=True, group=DIRECTORIES)

    # ----------------------------------------------
    # MainWindow, Wizard and DPI

    wizard = Item(default=True, q_set=True, group=USER, editable=True,
                  dtype=bool)
    check_updates = Item(default=True, q_set=True, group=USER, editable=True,
                         dtype=bool)
    highDPI = Item(default=True, q_set=True, group=USER, editable=False,
                   dtype=bool)
    main_geometry = Item(q_set=True, group=USER)
    development = Item(default=False, q_set=False, group=USER, dtype=bool)
    project_sort_column = Item(default=0, q_set=True, group=USER, dtype=int)
    project_sort_order = Item(default=0, q_set=True, group=USER, dtype=int)
    logbook_stream = Item(q_set=True, group=USER)
    logbook_topic = Item(q_set=True, group=USER)
    logbook_header_style = Item(q_set=True, group=USER)

    # ----------------------------------------------
    # Project db interface

    domain = Item(q_set=True, group=PROJECT)

    # ----------------------------------------------
    # GUI Server network connection

    username = Item(default="operator", q_set=True, group=NETWORK)
    access_level = Item(default="operator", q_set=True, group=NETWORK)
    gui_servers = Item(default=[], q_set=True, group=NETWORK)
    macro_development = Item(q_set=True, group=NETWORK)

    # ----------------------------------------------
    # Shared authentication interface

    refresh_token_user = SharedItem(group=AUTHENTICATION)
    refresh_token = SharedItem(group=AUTHENTICATION)

    def __new__(cls, *args, **kwargs):
        instance = super().__new__(cls, *args, **kwargs)
        # Memory filling avoiding __setattr__!
        memory = [getattr(cls, attr).name for attr in dir(cls)
                  if isinstance(getattr(cls, attr), Item)]
        instance.__dict__["_memory"] = memory
        return instance

    def __init__(self, parent=None):
        super().__init__(parent)

    def __setitem__(self, key, value):
        if key not in self._memory:
            raise KeyError(f"Configuration key is not valid: {key}")
        super().__setattr__(key, value)

    def __getitem__(self, item):
        if item not in self._memory:
            raise KeyError(f"Configuration key is not valid: {item}")
        return super().__getattribute__(item)

    def __contains__(self, key):
        return key in self._memory

    def __iter__(self):
        for key in self._memory:
            yield key, super().__getattribute__(key)

    def items(self):
        for key in self._memory:
            yield key, super().__getattribute__(key)

    def values(self):
        for key in self._memory:
            yield super().__getattribute__(key)

    def keys(self):
        yield from self._memory

    def groups(self):
        """Return a dictionary with groups as keys and config keys as values
        """
        ret = defaultdict(list)
        for key in self._memory:
            group = getattr(self.__class__, key).group
            if group == AUTHENTICATION:
                continue
            ret[group].append(getattr(self.__class__, key))
            ret[group] = sorted(ret[group], key=lambda i: i.name)
        return dict(sorted(ret.items()))

    def __delitem__(self, key):
        """Convenient shortcut to erase configuration information"""
        assert key in self._memory
        item = getattr(self.__class__, key)
        item.erase()

    def info(self):
        """Return the relevant user meta data information from the config

        This method only accounts `USER` group information.
        """
        ret = {USER: {}}
        for key in self._memory:
            group = getattr(self.__class__, key).group
            if group != USER:
                continue
            ret[group][key] = getattr(self, key)
        return ret

    def __len__(self):
        return len(self._memory)

    def __setattr__(self, key, value):
        if key not in self._memory:
            raise KeyError(f"Configuration key is not valid: {key}")
        super().__setattr__(key, value)

    def __repr__(self):
        names = "\n".join("\t" + attr + ": " + repr(getattr(self, attr))
                          for attr in self._memory)
        return f"{{\n{names}\n}}"
