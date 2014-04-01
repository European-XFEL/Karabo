#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 4, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import hashtypes
from enums import AccessMode, NavigationItemTypes
from registry import Monkey

from choicecomponent import ChoiceComponent
from editableapplylatercomponent import EditableApplyLaterComponent
from editablenoapplycomponent import EditableNoApplyComponent

from treewidgetitems.attributetreewidgetitem import AttributeTreeWidgetItem
from treewidgetitems.commandtreewidgetitem import CommandTreeWidgetItem
from treewidgetitems.imagetreewidgetitem import ImageTreeWidgetItem
from treewidgetitems.propertytreewidgetitem import PropertyTreeWidgetItem

from PyQt4.QtGui import QIcon


class Type(hashtypes.Type):
    __metaclass__ = Monkey

    @classmethod
    def populateItem(cls, item, attrs, classtype, treewidget):
        if "options" in attrs:
            item.classAlias = "Selection Field"
            item.enumeration = attrs["options"]
            item.setIcon(0, QIcon(":enum"))
        else:
            item.enumeration = None
        component = None
        item.editableComponent = None
        if classtype:
            if attrs['accessMode'] & (
                    AccessMode.INITONLY | AccessMode.RECONFIGURABLE):
                component = EditableNoApplyComponent
        else:
            if attrs['accessMode'] & AccessMode.RECONFIGURABLE:
                component = EditableApplyLaterComponent
        if component is not None:
            item.editableComponent = component(
                classAlias=item.classAlias, key=item.internalKey,
                value=item.defaultValue,
                metricPrefixSymbol=item.metricPrefixSymbol,
                enumeration=item.enumeration,
                unitSymbol=item.unitSymbol, valueType=item.valueType)
        if component is EditableApplyLaterComponent:
            item.editableComponent.signalApplyChanged.connect(
                treewidget.onApplyChanged)


class Char(hashtypes.Char):
    __metaclass__ = Monkey

    @classmethod
    def populateItem(cls, item, attrs, classtype, treewidget):
        item.classAlias = "Text Field"
        item.setIcon(0, QIcon(":string"))
        super(Char, cls).populateItem(item, attrs, classtype, treewidget)


class String(hashtypes.String):
    __metaclass__ = Monkey

    @classmethod
    def populateItem(cls, item, attrs, classtype, treewidget):
        item.classAlias = "Text Field"
        item.setIcon(0, QIcon(":string"))
        super(String, cls).populateItem(item, attrs, classtype, treewidget)

        ca = dict(directory='Directory', fileIn='File In',
                  fileOut='fileOut').get(attrs.get('displayType'))
        if ca is not None:
            item.classAlias = ca
            item.setIcon(0, QIcon(':path'))


class Integer(hashtypes.Integer):
    __metaclass__ = Monkey

    @classmethod
    def populateItem(self, item, attrs, classtype, treewidget):
        item.classAlias = 'Integer Field'
        item.setIcon(0, QIcon(":int"))
        super(Integer, self).populateItem(item, attrs, classtype, treewidget)


class Number(hashtypes.Number):
    __metaclass__ = Monkey

    @classmethod
    def populateItem(self, item, attrs, classtype, treewidget):
        item.classAlias = "Float Field"
        item.setIcon(0, QIcon(":float"))
        super(Number, self).populateItem(item, attrs, classtype, treewidget)


class Bool(hashtypes.Bool):
    __metaclass__ = Monkey

    @classmethod
    def populateItem(self, item, attrs, classtype, treewidget):
        item.classAlias = "Toggle Field"
        item.setIcon(0, QIcon(":boolean"))
        super(Bool, self).populateItem(item, attrs, classtype, treewidget)


class Vector(hashtypes.Vector):
    __metaclass__ = Monkey

    @classmethod
    def populateItem(self, item, attrs, classtype, treewidget):
        item.classAlias = 'Histogram'
        Type.populateItem(item, attrs, classtype, treewidget)


