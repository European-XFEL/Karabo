#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 27, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a class which inherits from BaseComponent
   and will edit values and apply them later to the system.
"""

__all__ = ["EditableApplyLaterComponent"]


from basecomponent import BaseComponent
import manager
from messagebox import MessageBox
from widget import EditableWidget

from PyQt4.QtCore import pyqtSignal, pyqtSlot, QSize, QTimer, Qt
from PyQt4.QtGui import QAction, QColor, QHBoxLayout, QLabel, QMenu, \
                        QToolButton, QWidget

import numpy
import numbers
import icons


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

        print classAlias
        self.__editableWidget = EditableWidget.get_class(classAlias)(
            box, self.__compositeWidget)
        self.__editableWidget.signalEditingFinished.connect(self.onEditingFinished)
        hLayout.addWidget(self.__editableWidget.widget)

        self.box = box
        d = box.descriptor
        unitLabel = (getattr(d, "metricPrefixSymbol", "") +
                     getattr(d, "unitSymbol", ""))
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

        box.addComponent(self)
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
        self.__editableWidget = factory.get_class(alias)(
            self.box, oldWidget.parent())
        self.__editableWidget.widget.setWindowFlags(Qt.BypassGraphicsProxyWidget)
        self.__editableWidget.widget.setAttribute(Qt.WA_NoSystemBackground, True)
        self.__editableWidget.signalEditingFinished.connect(self.onEditingFinished)
        oldWidget.parent().layout().insertWidget(0, self.__editableWidget.widget)
        oldWidget.setParent(None)
        self.__editableWidget.widget.show()

        for c in {b.configuration for b in self.__editableWidget.boxes}:
            c.refresh()


### slots ###
    # Slot called when changes need to be sent to Manager
    def onApplyClicked(self):
        self.changeApplyToBusy(True)
        # TODO: KeWe function with key/value pair needed
        for box in self.__editableWidget.boxes:
            box.set(self.__editableWidget.value, None)
        manager.Manager().onDeviceInstanceValuesChanged(
            self.__editableWidget.boxes)
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
        if value == self.__currentDisplayValue:
            self.applyEnabled = False
        else:
            self.applyEnabled = True

