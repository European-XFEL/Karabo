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
import icons

from layouts import ProxyWidget
from registry import Loadable
from const import ns_karabo
from messagebox import MessageBox
from widget import EditableWidget, DisplayWidget

from PyQt4.QtCore import QObject, pyqtSignal, pyqtSlot, QSize, QTimer, Qt
from PyQt4.QtGui import QAction, QColor, QHBoxLayout, QLabel, QMenu, \
                        QToolButton, QWidget

import numpy
import numbers

class BaseComponent(Loadable, QObject):
    factories = EditableWidget.factories

    signalValueChanged = pyqtSignal(object, object) # key, value


    def __init__(self, classAlias):
        super(BaseComponent, self).__init__()

        self.classAlias = classAlias

        # States, if the widget is associated with an online device from the distributed system
        self.__isOnline = False
        # Gives the position of the widget in the global coordinate system
        self.__windowPosition = None


    def attributes(self):
        """ returns a dict of attibutes for saving """
        d = { }
        d[ns_karabo + "class"] = self.__class__.__name__
        d[ns_karabo + "widgetFactory"] = self.widgetFactory.factory.__name__
        d[ns_karabo + "classAlias"] = self.classAlias
        d[ns_karabo + "keys"] = ",".join(b.key()
                                         for b in self.widgetFactory.boxes)
        return d


    @classmethod
    def load(cls, elem, layout):
        ks = "classAlias", "keys", "widgetFactory"
        if elem.get(ns_karabo + "classAlias") == "Command":
            ks += "command", "allowedStates", "commandText"
        d = {k: elem.get(ns_karabo + k) for k in ks}
        boxes = []
        for k in d['keys'].split(","):
            deviceId, path = k.split('.', 1)
            conf = manager.Manager().getDevice(deviceId)
            conf.addVisible()
            boxes.append(conf.getBox(path.split(".")))
        #commandEnabled=elem.get(ns_karabo + "commandEnabled") == "True"
        parent = ProxyWidget(layout.parentWidget())
        component = cls(d['classAlias'], boxes[0], parent, d['widgetFactory'])
        parent.setComponent(component)
        parent.addWidget(component.widget)
        layout.loadPosition(elem, parent)
        for b in boxes[1:]:
            component.addBox(b)
        return component


