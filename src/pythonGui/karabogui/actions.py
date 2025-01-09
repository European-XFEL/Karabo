#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 26, 2016
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
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QAction
from traits.api import Bool, Callable, HasStrictTraits, Instance, String


class KaraboAction(HasStrictTraits):
    """Base class for actions"""
    # The icon for this action
    icon = Instance(QIcon)
    # The text label for the action
    text = String
    # The tooltip text shown when hovering over the action
    tooltip = String
    # Whether or not this action is checkable
    checkable = Bool(False)
    # Whether or not this action is checked
    is_checked = Bool(False)
    # Defines the method which is called whenever the action is triggered
    triggered = Callable
    # Object name which can be used for finding actions programmatically
    name = String


def build_qaction(karabo_action, parent):
    """ Create a QAction from given ``karabo_action``

    NOTE: since the callback for the ``triggered`` Callable should be defined
    where it is needed and might have different signatures, the actual
    ``connect`` of a QAction should be done after calling this function.

    :param karabo_action: An object of type ``KaraboAction``
    :param parent: The parent of the ``QAction`` of type ``QObject``
    :return: ``QAction`` objects based on given ``karabo_action``
    """
    q_action = QAction(karabo_action.text, parent)
    # Attach ``KaraboAction`` object to its respective QAction
    q_action.setData(karabo_action)
    if karabo_action.icon is not None:
        q_action.setIcon(karabo_action.icon)
    q_action.setCheckable(karabo_action.checkable)
    if karabo_action.checkable:
        q_action.setChecked(karabo_action.is_checked)
    q_action.setStatusTip(karabo_action.tooltip)
    q_action.setToolTip(karabo_action.tooltip)
    if karabo_action.name:
        # Only if name is not empty
        q_action.setObjectName(karabo_action.name)
    return q_action
