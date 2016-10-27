#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QAction, QIcon

from traits.api import HasStrictTraits, Bool, Callable, Instance, String


class KaraboAction(HasStrictTraits):
    """ Base class for actions
    """
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


def build_qaction(karabo_action, parent):
    """ Create a QAction from given ``karabo_action``

    NOTE: since the callback for the ``triggered`` Callable should be defined
    where it is needed and might have different signatures, the actual
    ``connect`` of a QAction should be done after calling this function.

    :param karabo_action: An object of type ``KaraboAction``
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
    return q_action
