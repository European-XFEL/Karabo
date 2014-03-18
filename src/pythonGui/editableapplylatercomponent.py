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
from PyQt4.QtGui import QAction, QColor, QHBoxLayout, QIcon, QLabel, QMenu, \
                        QToolButton, QWidget


class EditableApplyLaterComponent(BaseComponent):
    # signals
    signalConflictStateChanged = pyqtSignal(str, bool) # key, hasConflict
    signalApplyChanged = pyqtSignal(object, bool) # key, state of apply button


    def __init__(self, classAlias, box):
        super(EditableApplyLaterComponent, self).__init__(classAlias)

        self.__isEditableValueInit = True
        
        self.__currentDisplayValue = None
        
        self.__compositeWidget = QWidget()
        hLayout = QHBoxLayout(self.__compositeWidget)
        hLayout.setContentsMargins(0,0,0,0)

        self.__editableWidget = EditableWidget.get_class(classAlias)(box)
        self.__editableWidget.signalEditingFinished.connect(self.onEditingFinished)
        hLayout.addWidget(self.__editableWidget.widget)

        d = box.descriptor
        unitLabel = (getattr(d, "metricPrefixSymbol", "") +
                     getattr(d, "unitSymbol", ""))
        if unitLabel:
            hLayout.addWidget(QLabel(unitLabel))

        self.__hasConflict = False

        text = "Apply"
        self.__tbApply = QToolButton()
        self.__tbApply.setStatusTip(text)
        self.__tbApply.setToolTip(text)
        self.__tbApply.setIconSize(QSize(24,24))
        self.__tbApply.setIcon(QIcon(":apply"))
        self.__tbApply.setEnabled(False)
        # Use action for button to reuse
        self.__acApply = QAction(QIcon(":apply"), text, self)
        self.__acApply.setStatusTip(text)
        self.__acApply.setToolTip(text)
        self.__acApply.setEnabled(False)
        self.__acApply.triggered.connect(self.onApplyClicked)
        self.__tbApply.clicked.connect(self.__acApply.triggered)
        # Add to layout
        hLayout.addWidget(self.__tbApply)

        text = "Reset"
        self.__tbReset = QToolButton()
        self.__tbReset.setStatusTip(text)
        self.__tbReset.setToolTip(text)
        self.__tbReset.setIconSize(QSize(24,24))
        self.__tbReset.setIcon(QIcon(":no"))
        self.__tbReset.setEnabled(False)
        # Use action for button to reuse
        self.__acReset = QAction(QIcon(":no"), text, self)
        self.__acReset.setStatusTip(text)
        self.__acReset.setToolTip(text)
        self.__acReset.setEnabled(False)
        self.__acReset.triggered.connect(self.onApplyRemoteChanges)
        self.__tbReset.clicked.connect(self.__acReset.triggered)
        # Add to layout
        hLayout.addWidget(self.__tbReset)

        # Add menu to toolbutton
        text = "Apply local changes"
        self.__mApply = QMenu(self.__tbApply)
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
        
        self.__busyTimer = QTimer(self)
        self.__busyTimer.timeout.connect(self.onTimeOut)

        # In case of attributes (Hash-V2) connect another function here
        self.signalConflictStateChanged.connect(
            manager.Manager().onConflictStateChanged)

        box.addComponent(self)


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
        return self.__tbApply.isEnabled()
    def _setApplyEnabled(self, enable):
        if self.__tbApply.isEnabled() is enable:
            return
        
        self.__tbApply.setEnabled(enable)
        self.__tbReset.setEnabled(enable)
        
        self.__acApply.setEnabled(enable)
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
            icon = QIcon(":apply-conflict")
            self.__tbApply.setIcon(icon)
            self.__tbApply.setPopupMode(QToolButton.InstantPopup)
            self.__tbApply.setMenu(self.__mApply)
            
            self.__acApply.setIcon(icon)
            self.__acApply.setMenu(self.__mApply)
            
            self.__tbReset.setEnabled(False)
            self.__acReset.setEnabled(False)
        else:
            self.changeColor = QColor(255,255,255,128) # white
            text = "Apply local changes"
            icon = QIcon(":apply")
            self.__tbApply.setIcon(icon)
            self.__tbApply.setPopupMode(QToolButton.DelayedPopup)
            self.__tbApply.setMenu(None)
            
            self.__acApply.setIcon(icon)
            self.__acApply.setMenu(None)
        self.__tbApply.setStatusTip(text)
        self.__tbApply.setToolTip(text)

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
        self.__initParams['value'] = self.value

        oldWidget = self.__editableWidget.widget
        self.__editableWidget = factory.get_class(alias)(**self.__initParams)
        self.__editableWidget.widget.setWindowFlags(Qt.BypassGraphicsProxyWidget)
        self.__editableWidget.widget.setAttribute(Qt.WA_NoSystemBackground, True)
        self.__editableWidget.signalEditingFinished.connect(self.onEditingFinished)
        oldWidget.parent().layout().insertWidget(0, self.__editableWidget.widget)
        oldWidget.setParent(None)
        self.__editableWidget.widget.show()

        # Refresh new widget...
        for key in self.__editableWidget.keys:
            manager.Manager().onRefreshInstance(key)


### slots ###
    # Slot called when changes need to be sent to Manager
    def onApplyClicked(self):
        self.changeApplyToBusy(True)
        # TODO: KeWe function with key/value pair needed
        for box in self.__editableWidget.boxes:
            box.value = self.__editableWidget.value
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
    def onValueChanged(self, key, value, timestamp=None):
        self.__editableWidget.valueChanged(key, value, timestamp, True)


    @pyqtSlot(str, object)
    def onDisplayValueChanged(self, key, value):
        if self.__isEditableValueInit:
            self.__editableWidget.valueChanged(key, value)
            self.__isEditableValueInit = False
        self.__currentDisplayValue = value

        EPSILON = 1e-4
        if isinstance(value, float):
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

        if isEqualEditable is False:
            self.changeApplyToBusy(True, False)
        else:
            self.changeApplyToBusy(False)


    # Triggered from self.__editableWidget when value was edited
    def onEditingFinished(self, key, value):
        if self.__currentDisplayValue is None:
            return

        # Update apply and reset buttons...
        if value == self.__currentDisplayValue:
            self.applyEnabled = False
        else:
            self.applyEnabled = True

