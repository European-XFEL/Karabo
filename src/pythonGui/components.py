#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a base class for all inherited
   factory classes for creation of certain widgets and bundles main functionalities.
"""

__all__ = ["BaseComponent"]


import manager
from network import Network
import icons

from layouts import ProxyWidget
from registry import Loadable
from const import ns_karabo
from messagebox import MessageBox
from widget import EditableWidget, DisplayWidget, Widget

from PyQt4.QtCore import QObject, pyqtSignal, pyqtSlot, QSize, QTimer, Qt
from PyQt4.QtGui import QAction, QColor, QHBoxLayout, QLabel, QMenu, \
                        QToolButton, QWidget

import numpy
import numbers

class BaseComponent(Loadable, QObject):
    factories = EditableWidget.factories

    signalValueChanged = pyqtSignal(object, object) # key, value


    def __init__(self, classAlias, box):
        super(BaseComponent, self).__init__()
        self.classAlias = classAlias
        box.signalNewDescriptor.connect(self.widgetFactory.typeChanged)
        if box.descriptor is not None:
            self.widgetFactory.typeChanged(box)


    def save(self, e):
        """saves this component into the ElementTree.Element e"""
        d = { }
        e.set(ns_karabo + "class", self.__class__.__name__)
        e.set(ns_karabo + "widget", type(self.widgetFactory).__name__)
        e.set(ns_karabo + "keys", ",".join(b.key()
                                           for b in self.widgetFactory.boxes))
        self.widgetFactory.save(e)


    @classmethod
    def load(cls, elem, layout):
        boxes = []
        for k in elem.get(ns_karabo + 'keys').split(","):
            deviceId, path = k.split('.', 1)
            conf = manager.getDevice(deviceId)
            conf.addVisible()
            boxes.append(conf.getBox(path.split(".")))
        #commandEnabled=elem.get(ns_karabo + "commandEnabled") == "True"
        parent = ProxyWidget(layout.parentWidget())
        component = cls(elem.get(ns_karabo + "widget"), boxes[0], parent)
        parent.setComponent(component)
        parent.setWidget(component.widget)
        layout.loadPosition(elem, parent)
        for b in boxes[1:]:
            component.addBox(b)
        component.widgetFactory.load(elem)
        elem.clear()
        return component


    @property
    def boxes(self):
        return self.widgetFactory.boxes


class DisplayComponent(BaseComponent):
    factories = DisplayWidget.factories


    def __init__(self, classAlias, box, parent, widgetFactory="DisplayWidget"):

        W = Widget.widgets.get(classAlias)
        if W is None:
            self.widgetFactory = DisplayWidget.factories[widgetFactory].\
                                   getClass(classAlias)(box, parent)
            super(DisplayComponent, self).__init__(classAlias, box)
        else:
            self.widgetFactory = W(box, parent)
            super(DisplayComponent, self).__init__(W.alias, box)
        self.widgetFactory.setReadOnly(True)
        self.connectWidget(box)


    def connectWidget(self, box):
        box.signalUpdateComponent.connect(self.widgetFactory.valueChanged)
        if box.hasValue():
            self.widgetFactory.valueChanged(box, box.value, box.timestamp)


    def addBox(self, box):
        if self.widgetFactory.addBox(box):
            self.connectWidget(box)
            return True
        return False


    def _getWidgetCategory(self):
        return self.widgetFactory.category
    widgetCategory = property(fget=_getWidgetCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.widgetFactory.widget
    widget = property(fget=_getWidget)


    def _getDisplayWidget(self):
        return self.widgetFactory
    displayWidget = property(fget=_getDisplayWidget)


    def _getKeys(self):
        return self.widgetFactory.keys
    keys = property(fget=_getKeys)


    def _getValue(self):
        return self.widgetFactory.value
    def _setValue(self, value):
        self.widgetFactory.value = value
    value = property(fget=_getValue, fset=_setValue)


    def setErrorState(self, isError):
        self.widgetFactory.setErrorState(isError)


    def removeKey(self, key):
        self.widgetFactory.removeKey(key)


    def destroy(self):
        for key in self.widgetFactory.keys:
            self.removeKey(key)


    def changeWidget(self, factory, alias):
        self.classAlias = alias
        oldWidget = self.widgetFactory.widget
        self.widgetFactory = factory.getClass(alias)(
            self.boxes[0], oldWidget.parent())
        self.widgetFactory.setReadOnly(True)
        self.connectWidget(self.boxes[0])
        for b in self.boxes[1:]:
            self.widgetFactory.addBox(b)
            self.connectWidget(b)
        oldWidget.parent().setWidget(self.widgetFactory.widget)
        oldWidget.setParent(None)
        self.widgetFactory.widget.show()


class EditableNoApplyComponent(BaseComponent):
    """ These components are used while editing the initial parameters
    of a class. """
    def __init__(self, classAlias, box, parent, widgetFactory=None):
        self.__compositeWidget = QWidget(parent)
        hLayout = QHBoxLayout(self.__compositeWidget)
        hLayout.setContentsMargins(0,0,0,0)

        W = Widget.widgets.get(classAlias)
        if W is None:
            self.widgetFactory = EditableWidget.getClass(classAlias)(
                                        box, self.__compositeWidget)
            super(EditableNoApplyComponent, self).__init__(classAlias, box)
        else:
            self.widgetFactory = W(box, self.__compositeWidget)
            super(EditableNoApplyComponent, self).__init__(W.alias, box)
        if box.hasValue():
            self.widgetFactory.valueChanged(box, box.value, box.timestamp)
        self.widgetFactory.setReadOnly(False)
        self.widgetFactory.signalEditingFinished.connect(self.onEditingFinished)
        hLayout.addWidget(self.widgetFactory.widget)

        unitLabel = (box.descriptor.metricPrefixSymbol +
                     box.descriptor.unitSymbol)

        if unitLabel:
            hLayout.addWidget(QLabel(unitLabel))


    def _getWidgetCategory(self):
        return self.widgetFactory.category
    widgetCategory = property(fget=_getWidgetCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__compositeWidget
    widget = property(fget=_getWidget)


    def _getKeys(self):
        return self.widgetFactory.keys
    keys = property(fget=_getKeys)


    def _getValue(self):
        return self.widgetFactory.value
    value = property(fget=_getValue)


    def setEnabled(self, enable):
        self.widget.setEnabled(enable)


    def addParameters(self, **params):
        self.widgetFactory.addParameters(**params)


    def addKeyValue(self, key, value):
        self.widgetFactory.addKeyValue(key, value)


    def removeKey(self, key):
        pass


    def destroy(self):
        for key in self.widgetFactory.keys:
            manager.Manager().unregisterEditableComponent(key, self)


    def changeWidget(self, factory, proxyWidget, alias):
        self.classAlias = alias
        self.__initParams['value'] = self.value

        oldWidget = self.widgetFactory.widget
        oldWidget.deleteLater()
        self.widgetFactory = factory.getClass(alias)(**self.__initParams)
        self.widgetFactory.setReadOnly(False)
        proxyWidget.setWidget(self.widgetFactory.widget)
        self.widgetFactory.widget.show()

        # Refresh new widget...
        for key in self.widgetFactory.keys:
            Network().onRefreshInstance(key)


    def onEditingFinished(self, box, value):
        box.set(value, None)


class EditableApplyLaterComponent(BaseComponent):
    # signals
    signalConflictStateChanged = pyqtSignal(str, bool) # key, hasConflict
    signalApplyChanged = pyqtSignal(object, bool) # key, state of apply button


    def __init__(self, classAlias, box, parent, widgetFactory=None):
        self.__currentDisplayValue = None

        self.__compositeWidget = QWidget(parent)
        hLayout = QHBoxLayout(self.__compositeWidget)
        hLayout.setContentsMargins(0,0,0,0)

        W = Widget.widgets.get(classAlias)
        if W is None:
            self.widgetFactory = EditableWidget.getClass(classAlias)(
                                            box, self.__compositeWidget)
            super(EditableApplyLaterComponent, self).__init__(classAlias, box)
        else:
            self.widgetFactory = W(box, self.__compositeWidget)
            super(EditableApplyLaterComponent, self).__init__(W.alias, box)
        self.widgetFactory.setReadOnly(False)
        self.widgetFactory.signalEditingFinished.connect(self.onEditingFinished)
        hLayout.addWidget(self.widgetFactory.widget)

        self.box = box
        d = box.descriptor
        if d is not None:
            unitLabel = d.metricPrefixSymbol + d.unitSymbol
            if unitLabel:
                hLayout.addWidget(QLabel(unitLabel))

        self.hasConflict = False

        text = "Apply"
        self.acApply = QAction(icons.applyGrey, text, self)
        self.acApply.setStatusTip(text)
        self.acApply.setToolTip(text)
        self.acApply.triggered.connect(self.onApplyClicked)
        tb = QToolButton()
        tb.setDefaultAction(self.acApply)
        tb.setPopupMode(QToolButton.InstantPopup)
        tb.setIconSize(QSize(24, 24))
        hLayout.addWidget(tb)

        text = "Reset"
        self.acReset = QAction(icons.no, text, self)
        self.acReset.setStatusTip(text)
        self.acReset.setToolTip(text)
        self.acReset.triggered.connect(self.onApplyRemoteChanges)
        tb = QToolButton()
        tb.setDefaultAction(self.acReset)
        tb.setIconSize(QSize(24, 24))
        hLayout.addWidget(tb)

        self.__busyTimer = QTimer(self)
        self.__busyTimer.setSingleShot(True)
        self.__busyTimer.timeout.connect(self.onTimeOut)
        self._applyEnabled = False

        # In case of attributes (Hash-V2) connect another function here
        self.signalConflictStateChanged.connect(
            manager.Manager().onConflictStateChanged)

        box.signalUpdateComponent.connect(self.onDisplayValueChanged)
        if box.hasValue():
            self.onDisplayValueChanged(box, box.value)
        box.configuration.value.state.signalUpdateComponent.connect(
            self.updateButtons)


    def _getWidgetCategory(self):
        return self.widgetFactory.category
    widgetCategory = property(fget=_getWidgetCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__compositeWidget
    widget = property(fget=_getWidget)


    def _getValue(self):
        return self.widgetFactory.value
    value = property(fget=_getValue)


    def setEnabled(self, enable):
        self.widget.setEnabled(enable)


    def addParameters(self, **params):
        self.widgetFactory.addParameters(**params)


    @property
    def applyEnabled(self):
        return self._applyEnabled


    @applyEnabled.setter
    def applyEnabled(self, value):
        emit = value != self._applyEnabled
        self._applyEnabled = value
        if emit:
            self.signalApplyChanged.emit(self.boxes[0], value)


    def addKeyValue(self, key, value):
        self.widgetFactory.addKeyValue(key, value)


    def removeKey(self, key):
        pass


    def destroy(self):
        for key in self.widgetFactory.keys:
            manager.Manager().unregisterEditableComponent(key, self)


    def changeWidget(self, factory, alias):
        self.classAlias = alias

        oldWidget = self.widgetFactory.widget
        self.widgetFactory = factory.getClass(alias)(
            self.box, oldWidget.parent())
        self.widgetFactory.setReadOnly(False)
        self.widgetFactory.signalEditingFinished.connect(self.onEditingFinished)
        oldWidget.parent().layout().insertWidget(0, self.widgetFactory.widget)
        oldWidget.setParent(None)
        self.widgetFactory.widget.show()

        for c in {b.configuration for b in self.widgetFactory.boxes}:
            c.refresh()


    # Slot called when changes need to be sent to Manager
    def onApplyClicked(self):
        self.__busyTimer.start(5000)
        Network().onReconfigure([(b, self.widgetFactory.value)
                                 for b in self.boxes])


    def onApplyRemoteChanges(self, key):
        self.widgetFactory.valueChanged(key, self.__currentDisplayValue)
        self.updateButtons()


    def onTimeOut(self):
        MessageBox.showWarning(
            "The attribute couldn't be set in the current state.")


    @pyqtSlot(str, object)
    def onDisplayValueChanged(self, key, value):
        if self.__currentDisplayValue is None:
            self.widgetFactory.valueChanged(key, value)
        self.__currentDisplayValue = value
        self.__busyTimer.stop()
        self.hasConflict = True
        self.updateButtons()


    def updateButtons(self):
        """ update the buttons to reflect the current state of affairs """
        allowed = self.boxes[0].isAllowed()
        self.acApply.setEnabled(allowed)

        EPSILON = 1e-4
        value = self.__currentDisplayValue
        if value is None:
            return

        if (isinstance(value, (numbers.Complex, numpy.inexact))
                and not isinstance(value, numbers.Integral)):
            diff = abs(value - self.widgetFactory.value)
            isEqualEditable = diff < EPSILON
        elif isinstance(value, list):
            if len(value) != len(self.widgetFactory.value):
                isEqualEditable = False
            else:
                for i in xrange(len(value)):
                    if value[i] != self.widgetFactory.value[i]:
                        isEqualEditable = False
                        break
                isEqualEditable = True
        else:
            isEqualEditable = (str(value) == str(self.widgetFactory.value))

        if isEqualEditable:
            self.acApply.setIcon(icons.applyGrey)
            self.hasConflict = False
            text = None
        elif self.hasConflict:
            self.acApply.setIcon(icons.applyConflict)
            text = "Resolve conflict"
        else:
            text = "Apply local changes"
            self.acApply.setIcon(icons.apply)
        self.acApply.setStatusTip(text)
        self.acApply.setToolTip(text)
        self.applyEnabled = allowed and not isEqualEditable


    def onEditingFinished(self, key, value):
        if self.__currentDisplayValue is None:
            return
        self.updateButtons()


class ChoiceComponent(BaseComponent):
    def __init__(self, classAlias, box, parent, widgetFactory=None):
        W = Widget.widgets.get(classAlias)
        if W is None:
            self.widgetFactory = EditableWidget.getClass(classAlias)(
                                                    box, parent)
            super(ChoiceComponent, self).__init__(classAlias, box)
        else:
            self.widgetFactory = W(box, parent)
            super(ChoiceComponent, self).__init__(W.alias, box)
        self.widget.setEnabled(False)
        box.signalUpdateComponent.connect(self.widgetFactory.valueChanged)
        if box.hasValue():
            self.widgetFactory.valueChanged(box, box.value, box.timestamp)


    def _getWidgetCategory(self):
        return self.widgetFactory.category
    widgetCategory = property(fget=_getWidgetCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.widgetFactory.widget
    widget = property(fget=_getWidget)


    def _getValue(self):
        return self.widgetFactory.value
    def _setValue(self, value):
        self.widgetFactory.value = value
    value = property(fget=_getValue, fset=_setValue)


    def setEnabled(self, enable):
        # Is not processed due to self.widget should always stay disabled
        pass


    def addParameters(self, **params):
        self.widgetFactory.addParameters(**params)


    # Triggered by DataNotifier signalAddKey
    def addKeyValue(self, key, value):
        pass


    # Triggered by DataNotifier signalRemoveKey
    def removeKey(self, key):
        pass
