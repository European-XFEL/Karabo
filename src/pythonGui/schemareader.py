#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 4, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a generic XSD format reader."""

__all__ = ["SchemaReader"]

from choicecomponent import ChoiceComponent
from editableapplylatercomponent import EditableApplyLaterComponent
from editablenoapplycomponent import EditableNoApplyComponent
from editablepathapplylatercomponent import EditablePathApplyLaterComponent
from editablepathnoapplycomponent import EditablePathNoApplyComponent

from enums import *

from treewidgetitems.attributetreewidgetitem import *
from treewidgetitems.commandtreewidgetitem import *
from treewidgetitems.imagetreewidgetitem import *
from treewidgetitems.propertytreewidgetitem import *

from schematest.sampleschema import SampleSchema 

from karabo.karathon import *

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
            return False
        
        # For testing (all elements)
        #sampleSchemaClass = SampleSchema.create("SampleSchema", Hash())
        #self.__schema = sampleSchemaClass.getSchema("SampleSchema")
        
        #print ""
        #print "++++ readSchema ++++"
        #print self.__schema
        #print ""
        
        self.__rootPath = path + ".configuration"
        
        keys = self.__schema.getKeys()
        for key in keys:
            self.r_readSchema(key)
        
        self.__treeWidget.resizeColumnToContents(0)
        return True


    def r_readSchema(self, key, parentItem=None):
        
        item = None
        
        if self.__schema.isLeaf(key):
            #print "isLeaf", key
            item = self._createPropertyItem(key, parentItem)
            self._handleLeaf(key, item)
        elif self.__schema.isCommand(key):
            #print "isCommand", key
            item = self._createCommandItem(key, parentItem)
        elif self.__schema.isNode(key):
            #print "isNode", key
            if (self.__schema.hasDisplayType(key) and (self.__schema.getDisplayType(key) == "Image")):
                item = self._createImageItem(key, parentItem)              
            else:
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
            
            self._setRequiredAccessLevel(key, item)
            self._setAllowedStates(key, item)
            
            return item


    def _createCommandItem(self, key, parentItem):
            fullPath = self.__rootPath + "." + key
            if parentItem:
                item = CommandTreeWidgetItem(key, fullPath, self.__treeWidget, parentItem)
            else:
                item = CommandTreeWidgetItem(key, fullPath, self.__treeWidget)
            
            item.classAlias = "Command"
        
            if self.__deviceType is NavigationItemTypes.DEVICE:
                item.enabled = True
            
            if self.__schema.hasDisplayedName(key):
                item.displayText = self.__schema.getDisplayedName(key)
            else:
                displayName = key.split(".")
                item.displayText = displayName[len(displayName)-1]
            
            self._setRequiredAccessLevel(key, item)
            self._setAllowedStates(key, item)
            
            return item


    def _createImageItem(self, key, parentItem):
        fullPath = self.__rootPath + "." + key
        if parentItem:
            item = ImageTreeWidgetItem(fullPath, self.__treeWidget, parentItem)
        else:
            item = ImageTreeWidgetItem(fullPath, self.__treeWidget)
        
        item.classAlias = "Image View"
        
        if self.__deviceType is NavigationItemTypes.DEVICE:
                item.enabled = True
                
        if self.__schema.hasDisplayedName(key):
                item.displayText = self.__schema.getDisplayedName(key)
                
        self._setRequiredAccessLevel(key, item)
        self._setAllowedStates(key, item)


    def _handleLeaf(self, key, item):
        #print "_handleLeaf"
        self._setAssignment(key, item)
        
        self._setAlias(key, item)
        self._setTags(key, item)
        self._setDescription(key, item)
        self._setDefaultValue(key, item)
        self._setMetricPrefixSymbol(key, item)
        self._setUnitSymbol(key, item)
        
        self._handleValueType(key, item)
        self._checkForFurtherAttributes(key, item)


    def _handleNode(self, key, item):
        self._setAlias(key, item)
        self._setTags(key, item)
        self._setDescription(key, item)
        
        nodeKeys = self.__schema.getKeys(key)
        for nKey in nodeKeys:
            self.r_readSchema(key + "." + nKey, item)


    def _handleChoiceOfNodes(self, key, parentItem):
        #print "_handleChoiceOfNodes"
        self._setDefaultValue(key, parentItem)
        
        parentItem.isChoiceElement = True
        parentItem.classAlias = "Choice Element"
        # Choiceelements can not have strings as arguments
        parentItem.defaultValue = Hash(str(parentItem.defaultValue))
        
        accessMode = self._getAccessMode(key)
        choiceComponent = None
        
        if self.__deviceType is NavigationItemTypes.CLASS:
            if (accessMode is AccessMode.INITONLY) or (accessMode is AccessMode.RECONFIGURABLE) or (accessMode is AccessMode.UNDEFINED):
                choiceComponent = EditableNoApplyComponent(parentItem.classAlias, key=parentItem.internalKey, value=parentItem.defaultValue)
        else:
            if accessMode is AccessMode.RECONFIGURABLE:
                choiceComponent = EditableApplyLaterComponent(parentItem.classAlias, key=parentItem.internalKey, value=None)
                choiceComponent.signalApplyChanged.connect(self.__treeWidget.onApplyChanged)
            else:
                choiceComponent = ChoiceComponent(parentItem.classAlias, key=parentItem.internalKey, value=None)

        parentItem.editableComponent = choiceComponent        

        choiceKeys = self.__schema.getKeys(key)
        for cKey in choiceKeys:
            childItem = self.r_readSchema(key + "." + cKey, parentItem)
            if cKey != parentItem.defaultValue:
                childItem.setHidden(True)
            if choiceComponent:
                choiceComponent.addParameters(itemToBeAdded=childItem)


    def _handleListOfNodes(self, key, item):
        listKeys = self.__schema.getKeys(key)
        for lKey in listKeys:
            self.r_readSchema(key + "." + lKey, item)


