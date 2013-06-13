#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 4, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a generic XSD format reader."""

__all__ = ["SchemaReader"]

from editableapplylatercomponent import EditableApplyLaterComponent
from editablenoapplycomponent import EditableNoApplyComponent

from enums import NavigationItemTypes

from treewidgetitems.attributetreewidgetitem import *
from treewidgetitems.commandtreewidgetitem import *
from treewidgetitems.imagetreewidgetitem import *
from treewidgetitems.propertytreewidgetitem import *

from schematest.sampleschema import SampleSchema 

from libkarathon import *

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class SchemaReader(object):

    def __init__(self):
        super(SchemaReader, self).__init__()
        
        self.__schema = None
        self.__treeWidget = None # Treewidget displays schema
        self.__deviceType = None # CLASS or DEVICE
        self.__rootPath = None # full internal key (e.g. "server.serverId.classes.classId")
        

    def setDeviceType(self, deviceType):
        self.__deviceType = deviceType


    def readSchema(self, path, schema, treeWidget):
        self.__schema = schema
        self.__treeWidget = treeWidget
        
        if self.__schema is None:
            #print "No schema valid schema was provided!"
            return
        
        # For testing (all elements)
        #sampleSchemaClass = SampleSchema.create("SampleSchema", Hash())
        #self.__schema = sampleSchemaClass.getSchema("SampleSchema")
        
        print ""
        print "readSchema"
        print self.__schema
        print ""
        
        self.__rootPath = path
        
        keys = self.__schema.getKeys()
        for key in keys:
            self.r_readSchema(key)
        
        self.__treeWidget.resizeColumnToContents(0)


    def r_readSchema(self, key, parentItem=None):
        
        item = None
        
        if self.__schema.isLeaf(key):
            #print "isLeaf", key
            item = self._createPropertyItem(key, parentItem)
            self._handleLeaf(key, item)
        elif self.__schema.isCommand(key):
            #print "isCommand", key
            item = self._createCommandItem(key, parentItem)
            #self._handleCommand(key, item)
        elif self.__schema.isNode(key):
            #print "isNode", key
            item = self._createPropertyItem(key, parentItem)
            self._handleNode(key, item)
        elif self.__schema.isChoiceOfNodes(key):
            #print "isChoiceOfNodes", key
            item = self._createPropertyItem(key, parentItem)
            self._handleChoiceOfNodes(key, item)
        elif self.__schema.isListOfNodes(key):
            print "isListOfNodes", key
            item = self._createPropertyItem(key, parentItem)
            self._handleListOfNodes(key, item)
        
        return item


    def _createPropertyItem(self, key, parentItem):
            fullPath = self.__rootPath + "." + key
            if parentItem:
                item = PropertyTreeWidgetItem(fullPath, self.__treeWidget, parentItem)
            else:
                item = PropertyTreeWidgetItem(fullPath, self.__treeWidget)

            if self.__schema.hasDisplayedName(key):
                item.displayText = self.__schema.getDisplayedName(key)
            else:
                displayName = key.split(".")
                item.displayText = displayName[len(displayName)-1]
            
            if self.__schema.hasExpertLevel(key):
                item.expertLevel = self.__schema.getExpertLevel(key)
            
            return item


    def _createCommandItem(self, key, parentItem):
            fullPath = self.__rootPath + "." + key
            if parentItem:
                item = CommandTreeWidgetItem(fullPath, self.__treeWidget, parentItem)
            else:
                item = CommandTreeWidgetItem(fullPath, self.__treeWidget)
            
            if self.__deviceType is NavigationItemTypes.DEVICE:
                item.enabled = True
            
            if self.__schema.hasDisplayedName(key):
                item.displayText = self.__schema.getDisplayedName(key)
            else:
                displayName = key.split(".")
                item.displayText = displayName[len(displayName)-1]
            
            if self.__schema.hasExpertLevel(key):
                item.expertLevel = self.__schema.getExpertLevel(key)
                
            return item


    def _handleLeaf(self, key, item):
        self._getAssignment(key, item)
        self._getAccessMode(key, item)
        
        description = self._getDescription(key, item)
        defaultValue = self._getDefaultValue(key, item)
        unitSymbol = self._getUnit(key, item)
        
        expertLevel = self._getExpertLevel(key, item)
        allowedStates = self._getAllowedStates(key, item)
        
        #if self.__schema.hasDisplayType(key):
        #    print "type", self.__schema.getDisplayType(key)

        minInc = self._getMinInc(key, item)
        maxInc = self._getMaxInc(key, item)
        minExc = self._getMinExc(key, item)
        maxExc = self._getMaxExc(key, item)
        
        self._getValueType(key, item, defaultValue, unitSymbol)


    def _handleNode(self, key, parentItem):
        self._getDescription(key, parentItem)
        nodeKeys = self.__schema.getKeys(key)
        for nKey in nodeKeys:
            self.r_readSchema(key + "." + nKey, parentItem)


    def _handleChoiceOfNodes(self, key, parentItem):
        defaultValue = self._getDefaultValue(key, parentItem)
        
        parentItem.isChoiceElement = True
        parentItem.classAlias = "Choice Element"
        # Choiceelements can not have strings as arguments
        #parentItem.defaultValue = Hash(str(parentItem.defaultValue))
        
        if self.__deviceType is NavigationItemTypes.CLASS:
            choiceComponent = EditableNoApplyComponent(parentItem.classAlias, key=key, value=defaultValue)
        #else:
            #if parentItem.accessType == AccessTypes.READONLY or parentItem.accessType == AccessTypes.INIT:
            #    choiceComponent = ChoiceComponent(parentItem.classAlias, key=parentItem.key, value=None)
            #else:
            #    choiceComponent = EditableApplyLaterComponent(parentItem.classAlias, key=key, value=None)
            #    choiceComponent.signalApplyChanged.connect(twAttributeEditorPage.onApplyChanged)

        parentItem.editableComponent = choiceComponent        

        choiceKeys = self.__schema.getKeys(key)
        for cKey in choiceKeys:
            childItem = self.r_readSchema(key + "." + cKey, parentItem)
            if cKey != defaultValue:
                childItem.setHidden(True)
            choiceComponent.addParameters(itemToBeAdded=childItem)


    def _handleListOfNodes(self, key, parentItem):
        listKeys = self.__schema.getKeys(key)
        for lKey in listKeys:
            self.r_readSchema(key + "." + lKey, parentItem)