class SchemaReader(object):
    def setDeviceType(self, deviceType):
        self.deviceType = deviceType


    def readSchema(self, path, schema, treeWidget):
        self.treeWidget = treeWidget
        if schema is None:
            return
        for k in schema.hash:
            self.parse(path + '.' + k, schema.hash[k], schema.hash[k, ...])
        treeWidget.resizeColumnToContents(0)
        return True


    def parse(self, key, hash, attrs, parent=None):
        nodes = (self.parseLeaf, self.parseNode, self.parseChoiceOfNodes,
                 self.parseListOfNodes)
        item = nodes[attrs['nodeType']](key, hash, attrs, parent)
        ral = 0 if parent is None else parent.requiredAccessLevel
        item.requiredAccessLevel = max(attrs.get('requiredAccessLevel', 0), ral)
        return item


    def copyAttr(self, item, attrs, out, ain=None):
        if ain is None:
            ain = out
        if ain in attrs:
            setattr(item, out, attrs[ain])


    def _createPropertyItem(self, key, hash, attrs, parent):
        item = PropertyTreeWidgetItem(key, self.treeWidget, parent)
        item.displayText = attrs.get('displayedName', key.split('.')[-1])
        self.copyAttr(item, attrs, 'allowedStates')
        return item


    def _createCommandItem(self, key, hash, attrs, parent):
        item = CommandTreeWidgetItem(key.split('.')[-1], key,
                                     self.treeWidget, parent)
        item.classAlias = "Command"

        if self.deviceType == NavigationItemTypes.DEVICE:
            item.enabled = True

        item.displayText = attrs.get('displayedName', key.split('.')[-1])
        self.copyAttr(item, attrs, 'allowedStates')
        return item


    def _createImageItem(self, key, hash, attrs, parent):
        item = ImageTreeWidgetItem(key, self.treeWidget, parent)
        item.classAlias = "Image View"

        if self.deviceType == NavigationItemTypes.DEVICE:
            item.enabled = True

        self.copyAttr(item, attrs, 'displayText', 'displayedName')
        self.copyAttr(item, attrs, 'allowedStates')
        return item


    def parseLeaf(self, key, hash, attrs, parent):
        item = self._createPropertyItem(key, hash, attrs, parent)
        self._setAssignment(item, attrs)
        self.copyAttr(item, attrs, 'alias')
        self.copyAttr(item, attrs, 'tags')
        self.copyAttr(item, attrs, 'description')
        self.copyAttr(item, attrs, 'defaultValue')
        self.copyAttr(item, attrs, 'metricPrefixSymbol')
        self.copyAttr(item, attrs, 'unitSymbol')
        item.valueType = Type.fromname[attrs['valueType']]
        item.valueType.populateItem(
            item, attrs, self.deviceType == NavigationItemTypes.CLASS,
            self.treeWidget)
        keys = dict(warnLow='Warn Low', warnHigh='Warn High',
                    alarmLow='Alarm Low', alarmHigh='Alarm High')
        if self.deviceType == NavigationItemTypes.CLASS:
            accessMode = AccessMode.INITONLY
            keys.update(minInc='Minimum inclusive', maxInc='Maximum inclusive',
                        minExc='Minimum exclusive', maxExc='Maximum exclusive')
        else:
            accessMode = AccessMode.RECONFIGURABLE
        for k, v in keys.iteritems():
            if k in attrs:
                fullPath = key + "@" + k
                value = attrs[k]

                aitem = AttributeTreeWidgetItem(fullPath, self.treeWidget, item)
                aitem.setText(0, v)

                if item.classAlias == "Integer Field":
                    aitem.classAlias = "Integer Field"
                    aitem.setIcon(0, QIcon(":int-attribute"))
                else:
                    aitem.classAlias = "Float Field"
                    aitem.setIcon(0, QIcon(":float-attribute"))

                editableComponent = None
                if self.deviceType == NavigationItemTypes.CLASS:
                    editableComponent = EditableNoApplyComponent(
                        classAlias=aitem.classAlias, key=aitem.internalKey,
                        value=value, valueType=aitem.valueType)
                else:
                    aitem.displayComponent.onValueChanged(key, value)

                    if accessMode == AccessMode.RECONFIGURABLE:
                        editableComponent = EditableApplyLaterComponent(
                            classAlias=aitem.classAlias, key=aitem.internalKey,
                            value=value, valueType=aitem.valueType)
                        editableComponent.signalApplyChanged.connect(
                            self.treeWidget.onApplyChanged)
                aitem.editableComponent = editableComponent
        return item


    def _setAssignment(self, item, attrs):
        if attrs.get('assignment', None) == 1: # Mandatory
            f = item.font(0)
            f.setBold(True)
            item.setFont(0, f)


    def parseNode(self, key, hash, attrs, parent):
        if attrs.get('displayType') == "Image":
            item = self._createImageItem(key, hash, attrs, parent)
        elif attrs.get('displayType') == "Slot":
            return self._createCommandItem(key, hash, attrs, parent)
        else:
            item = self._createPropertyItem(key, hash, attrs, parent)

        self.copyAttr(item, attrs, 'alias')
        self.copyAttr(item, attrs, 'tags')
        self.copyAttr(item, attrs, 'description')

        for k in hash:
            self.parse(key + '.' + k, hash[k], hash[k, ...], item)
        return item


    def parseChoiceOfNodes(self, key, hash, attrs, parent):
        item = self._createPropertyItem(key, hash, attrs, parent)
        self._setAssignment(item, attrs)
        self.copyAttr(item, attrs, 'defaultValue')

        item.isChoiceElement = True
        item.classAlias = "Choice Element"

        item.editableComponent = None
        component = None

        if self.deviceType == NavigationItemTypes.CLASS:
            if attrs['accessMode'] & (
                    AccessMode.INITONLY | AccessMode.RECONFIGURABLE):
                component = EditableNoApplyComponent
        else:
            if False: #attrs['accessMode'] & AccessMode.RECONFIGURABLE:
                component = EditableApplyLaterComponent
            else:
                component = ChoiceComponent
        if component is not None:
            item.editableComponent = component(
                item.classAlias, key=item.internalKey, value=item.defaultValue)
        if component is EditableApplyLaterComponent:
            item.editableComponent.signalApplyChanged.connect(
                self.treeWidget.onApplyChanged)

        for i, k in enumerate(hash):
            childItem = self.parse(key + "." + k, hash[k], hash[k, ...], item)

            if item.defaultValue is not None:
                if k != item.defaultValue:
                    childItem.setHidden(True)
            else:
                if i > 0:
                    childItem.setHidden(True)

            if item.editableComponent is not None:
                item.editableComponent.addParameters(itemToBeAdded=childItem)
        # Set default value in choice combobox
        item.onSetToDefault()
        
        return item


    def parseListOfNodes(self, key, hash, attrs, parent):
        item = self._createPropertyItem(key, hash, attrs, parent)
        for k in hash:
            self.parse(key + "." + k, hash[k], hash[k, ...], item)
        return item