### Schema getter functions ###
    def _handleValueType(self, key, item):
        valueType = self.__schema.getValueType(key)
        item.valueType = valueType
        
        if valueType == Types.STRING:
            #print "STRING"
            self._handleString(key, item)
        elif valueType == Types.CHAR:
            #print "CHAR"
            self._handleString(key, item)
        elif valueType == Types.BOOL:
            #print "BOOL"
            self._handleBool(key, item)
        elif valueType == Types.FLOAT:
            #print "FLOAT"
            self._handleFloat(key, item)
        elif valueType == Types.COMPLEX_FLOAT:
            #print "COMPLEX_FLOAT"
            self._handleFloat(key, item)
        elif valueType == Types.DOUBLE:
            #print "DOUBLE"
            self._handleFloat(key, item)
        elif valueType == Types.COMPLEX_DOUBLE:
            #print "COMPLEX_DOUBLE"
            self._handleFloat(key, item)
        elif valueType == Types.UINT8:
            #print "UINT8"
            self._handleInteger(key, item)
        elif valueType == Types.INT16:
            #print "INT16"
            self._handleInteger(key, item)
        elif valueType == Types.UINT16:
            #print "UINT16"
            self._handleInteger(key, item)
        elif valueType == Types.INT32:
            #print "INT32"
            self._handleInteger(key, item)
        elif valueType == Types.UINT32:
            #print "UINT32"
            self._handleInteger(key, item)
        elif valueType == Types.INT64:
            #print "INT64"
            self._handleInteger(key, item)
        elif valueType == Types.UINT64:
            #print "UINT64"
            self._handleInteger(key, item)
        elif valueType == Types.VECTOR_STRING:
            print "VECTOR_STRING"
            self._handleVector(key, item)
        elif valueType == Types.VECTOR_CHAR:
            print "VECTOR_CHAR"
            self._handleVector(key, item)
        elif valueType == Types.VECTOR_INT8:
            print "VECTOR_INT8"
            self._handleVector(key, item)
        elif valueType == Types.VECTOR_UINT8:
            print "VECTOR_UINT8"
            self._handleVector(key, item)
        elif valueType == Types.VECTOR_INT16:
            print "VECTOR_INT16"
            self._handleVector(key, item)
        elif valueType == Types.VECTOR_UINT16:
            print "VECTOR_UINT16"
            self._handleVector(key, item)
        elif valueType == Types.VECTOR_INT32:
            print "VECTOR_INT32"
            self._handleVector(key, item)
        elif valueType == Types.VECTOR_UINT32:
            print "VECTOR_UINT32"
            self._handleVector(key, item)
        elif valueType == Types.VECTOR_INT64:
            print "VECTOR_INT64"
            self._handleVector(key, item)
        elif valueType == Types.VECTOR_UINT64:
            print "VECTOR_UINT64"
            self._handleVector(key, item)
        elif valueType == Types.VECTOR_FLOAT:
            print "VECTOR_FLOAT"
            self._handleVector(key, item)
        elif valueType == Types.VECTOR_DOUBLE:
            print "VECTOR_DOUBLE"
            self._handleVector(key, item)
        elif valueType == Types.VECTOR_COMPLEX_FLOAT:
            print "VECTOR_COMPLEX_FLOAT"
            self._handleVector(key, item)
        elif valueType == Types.VECTOR_COMPLEX_DOUBLE:
            print "VECTOR_COMPLEX_DOUBLE"
            self._handleVector(key, item)
        elif valueType == Types.VECTOR_BOOL:
            print "VECTOR_BOOL"
            self._handleVector(key, item)
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


    def _setAlias(self, key, item):
        if not self.__schema.keyHasAlias(key):
            return
        
        item.alias = self.__schema.getAliasFromKey(key)


    def _setTags(self, key, item):
        if not self.__schema.hasTags(key):
            return
        
        item.tags = self.__schema.getTags(key)


    def _setDescription(self, key, item):
        if not self.__schema.hasDescription(key):
            return
        
        item.description = self.__schema.getDescription(key)


    def _setDefaultValue(self, key, item):
        if not self.__schema.hasDefaultValue(key):
            return None

        item.defaultValue = self.__schema.getDefaultValue(key)


    #def _setTimestamp(self, key, item):
    #    if not self.__schema.hasTimestamp(key):
    #        return
        
    #    item.timestamp = self.__schema.getTimestamp(key)


    def _setMetricPrefixSymbol(self, key, item):
        if not self.__schema.hasMetricPrefix(key):
            return None
        
        #metricPrefix = self.__schema.getMetricPrefix(key)
        #metricPrefixName = self.__schema.getMetricPrefixName(key)
        item.metricPrefixSymbol = self.__schema.getMetricPrefixSymbol(key)


    def _setUnitSymbol(self, key, item):
        if not self.__schema.hasUnit(key):
            return None
        
        #unit = self.__schema.getUnit(key)
        #unitName = self.__schema.getUnitName(key)
        item.unitSymbol = self.__schema.getUnitSymbol(key)


    def _setAssignment(self, key, item):
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


    def _getAccessMode(self, key):
        if not self.__schema.hasAccessMode(key):
            #print "AccessMode.UNDEFINEhasWarnLowD"
            return AccessMode.UNDEFINED
        
        if self.__schema.isAccessInitOnly(key):
            #print "AccessMode.INITONLY"
            return AccessMode.INITONLY
        elif self.__schema.isAccessReconfigurable(key):
            #print "AccessMode.RECONFIGURABLE"
            return AccessMode.RECONFIGURABLE
        elif self.__schema.isAccessReadOnly(key):
            #print "AccessMode.READONLY"
            return AccessMode.READONLY
        
        return AccessMode.UNDEFINED


    def _checkForFurtherAttributes(self, key, parentItem):
        
        if self.__deviceType is NavigationItemTypes.CLASS:
            accessMode = AccessMode.INITONLY
        else:
            accessMode = AccessMode.RECONFIGURABLE
        
        # INITONLY + RECONFIGURABLE for warnLow/High, alarmLow/High, requiredAccessLevel
        if self.__schema.hasWarnLow(key):
            fullPath = key + "@warnLow"
            displayName = "Warn low"
            value = self.__schema.getWarnLow(key)
            
            if parentItem.classAlias == "Integer Field":
                self._handleIntegerAttribute(parentItem, fullPath,
                                             displayName, value,
                                             accessMode)
            else:
                self._handleFloatAttribute(parentItem, fullPath,
                                           displayName, value,
                                           accessMode)
        
        if self.__schema.hasWarnHigh(key):
            fullPath = key + "@warnHigh"
            displayName = "Warn high"
            value = self.__schema.getWarnHigh(key)
            
            if parentItem.classAlias == "Integer Field":
                self._handleIntegerAttribute(parentItem, fullPath,
                                             displayName, value,
                                             accessMode)
            else:
                self._handleFloatAttribute(parentItem, fullPath,
                                           displayName, value,
                                           accessMode)
        
        if self.__schema.hasAlarmLow(key):
            fullPath = key + "@alarmLow"
            displayName = "Alarm low"
            value = self.__schema.getAlarmLow(key)
            
            if parentItem.classAlias == "Integer Field":
                self._handleIntegerAttribute(parentItem, fullPath,
                                             displayName, value,
                                             accessMode)
            else:
                self._handleFloatAttribute(parentItem, fullPath,
                                           displayName, value,
                                           accessMode)
        
        if self.__schema.hasAlarmHigh(key):
            fullPath = key + "@alarmHigh"
            displayName = "Alarm high"
            value = self.__schema.getAlarmHigh(key)
            
            if parentItem.classAlias == "Integer Field":
                self._handleIntegerAttribute(parentItem, fullPath,
                                             displayName, value,
                                             accessMode)
            else:
                self._handleFloatAttribute(parentItem, fullPath,
                                           displayName, value,
                                           accessMode)

        # TODO: requiredAccesLevel INIT/RECONFIGURABLE
        #if self.__schema.hasRequiredAccessLevel(key):
        #    value = self.__schema.getRequiredAccessLevel(key)

        # INITONLY for min/maxInc and min/maxExc and archivePolicy
        if accessMode is AccessMode.INITONLY:
            if self.__schema.hasMinInc(key):
                fullPath = key + "@minInc"
                displayName = "Minimum inclusive"
                value = self.__schema.getMinInc(key)

                if parentItem.classAlias == "Integer Field":
                    self._handleIntegerAttribute(parentItem, fullPath,
                                                 displayName, value,
                                                 accessMode)
                else:
                    self._handleFloatAttribute(parentItem, fullPath,
                                               displayName, value,
                                               accessMode)

            if self.__schema.hasMaxInc(key):
                fullPath = key + "@maxInc"
                displayName = "Maximum inclusive"
                value = self.__schema.getMaxInc(key)

                if parentItem.classAlias == "Integer Field":
                    self._handleIntegerAttribute(parentItem, fullPath,
                                                 displayName, value,
                                                 accessMode)
                else:
                    self._handleFloatAttribute(parentItem, fullPath,
                                               displayName, value,
                                               accessMode)

            if self.__schema.hasMinExc(key):
                fullPath = key + "@minExc"
                displayName = "Minimum exclusive"
                value = self.__schema.getMinExc(key)

                if parentItem.classAlias == "Integer Field":
                    self._handleIntegerAttribute(parentItem, fullPath,
                                                 displayName, value,
                                                 accessMode)
                else:
                    self._handleFloatAttribute(parentItem, fullPath,
                                               displayName, value,
                                               accessMode)

            if self.__schema.hasMaxExc(key):
                fullPath = key + "@maxExc"
                displayName = "Maximum exclusive"
                value = self.__schema.getMaxExc(key)

                if parentItem.classAlias == "Integer Field":
                    self._handleIntegerAttribute(parentItem, fullPath,
                                                 displayName, value,
                                                 accessMode)
                else:
                    self._handleFloatAttribute(parentItem, fullPath,
                                               displayName, value,
                                               accessMode)

            if self.__schema.hasArchivePolicy(key):
                value = self.__schema.getArchivePolicy(key)
                print "ArchivePolicy", value
                #EVERY_EVENT,
                #EVERY_100MS,
                #EVERY_1S,
                #EVERY_5S,
                #EVERY_10S,
                #EVERY_1MIN,
                #EVERY_10MIN,
                #NO_ARCHIVING

        else: # AccessMode.RECONFIGURABLE
            # TODO:
            # RECONFIGURABLE for epsilon, forcedValue, isForced
            pass
            #if self.__schema.hasEpsilon(key):
            #    value = self.__schema.getEpsilon(key)
            
            #if self.__schema.hasForcedValue(key):
            #    value = self.__schema.getForcedValue(key)
            
            #if self.__schema.hasIsForced(key):
            #    value = self.__schema.getIsForced(key)


    def _setRequiredAccessLevel(self, key, item):
        item.requiredAccessLevel = self.__schema.getRequiredAccessLevel(key)


    def _setAllowedStates(self, key, item):
        if not self.__schema.hasAllowedStates(key):
            return
        
        item.allowedStates = self.__schema.getAllowedStates(key)


    def _getMinInc(self, key):
        if not self.__schema.hasMinInc(key):
            return None
        
        return self.__schema.getMinInc(key)
        

    def _getMaxInc(self, key):
        if not self.__schema.hasMaxInc(key):
            return None
        
        return self.__schema.getMaxInc(key)
        

    def _getMinExc(self, key):
        if not self.__schema.hasMinExc(key):
            return None
        
        return self.__schema.getMinExc(key)
        

    def _getMaxExc(self, key):
        if not self.__schema.hasMaxExc(key):
            return None
        
        return self.__schema.getMaxExc(key)


    def _setMinMaxIncAndExc(self, key, editableComponent):
        if not editableComponent:
            return
        
        # Check minimum and maximum inclusives/exclusives
        minInc = self._getMinInc(key)
        maxInc = self._getMaxInc(key)
        minExc = self._getMinExc(key)
        maxExc = self._getMaxExc(key)
        
        params = dict()
        if minInc:
            params['minInc'] =minInc
        if maxInc:
            params['maxInc'] = maxInc
        if minExc:
            params['minExc'] = minExc
        if maxExc:
            params['maxExc'] = maxExc
        
        editableComponent.addParameters(**params)