### Schema getter functions ###
    def _getValueType(self, key, item, defaultValue, unitSymbol):
        valueType = self.__schema.getValueType(key)
        
        if valueType == Types.STRING:
            #print "STRING"
            self._handleString(key, item, defaultValue, unitSymbol)
        elif valueType == Types.CHAR:
            #print "CHAR"
            self._handleString(key, item, defaultValue, unitSymbol)
        elif valueType == Types.BOOL:
            #print "BOOL"
            self._handleBool(key, item, defaultValue, unitSymbol)
        elif valueType == Types.FLOAT:
            #print "FLOAT"
            self._handleFloat(key, item, defaultValue, unitSymbol)
        elif valueType == Types.COMPLEX_FLOAT:
            #print "COMPLEX_FLOAT"
            self._handleFloat(key, item, defaultValue, unitSymbol)
        elif valueType == Types.DOUBLE:
            #print "DOUBLE"
            self._handleFloat(key, item, defaultValue, unitSymbol)
        elif valueType == Types.COMPLEX_DOUBLE:
            #print "COMPLEX_DOUBLE"
            self._handleFloat(key, item, defaultValue, unitSymbol)
        elif valueType == Types.UINT8:
            #print "UINT8"
            self._handleInteger(key, item, defaultValue, unitSymbol)
        elif valueType == Types.INT16:
            #print "INT16"
            self._handleInteger(key, item, defaultValue, unitSymbol)
        elif valueType == Types.UINT16:
            #print "UINT16"
            self._handleInteger(key, item, defaultValue, unitSymbol)
        elif valueType == Types.INT32:
            #print "INT32"
            self._handleInteger(key, item, defaultValue, unitSymbol)
        elif valueType == Types.UINT32:
            #print "UINT32"
            self._handleInteger(key, item, defaultValue, unitSymbol)
        elif valueType == Types.INT64:
            #print "INT64"
            self._handleInteger(key, item, defaultValue, unitSymbol)
        elif valueType == Types.UINT64:
            #print "UINT64"
            self._handleInteger(key, item, defaultValue, unitSymbol)
        elif valueType == Types.VECTOR_STRING:
            print "VECTOR_STRING"
            self._handleVectorString(key, item, defaultValue, unitSymbol)
        elif valueType == Types.VECTOR_CHAR:
            print "VECTOR_CHAR"
        elif valueType == Types.VECTOR_INT8:
            print "VECTOR_INT8"
        elif valueType == Types.VECTOR_UINT8:
            print "VECTOR_UINT8"
        elif valueType == Types.VECTOR_INT16:
            print "VECTOR_INT16"
        elif valueType == Types.VECTOR_UINT16:
            print "VECTOR_UINT16"
        elif valueType == Types.VECTOR_INT32:
            print "VECTOR_INT32"
        elif valueType == Types.VECTOR_UINT32:
            print "VECTOR_UINT32"
        elif valueType == Types.VECTOR_INT64:
            print "VECTOR_INT64"
        elif valueType == Types.VECTOR_UINT64:
            print "VECTOR_UINT64"
        elif valueType == Types.VECTOR_FLOAT:
            print "VECTOR_FLOAT"
        elif valueType == Types.VECTOR_DOUBLE:
            print "VECTOR_DOUBLE"
        elif valueType == Types.VECTOR_COMPLEX_FLOAT:
            print "VECTOR_COMPLEX_FLOAT"
        elif valueType == Types.VECTOR_COMPLEX_DOUBLE:
            print "VECTOR_COMPLEX_DOUBLE"
        elif valueType == Types.VECTOR_BOOL:
            print "VECTOR_BOOL"
        elif valueType == Types.VECTOR_HASH:
            print "VECTOR_HASH"
        elif valueType == Types.HASH:
            print "HASH"
        elif valueType == Types.NDARRAY_BOOL:
            print "NDARRAY_BOOL"
        elif valueType == Types.NDARRAY_INT16:
            print "NDARRAY_INT16"
        elif valueType == Types.NDARRAY_UINT16:
            print "NDARRAY_UINT16"
        elif valueType == Types.NDARRAY_INT32:
            print "NDARRAY_INT32"
        elif valueType == Types.NDARRAY_UINT32:
            print "NDARRAY_UINT32"
        elif valueType == Types.NDARRAY_INT64:
            print "NDARRAY_INT64"
        elif valueType == Types.NDARRAY_UINT64:
            print "NDARRAY_UINT64"
        elif valueType == Types.NDARRAY_FLOAT:
            print "NDARRAY_FLOAT"
        elif valueType == Types.NDARRAY_DOUBLE:
            print "NDARRAY_DOUBLE"
        elif valueType == Types.NDARRAY_COMPLEX_FLOAT:
            print "NDARRAY_COMPLEX_FLOAT"
        elif valueType == Types.NDARRAY_COMPLEX_DOUBLE:
            print "NDARRAY_COMPLEX_DOUBLE"


    def _getDescription(self, key, parentItem):
        if not self.__schema.hasDescription(key):
            return None
        
        description = self.__schema.getDescription(key)
        
        text = QString("<b>Description: </b>%1").arg(description)
        toolTip = parentItem.toolTip(0)
        if toolTip.isEmpty():
            parentItem.setToolTip(0, text)
        else:
            parentItem.setToolTip(0, toolTip + "<br>" + text)
        
        #fullPath = self.__rootPath + "." + key
        #cItem = AttributeTreeWidgetItem(fullPath, self.__treeWidget, parentItem)
        #cItem.setText(0, "Description")
        #cItem.setText(2, description)
        
        return None


    def _getDefaultValue(self, key, parentItem):
        if not self.__schema.hasDefaultValue(key):
            return None

        defaultValue = self.__schema.getDefaultValue(key)
        
        if isinstance(defaultValue, list):
            defaultValueStr = str()
            for value in defaultValue:
                defaultValueStr += value + ","
            text = QString("<b>Default value: </b>%1").arg(defaultValueStr)
        else:
            text = QString("<b>Default value: </b>%1").arg(defaultValue)
        
        toolTip = parentItem.toolTip(0)
        if toolTip.isEmpty():
            parentItem.setToolTip(0, text)
        else:
            parentItem.setToolTip(0, toolTip + "<br>" + text)
        
        return defaultValue


    def _getUnit(self, key, item):
        if not self.__schema.hasUnit(key):
            return None
        
        unit = self.__schema.getUnit(key)
        unitName = self.__schema.getUnitName(key)
        unitSymbol = self.__schema.getUnitSymbol(key)
        
        #item.setText(2, unitSymbol)
        return unitSymbol


    def _getAssignment(self, key, item):
        if not self.__schema.hasAssignment(key):
            return
        
        if self.__schema.isAssignmentMandatory(key):
            f = item.font(0)
            f.setBold(True)
            item.setFont(0, f)
        elif self.__schema.isAssignmentInternal(key):
            #print "assignmentInternal"
            pass
        elif self.__schema.isAssignmentOptional(key):
            #print "assignmentOptional"
            pass


    def _getAccessMode(self, key, parentItem):
        if not self.__schema.hasAccessMode(key):
            return
        
        if self.__schema.isAccessInitOnly(key):
            #print "isInitOnly"
            pass
        elif self.__schema.isAccessReconfigurable(key):
            #print "isReconfigurable"
            pass
        elif self.__schema.isAccessReadOnly(key):
            #print "isReadOnly"
            if self.__schema.hasWarnLow(key):
                self._handleFloatAttribute(parentItem, key + ".warnLow",
                                           "Warn low", self.__schema.getWarnLow(key))
            if self.__schema.hasWarnHigh(key):
                self._handleFloatAttribute(parentItem, key + ".warnHigh",
                                           "Warn high", self.__schema.getWarnHigh(key))
            if self.__schema.hasAlarmLow(key):
                self._handleFloatAttribute(parentItem, key + ".alarmLow",
                                           "Alarm low", self.__schema.getAlarmLow(key))
            if self.__schema.hasAlarmHigh(key):
                self._handleFloatAttribute(parentItem, key + ".alarmHigh",
                                           "Alarm high", self.__schema.getAlarmHigh(key))
        

    def _getExpertLevel(self, key, item):
        if not self.__schema.hasExpertLevel(key):
            return None
        
        return self.__schema.getExpertLevel(key)


    def _getAllowedStates(self, key, item):
        if not self.__schema.hasAllowedStates(key):
            return None
        
        return self.__schema.getAllowedStates(key)


    def _getMinInc(self, key, item):
        if not self.__schema.hasMinInc(key):
            return None
        
        return self.__schema.getMinInc(key)
        

    def _getMaxInc(self, key, item):
        if not self.__schema.hasMaxInc(key):
            return None
        
        return self.__schema.getMaxInc(key)
        

    def _getMinExc(self, key, item):
        if not self.__schema.hasMinExc(key):
            return None
        
        return self.__schema.getMinExc(key)
        

    def _getMaxExc(self, key, item):
        if not self.__schema.hasMaxExc(key):
            return None
        
        return self.__schema.getMaxExc(key)