class DisplayComponent(BaseComponent):
    factories = DisplayWidget.factories


    def __init__(self, classAlias, box, parent, widgetFactory="DisplayWidget"):
        super(DisplayComponent, self).__init__(classAlias)

        self.__displayWidget = DisplayWidget.factories[widgetFactory].getClass(
            classAlias)(box, parent)
        self.__displayWidget.setReadOnly(True)


    def _getWidgetCategory(self):
        return self.__displayWidget.category
    widgetCategory = property(fget=_getWidgetCategory)


    def _getWidgetFactory(self):
        return self.__displayWidget
    widgetFactory = property(fget=_getWidgetFactory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__displayWidget.widget
    widget = property(fget=_getWidget)


    def _getDisplayWidget(self):
        return self.__displayWidget
    displayWidget = property(fget=_getDisplayWidget)


    def _getKeys(self):
        return self.__displayWidget.keys
    keys = property(fget=_getKeys)


    @property
    def boxes(self):
        return self.__displayWidget.boxes


    def _getValue(self):
        return self.__displayWidget.value
    def _setValue(self, value):
        self.__displayWidget.value = value
    value = property(fget=_getValue, fset=_setValue)


    def setErrorState(self, isError):
        self.__displayWidget.setErrorState(isError)


    def addBox(self, box):
        return self.__displayWidget.addBox(box)


    def removeKey(self, key):
        manager.Manager().unregisterDisplayComponent(key, self)
        self.__displayWidget.removeKey(key)


    def destroy(self):
        for key in self.__displayWidget.keys:
            self.removeKey(key)


    def changeWidget(self, factory, alias):
        self.classAlias = alias
        oldWidget = self.__displayWidget.widget
        self.__displayWidget = factory.getClass(alias)(
            self.boxes[0], oldWidget.parent())
        self.__displayWidget.setReadOnly(True)
        for b in self.boxes[1:]:
            self.__displayWidget.addBox(b)
        oldWidget.parent().addWidget(self.__displayWidget.widget)
        oldWidget.parent().removeWidget(oldWidget)
        oldWidget.parent().setCurrentWidget(self.__displayWidget.widget)
        oldWidget.setParent(None)
        self.__displayWidget.widget.show()


class EditableNoApplyComponent(BaseComponent):
    def __init__(self, classAlias, box, parent, widgetFactory=None):
        super(EditableNoApplyComponent, self).__init__(classAlias)

        self.__compositeWidget = QWidget(parent)
        hLayout = QHBoxLayout(self.__compositeWidget)
        hLayout.setContentsMargins(0,0,0,0)

        self.__editableWidget = EditableWidget.getClass(classAlias)(
            box, self.__compositeWidget)
        self.__editableWidget.setReadOnly(False)
        self.__editableWidget.signalEditingFinished.connect(self.onEditingFinished)
        hLayout.addWidget(self.__editableWidget.widget)

        unitLabel = (box.descriptor.metricPrefixSymbol +
                     box.descriptor.unitSymbol)

        if unitLabel:
            hLayout.addWidget(QLabel(unitLabel))


    def copy(self):
        copyComponent = EditableNoApplyComponent(self.classAlias, **self.__initParams)
        return copyComponent


    def _getWidgetCategory(self):
        return self.__editableWidget.category
    widgetCategory = property(fget=_getWidgetCategory)


    def _getWidgetFactory(self):
        return self.__editableWidget
    widgetFactory = property(fget=_getWidgetFactory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__compositeWidget
    widget = property(fget=_getWidget)


    def _getKeys(self):
        return self.__editableWidget.keys
    keys = property(fget=_getKeys)


    @property
    def boxes(self):
        return self.__editableWidget.boxes


    def _getValue(self):
        return self.__editableWidget.value
    value = property(fget=_getValue)


    def setEnabled(self, enable):
        self.widget.setEnabled(enable)


    def addParameters(self, **params):
        self.__editableWidget.addParameters(**params)


    def addKeyValue(self, key, value):
        self.__editableWidget.addKeyValue(key, value)


    def removeKey(self, key):
        pass


    def destroy(self):
        for key in self.__editableWidget.keys:
            manager.Manager().unregisterEditableComponent(key, self)


    def changeWidget(self, factory, proxyWidget, alias):
        self.classAlias = alias
        self.__initParams['value'] = self.value

        oldWidget = self.__editableWidget.widget
        oldWidget.deleteLater()
        self.__editableWidget = factory.getClass(alias)(**self.__initParams)
        self.__editableWidget.setReadOnly(False)
        proxyWidget.setWidget(self.__editableWidget.widget)
        self.__editableWidget.widget.show()

        # Refresh new widget...
        for key in self.__editableWidget.keys:
            manager.Manager().onRefreshInstance(key)


    def onEditingFinished(self, box, value):
        box.set(value, None)


class EditableApplyLaterComponent(BaseComponent):
    # signals
    signalConflictStateChanged = pyqtSignal(str, bool) # key, hasConflict
    signalApplyChanged = pyqtSignal(object, bool) # key, state of apply button


    def __init__(self, classAlias, box, parent, widgetFactory=None):
        super(EditableApplyLaterComponent, self).__init__(classAlias)

        self.__isEditableValueInit = True
        self.__currentDisplayValue = None

        self.__compositeWidget = QWidget(parent)
        hLayout = QHBoxLayout(self.__compositeWidget)
        hLayout.setContentsMargins(0,0,0,0)

        self.__editableWidget = EditableWidget.getClass(classAlias)(
            box, self.__compositeWidget)
        self.__editableWidget.setReadOnly(False)
        self.__editableWidget.signalEditingFinished.connect(self.onEditingFinished)
        hLayout.addWidget(self.__editableWidget.widget)

        self.box = box
        d = box.descriptor
        if d is not None:
            unitLabel = d.metricPrefixSymbol + d.unitSymbol
            if unitLabel:
                hLayout.addWidget(QLabel(unitLabel))

        self.__hasConflict = False

        text = "Apply"
        self.__acApply = QAction(icons.applyGrey, text, self)
        self.__acApply.setStatusTip(text)
        self.__acApply.setToolTip(text)
        self.__acApply.triggered.connect(self.onApplyClicked)
        tb = QToolButton()
        tb.setDefaultAction(self.__acApply)
        tb.setPopupMode(QToolButton.InstantPopup)
        tb.setIconSize(QSize(24, 24))
        hLayout.addWidget(tb)

        # Add menu to toolbutton
        text = "Apply local changes"
        self.__mApply = QMenu(tb)
        self.__acApplyChanges = QAction(text, self)
        self.__acApplyChanges.setStatusTip(text)
        self.__acApplyChanges.setToolTip(text)
        self.__acApplyChanges.triggered.connect(self.onApplyClicked)
        self.__mApply.addAction(self.__acApplyChanges)

        text = "Accept current value on device"
        self.__acApplyRemoteChanges = QAction(text, self)
        self.__acApplyRemoteChanges.setStatusTip(text)
        self.__acApplyRemoteChanges.setToolTip(text)
        self.__acApplyRemoteChanges.triggered.connect(self.onApplyRemoteChanges)
        self.__mApply.addAction(self.__acApplyRemoteChanges)

        text = "Reset"
        self.__acReset = QAction(icons.no, text, self)
        self.__acReset.setStatusTip(text)
        self.__acReset.setToolTip(text)
        self.__acReset.setEnabled(False)
        self.__acReset.triggered.connect(self.onApplyRemoteChanges)
        tb = QToolButton()
        tb.setDefaultAction(self.__acReset)
        tb.setIconSize(QSize(24, 24))
        # Add to layout
        hLayout.addWidget(tb)

        self.__busyTimer = QTimer(self)
        self.__busyTimer.timeout.connect(self.onTimeOut)

        # In case of attributes (Hash-V2) connect another function here
        self.signalConflictStateChanged.connect(
            manager.Manager().onConflictStateChanged)

        box.signalUpdateComponent.connect(self.onDisplayValueChanged)
        if box.hasValue():
            self.onDisplayValueChanged(box, box.value)
        box.configuration.configuration.state.signalUpdateComponent.connect(
            self.onStateChanged)


    def copy(self):
        copyComponent = EditableApplyLaterComponent(self.classAlias, **self.__initParams)
        return copyComponent


### getter and setter functions ###
    def _getWidgetCategory(self):
        return self.__editableWidget.category
    widgetCategory = property(fget=_getWidgetCategory)


    def _getWidgetFactory(self):
        return self.__editableWidget
    widgetFactory = property(fget=_getWidgetFactory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__compositeWidget
    widget = property(fget=_getWidget)


    @property
    def boxes(self):
        return self.__editableWidget.boxes


    def _getValue(self):
        return self.__editableWidget.value
    value = property(fget=_getValue)


    # States whether the first incoming displayValue overwrites editableValue
    def _isEditableValueInit(self):
        return self.__isEditableValueInit
    def _setEditableValueInit(self, editableValueInit):
        self.__isEditableValueInit = editableValueInit
    isEditableValueInit = property(fget=_isEditableValueInit, fset=_setEditableValueInit)


    def setEnabled(self, enable):
        self.widget.setEnabled(enable)


    def addParameters(self, **params):
        self.__editableWidget.addParameters(**params)


    def _applyEnabled(self):
        return self.__acReset.isEnabled()
    def _setApplyEnabled(self, enable):
        if self.__acReset.isEnabled() is enable:
            return

        self.__acApply.setIcon(icons.apply if enable else icons.applyGrey)
        self.__acReset.setEnabled(enable)

        if enable is False:
            self.hasConflict = False

        # Broadcast to ConfigurationPanel - treewidget
        self.signalApplyChanged.emit(self.boxes[0], enable)
    applyEnabled = property(fget=_applyEnabled, fset=_setApplyEnabled)


    def _hasConflict(self):
        return self.__hasConflict
    def _setHasConflict(self, hasConflict):
        # Set member variable
        self.__hasConflict = hasConflict

        # Change color, icon and menu
        if hasConflict is True:
            self.changeColor = QColor(204,240,255,128) # blue
            text = "Resolve conflict"
            self.__acApply.setIcon(icons.applyConflict)
            self.__acApply.setMenu(self.__mApply)
            self.__acReset.setEnabled(False)
        else:
            self.changeColor = QColor(255,255,255,128) # white
            text = "Apply local changes"
            self.__acApply.setIcon(icons.apply if self.applyEnabled
                                   else icons.applyGrey)
            self.__acApply.setMenu(None)
        self.__acApply.setStatusTip(text)
        self.__acApply.setToolTip(text)

        self.signalConflictStateChanged.emit(self.boxes[0].configuration.path,
                                             hasConflict)
    hasConflict = property(fget=_hasConflict, fset=_setHasConflict)


    def changeApplyToBusy(self, isApplyBusy, changeTimer=True):
        palette = self.widget.palette()

        if isApplyBusy is True:
            # Remotely changed
            if changeTimer is True:
                self.__busyTimer.start(5000)
                self.hasConflict = False
                self.applyEnabled = False
            else:
                self.hasConflict = True
                self.applyEnabled = True
        else:
            # no more changes
            self.hasConflict = False
            if changeTimer is True:
                self.__busyTimer.stop()
                self.applyEnabled = False
            else:
                self.applyEnabled = True


    def addKeyValue(self, key, value):
        self.__editableWidget.addKeyValue(key, value)


    def removeKey(self, key):
        pass


    def destroy(self):
        for key in self.__editableWidget.keys:
            manager.Manager().unregisterEditableComponent(key, self)


    def changeWidget(self, factory, alias):
        self.classAlias = alias

        oldWidget = self.__editableWidget.widget
        self.__editableWidget = factory.getClass(alias)(
            self.box, oldWidget.parent())
        self.__editableWidget.setReadOnly(False)
        self.__editableWidget.signalEditingFinished.connect(self.onEditingFinished)
        oldWidget.parent().layout().insertWidget(0, self.__editableWidget.widget)
        oldWidget.setParent(None)
        self.__editableWidget.widget.show()

        for c in {b.configuration for b in self.__editableWidget.boxes}:
            c.refresh()


    # Slot called when changes need to be sent to Manager
    def onApplyClicked(self):
        self.changeApplyToBusy(True)
        # TODO: KeWe function with key/value pair needed
        for box in self.__editableWidget.boxes:
            box.set(self.__editableWidget.value, None)
        manager.Manager().onDeviceInstanceValuesChanged(self.__editableWidget.boxes)
        self.applyEnabled = False


    def onApplyRemoteChanges(self, key):
        self.__editableWidget.valueChanged(key, self.__currentDisplayValue)
        self.applyEnabled = False


    def onTimeOut(self):
        MessageBox.showWarning("The attribute couldn't be set in the current state.")

        self.changeApplyToBusy(False)
        self.changeColor = QColor(255,218,153,128) # orange
        self.applyEnabled = True


    @pyqtSlot(str, object)
    def onDisplayValueChanged(self, key, value):
        if self.__isEditableValueInit:
            self.__editableWidget.valueChanged(key, value)
            self.__isEditableValueInit = False
        self.__currentDisplayValue = value

        EPSILON = 1e-4
        if (isinstance(value, (numbers.Complex, numpy.inexact))
                and not isinstance(value, numbers.Integral)):
            diff = abs(value - self.__editableWidget.value)
            isEqualEditable = diff < EPSILON
        elif isinstance(value, list):
            if len(value) != len(self.__editableWidget.value):
                isEqualEditable = False
            else:
                for i in xrange(len(value)):
                    if value[i] != self.__editableWidget.value[i]:
                        isEqualEditable = False
                        break
                isEqualEditable = True
        else:
            isEqualEditable = (str(value) == str(self.__editableWidget.value)) # string comparison, problems with float values...

        if isEqualEditable:
            self.changeApplyToBusy(False)
        else:
            self.changeApplyToBusy(True, False)


    def onStateChanged(self):
        allowed = self.boxes[0].isAllowed()
        self.__acApply.setEnabled(allowed)
        self.widget.setEnabled(allowed)


    # Triggered from self.__editableWidget when value was edited
    def onEditingFinished(self, key, value):
        if self.__currentDisplayValue is None:
            return

        # Update apply and reset buttons...
        self.applyEnabled = value != self.__currentDisplayValue


class ChoiceComponent(BaseComponent):
    def __init__(self, classAlias, box, parent, widgetFactory=None):
        super(ChoiceComponent, self).__init__(classAlias)

        self.__choiceWidget = EditableWidget.getClass(classAlias)(box, parent)
        self.widget.setEnabled(False)


    def copy(self):
        return ChoiceComponent(self.classAlias, **self.__initParams)


    def _getWidgetCategory(self):
        return self.__choiceWidget.category
    widgetCategory = property(fget=_getWidgetCategory)


    def _getWidgetFactory(self):
        return self.__choiceWidget
    widgetFactory = property(fget=_getWidgetFactory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__choiceWidget.widget
    widget = property(fget=_getWidget)


    @property
    def boxes(self):
        return self.__choiceWidget.boxes


    def _getValue(self):
        return self.__choiceWidget.value
    def _setValue(self, value):
        self.__choiceWidget.value = value
    value = property(fget=_getValue, fset=_setValue)


    def setEnabled(self, enable):
        # Is not processed due to self.widget should always stay disabled
        pass


    def addParameters(self, **params):
        self.__choiceWidget.addParameters(**params)


    # Triggered by DataNotifier signalAddKey
    def addKeyValue(self, key, value):
        pass


    # Triggered by DataNotifier signalRemoveKey
    def removeKey(self, key):
        pass


    @pyqtSlot(str, object)
    def onValueChanged(self, key, value, timestamp=None):
        self.__choiceWidget.valueChanged(key, value, timestamp)


    @pyqtSlot(str, object)
    def onDisplayValueChanged(self, key, value, timestamp=None):
        self.__choiceWidget.valueChanged(key, value, timestamp)