### functions for setting editable components depending on value type ###
    def _handleBool(self, key, item):
        #print "_handleBool"
        item.classAlias = "Toggle Field"
        item.setIcon(0, QIcon(":boolean"))
        
        accessMode = self._getAccessMode(key)
        editableComponent = None
        
        if self.__deviceType is NavigationItemTypes.CLASS:
            if (accessMode is AccessMode.INITONLY) or (accessMode is AccessMode.RECONFIGURABLE):
                editableComponent = EditableNoApplyComponent(classAlias=item.classAlias,
                                                             key=item.internalKey,
                                                             value=item.defaultValue, 
                                                             metricPrefixSymbol=item.metricPrefixSymbol,
                                                             unitSymbol=item.unitSymbol)
        else:
            if accessMode is AccessMode.RECONFIGURABLE:
                editableComponent = EditableApplyLaterComponent(classAlias=item.classAlias,
                                                                key=item.internalKey,
                                                                value=item.defaultValue,
                                                                metricPrefixSymbol=item.metricPrefixSymbol,
                                                                unitSymbol=item.unitSymbol)
                editableComponent.signalApplyChanged.connect(self.__treeWidget.onApplyChanged)

        item.editableComponent = editableComponent


    def _handleString(self, key, item):
        #print "_handleString"
        hasOptions = self.__schema.hasOptions(key)
        if hasOptions:
            item.classAlias = "Selection Field"
            item.enumeration = self.__schema.getOptions(key)
            item.setIcon(0, QIcon(":enum"))
        else:
            item.classAlias = "Text Field"
            item.enumeration = None
            item.setIcon(0, QIcon(":string"))
        
        accessMode = self._getAccessMode(key)
        editableComponent = None
        
        # PATH_ELEMENT
        if self.__schema.hasDisplayType(key):
            pathType = None
            if self.__schema.getDisplayType(key) == "directory":
                pathType = "directory"
            elif self.__schema.getDisplayType(key) == "fileIn":
                pathType = "fileIn"
            elif self.__schema.getDisplayType(key) == "fileOut":
                pathType = "fileOut"
            
            if pathType:
                item.setIcon(0, QIcon(":path"))
                
                if self.__deviceType is NavigationItemTypes.CLASS:
                    if (accessMode is AccessMode.INITONLY) or (accessMode is AccessMode.RECONFIGURABLE):
                        editableComponent = EditablePathNoApplyComponent(classAlias=item.classAlias,
                                                                            key=item.internalKey,
                                                                            value=item.defaultValue,
                                                                            enumeration=item.enumeration,
                                                                            metricPrefixSymbol=item.metricPrefixSymbol,
                                                                            unitSymbol=item.unitSymbol,
                                                                            pathType=pathType)
                else:
                    if accessMode is AccessMode.RECONFIGURABLE:
                        editableComponent = EditablePathApplyLaterComponent(classAlias=item.classAlias,
                                                                               key=item.internalKey,
                                                                               enumeration=item.enumeration, 
                                                                               metricPrefixSymbol=item.metricPrefixSymbol,
                                                                               unitSymbol=item.unitSymbol,
                                                                               pathType=pathType)
                        editableComponent.signalApplyChanged.connect(self.__treeWidget.onApplyChanged)
        
        if not editableComponent:
            # Not yet set as PATH_ELEMENT...
            if self.__deviceType is NavigationItemTypes.CLASS:
                if (accessMode is AccessMode.INITONLY) or (accessMode is AccessMode.RECONFIGURABLE):
                    editableComponent = EditableNoApplyComponent(classAlias=item.classAlias,
                                                                 key=item.internalKey,
                                                                 value=item.defaultValue,
                                                                 enumeration=item.enumeration,
                                                                 metricPrefixSymbol=item.metricPrefixSymbol,
                                                                 unitSymbol=item.unitSymbol)
            else:
                if accessMode is AccessMode.RECONFIGURABLE:
                    editableComponent = EditableApplyLaterComponent(classAlias=item.classAlias,
                                                                    key=item.internalKey,
                                                                    value=item.defaultValue,
                                                                    enumeration=item.enumeration, 
                                                                    metricPrefixSymbol=item.metricPrefixSymbol,
                                                                    unitSymbol=item.unitSymbol)
                    editableComponent.signalApplyChanged.connect(self.__treeWidget.onApplyChanged)

        item.editableComponent = editableComponent


    def _handleInteger(self, key, item):
        #print "_handleInteger"
        hasOptions = self.__schema.hasOptions(key)
        if hasOptions:
            item.classAlias = "Selection Field"
            item.enumeration = self.__schema.getOptions(key)
            item.setIcon(0, QIcon(":enum"))
        else:
            item.classAlias = "Integer Field"
            item.enumeration = None
            item.setIcon(0, QIcon(":int"))
        
        accessMode = self._getAccessMode(key)
        editableComponent = None
        
        if self.__deviceType is NavigationItemTypes.CLASS:
            if (accessMode is AccessMode.INITONLY) or (accessMode is AccessMode.RECONFIGURABLE):
                editableComponent = EditableNoApplyComponent(classAlias=item.classAlias,
                                                             key=item.internalKey,
                                                             value=item.defaultValue,
                                                             enumeration=item.enumeration,
                                                             metricPrefixSymbol=item.metricPrefixSymbol,
                                                             unitSymbol=item.unitSymbol)
        else:
            if accessMode is AccessMode.RECONFIGURABLE:
                editableComponent = EditableApplyLaterComponent(classAlias=item.classAlias,
                                                                key=item.internalKey,
                                                                value=item.defaultValue,
                                                                enumeration=item.enumeration,
                                                                metricPrefixSymbol=item.metricPrefixSymbol,
                                                                unitSymbol=item.unitSymbol)
                editableComponent.signalApplyChanged.connect(self.__treeWidget.onApplyChanged)
        
        # Check minimum and maximum inclusives/exclusives
        self._setMinMaxIncAndExc(key, editableComponent)
        item.editableComponent = editableComponent


    def _handleFloat(self, key, item):
        #print "_handleFloat"
        hasOptions = self.__schema.hasOptions(key)
        if hasOptions:
            item.classAlias = "Selection Field"
            item.enumeration = self.__schema.getOptions(key)
            item.setIcon(0, QIcon(":enum"))
        else:
            item.classAlias = "Float Field"
            item.enumeration = None
            item.setIcon(0, QIcon(":float"))

        accessMode = self._getAccessMode(key)
        editableComponent = None
        
        if self.__deviceType is NavigationItemTypes.CLASS:
            if (accessMode is AccessMode.INITONLY) or (accessMode is AccessMode.RECONFIGURABLE):
                editableComponent = EditableNoApplyComponent(classAlias=item.classAlias,
                                                             key=item.internalKey,
                                                             value=item.defaultValue,
                                                             enumeration=item.enumeration,
                                                             metricPrefixSymbol=item.metricPrefixSymbol,
                                                             unitSymbol=item.unitSymbol)
        else:
            if accessMode is AccessMode.RECONFIGURABLE:
                editableComponent = EditableApplyLaterComponent(classAlias=item.classAlias,
                                                                key=item.internalKey,
                                                                value=item.defaultValue,
                                                                enumeration=item.enumeration, 
                                                                metricPrefixSymbol=item.metricPrefixSymbol,
                                                                unitSymbol=item.unitSymbol)
                editableComponent.signalApplyChanged.connect(self.__treeWidget.onApplyChanged)
        
        # Check minimum and maximum inclusives/exclusives
        self._setMinMaxIncAndExc(key, editableComponent)
        item.editableComponent = editableComponent


    def _handleVector(self, key, item):
        #print "_handleVector"

        item.classAlias = "Histogram"

        accessMode = self._getAccessMode(key)
        editableComponent = None
        
        if self.__deviceType is NavigationItemTypes.CLASS:
            if (accessMode is AccessMode.INITONLY) or (accessMode is AccessMode.RECONFIGURABLE):
                editableComponent = EditableNoApplyComponent(classAlias=item.classAlias,
                                                             key=item.internalKey,
                                                             value=item.defaultValue,
                                                             metricPrefixSymbol=item.metricPrefixSymbol,
                                                             unitSymbol=item.unitSymbol)
        else:
            if accessMode is AccessMode.RECONFIGURABLE:
                editableComponent = EditableApplyLaterComponent(classAlias=item.classAlias,
                                                                key=item.internalKey,
                                                                value=None,
                                                                metricPrefixSymbol=item.metricPrefixSymbol,
                                                                unitSymbol=item.unitSymbol)
                editableComponent.signalApplyChanged.connect(self.__treeWidget.onApplyChanged)
        
        item.editableComponent = editableComponent


    def _handleFloatAttribute(self, parentItem, key, text, value, accessMode):
        
        fullPath = self.__rootPath + "." + key
        item = AttributeTreeWidgetItem(fullPath, self.__treeWidget, parentItem)
        item.setText(0, text)
        
        item.classAlias = "Float Field"
        item.setIcon(0, QIcon(":float-attribute"))
        
        editableComponent = None

        if self.__deviceType is NavigationItemTypes.CLASS:
            if (accessMode is AccessMode.INITONLY) or (accessMode is AccessMode.RECONFIGURABLE):
                editableComponent = EditableNoApplyComponent(classAlias=item.classAlias,
                                                             key=item.internalKey,
                                                             value=value)
        else:
            # Notify displayComponent
            item.displayComponent.onValueChanged(key, value)
            
            if accessMode is AccessMode.RECONFIGURABLE:
                editableComponent = EditableApplyLaterComponent(classAlias=item.classAlias,
                                                                key=item.internalKey,
                                                                value=value)
                editableComponent.signalApplyChanged.connect(self.__treeWidget.onApplyChanged)

        item.editableComponent = editableComponent


    def _handleIntegerAttribute(self, parentItem, key, text, value, accessMode):
        
        fullPath = self.__rootPath + "." + key
        item = AttributeTreeWidgetItem(fullPath, self.__treeWidget, parentItem)
        item.setText(0, text)
        
        item.classAlias = "Integer Field"
        item.setIcon(0, QIcon(":int-attribute"))
        
        editableComponent = None

        if self.__deviceType is NavigationItemTypes.CLASS:
            if (accessMode is AccessMode.INITONLY) or (accessMode is AccessMode.RECONFIGURABLE):
                editableComponent = EditableNoApplyComponent(classAlias=item.classAlias,
                                                             key=item.internalKey,
                                                             value=value)
        else:
            # Notify displayComponent
            item.displayComponent.onValueChanged(key, value)
            
            if accessMode is AccessMode.RECONFIGURABLE:
                editableComponent = EditableApplyLaterComponent(classAlias=item.classAlias,
                                                                key=item.internalKey,
                                                                value=value)
                editableComponent.signalApplyChanged.connect(self.__treeWidget.onApplyChanged)

        item.editableComponent = editableComponent