### functions for setting editable components depending on value type ###
    def _handleBool(self, key, item, defaultValue, unitSymbol):
        item.classAlias = "Toggle Field"
        if self.__deviceType is NavigationItemTypes.CLASS:
            editableComponent = EditableNoApplyComponent(classAlias=item.classAlias, key=item.internalKey,
                                                         value=defaultValue, unitSymbol=unitSymbol)
            #, value=item.defaultValue, valueType=item.valueType, unitSymbol=unitSymbol)
        else:
            editableComponent = EditableApplyLaterComponent(classAlias=item.classAlias, key=item.internalKey,
                                                            unitSymbol=unitSymbol)
            #, value=None, valueType=item.valueType, unitSymbol=unitSymbol)
            editableComponent.signalApplyChanged.connect(self.__treeWidget.onApplyChanged)

        item.setIcon(0, QIcon(":boolean"))
        item.editableComponent = editableComponent


    def _handleString(self, key, item, defaultValue, unitSymbol):
        hasOptions = self.__schema.hasOptions(key)
        if hasOptions:
            item.classAlias = "Selection Field"
            enumeration = self.__schema.getOptions(key)
            item.setIcon(0, QIcon(":enum"))
        else:
            item.classAlias = "Text Field"
            enumeration = None
            item.setIcon(0, QIcon(":string"))
        
        # TODO: do not forget PATH_ELEMENT "File Path"
        if self.__deviceType is NavigationItemTypes.CLASS:
            editableComponent = EditableNoApplyComponent(classAlias=item.classAlias, key=item.internalKey,
                                                         value=defaultValue, enumeration=enumeration,
                                                         unitSymbol=unitSymbol)
            #value=item.defaultValue, valueType=item.valueType, unitSymbol=unitSymbol)
        else:
            editableComponent = EditableApplyLaterComponent(classAlias=item.classAlias, key=item.internalKey,
                                                            enumeration=enumeration, unitSymbol=unitSymbol)
            #value=None, valueType=item.valueType, unitSymbol=unitSymbol)
            editableComponent.signalApplyChanged.connect(self.__treeWidget.onApplyChanged)

        #item.setIcon(0, QIcon(":path"))
        item.editableComponent = editableComponent


    def _handleInteger(self, key, item, defaultValue, unitSymbol):
        hasOptions = self.__schema.hasOptions(key)
        if hasOptions:
            item.classAlias = "Selection Field"
            enumeration = self.__schema.getOptions(key)
            item.setIcon(0, QIcon(":enum"))
        else:
            item.classAlias = "Integer Field"
            enumeration = None
            item.setIcon(0, QIcon(":int"))
            
        if self.__deviceType is NavigationItemTypes.CLASS:
            editableComponent = EditableNoApplyComponent(classAlias=item.classAlias, key=item.internalKey,
                                                         value=defaultValue, enumeration=enumeration,
                                                         unitSymbol=unitSymbol)
            #, value=item.defaultValue, valueType=item.valueType, unitSymbol=unitSymbol)
        else:
            editableComponent = EditableApplyLaterComponent(classAlias=item.classAlias, key=item.internalKey,
                                                            enumeration=enumeration, unitSymbol=unitSymbol)
            #, value=None, valueType=item.valueType, unitSymbol=unitSymbol)
            editableComponent.signalApplyChanged.connect(self.__treeWidget.onApplyChanged)

        #if minInclusive and len(minInclusive) > 0:
        #    editableComponent.addParameters(minimum=int(minInclusive))
        #if maxInclusive and len(maxInclusive) > 0:
        #    editableComponent.addParameters(maximum=int(maxInclusive))
        item.editableComponent = editableComponent


    def _handleFloat(self, key, item, defaultValue, unitSymbol):
        hasOptions = self.__schema.hasOptions(key)
        if hasOptions:
            item.classAlias = "Selection Field"
            enumeration = self.__schema.getOptions(key)
            item.setIcon(0, QIcon(":enum"))
        else:
            item.classAlias = "Float Field"
            enumeration = None
            item.setIcon(0, QIcon(":float"))
        
        if self.__deviceType is NavigationItemTypes.CLASS:
            editableComponent = EditableNoApplyComponent(classAlias=item.classAlias, key=item.internalKey,
                                                         value=defaultValue, enumeration=enumeration,
                                                         unitSymbol=unitSymbol)
            #value=attributeItem.defaultValue, valueType=attributeItem.valueType, unitSymbol=unitSymbol)
        else:
            editableComponent = EditableApplyLaterComponent(classAlias=item.classAlias, key=item.internalKey,
                                                            enumeration=enumeration, unitSymbol=unitSymbol)
            #value=None, valueType=attributeItem.valueType, unitSymbol=unitSymbol)
            editableComponent.signalApplyChanged.connect(self.__treeWidget.onApplyChanged)

        #if minInclusive and len(minInclusive) > 0:
        #    editableComponent.addParameters(minimum=float(minInclusive))
        #if maxInclusive and len(maxInclusive) > 0:
        #    editableComponent.addParameters(maximum=float(maxInclusive))
        item.editableComponent = editableComponent


    def _handleFloatAttribute(self, parentItem, key, text, value):
        # TODO: needs to have unique key
        fullPath = self.__rootPath + "." + key
        item = AttributeTreeWidgetItem(fullPath, self.__treeWidget, parentItem)
        item.setText(0, text)
        
        #print "_handleFloatAttribute", key, text, value
        
        item.classAlias = "Float Field"
        item.setIcon(0, QIcon(":float"))
        
        if self.__deviceType is NavigationItemTypes.CLASS:
            editableComponent = EditableNoApplyComponent(classAlias=item.classAlias, key=item.internalKey, value=value)
            #value=attributeItem.defaultValue, valueType=attributeItem.valueType, unitSymbol=unitSymbol)
        else:
            editableComponent = EditableApplyLaterComponent(classAlias=item.classAlias, key=item.internalKey, value=value)
            #value=None, valueType=attributeItem.valueType, unitSymbol=unitSymbol)
            editableComponent.signalApplyChanged.connect(self.__treeWidget.onApplyChanged)

        item.editableComponent = editableComponent


    def _handleVectorString(self, key, item, defaultValue, unitSymbol):

        item.classAlias = "Histogram"
        item.setIcon(0, QIcon(":enum"))

        defaultVec = []
        if defaultValue:
             defaultVec = str(defaultValue).split(',')
        default = []
        for index in defaultVec:
            default.append(str(index))

        if self.__deviceType is NavigationItemTypes.CLASS:
            item.editableComponent = EditableNoApplyComponent(classAlias=item.classAlias, key=item.internalKey, value=default, unitSymbol=unitSymbol)
        else:
            editableComponent = EditableApplyLaterComponent(classAlias=item.classAlias, key=item.internalKey, value=None, unitSymbol=unitSymbol)
            editableComponent.signalApplyChanged.connect(self.__treeWidget.onApplyChanged)
            item.editableComponent = editableComponent

