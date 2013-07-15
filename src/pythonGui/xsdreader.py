#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 14, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a generic XSD format reader."""

__all__ = ["XsdReader"]


from attributetreewidgetitem import AttributeTreeWidgetItem
from choicecomponent import ChoiceComponent
from editableapplylatercomponent import EditableApplyLaterComponent
from editablenoapplycomponent import EditableNoApplyComponent
from imagetreewidgetitem import ImageTreeWidgetItem
from karabo.karathon import *
from slottreewidgetitem import SlotTreeWidgetItem
from enums import *

from PyQt4.QtCore import *
from PyQt4.QtGui import *

class XsdReader(QXmlStreamReader):

    def __init__(self):
        super(XsdReader, self).__init__()
        self.__type = None


    def parseContent(self, twAttributeEditorPage, itemInfo):

        self.__type = itemInfo.get(QString('type'))
        if self.__type is None:
            self.__type = itemInfo.get('type')
        schema = itemInfo.get(QString('schema'))
        if schema is None:
            schema = itemInfo.get('schema')

        #file = QFile("/home/wegerk/Development/DemoDevice2.xsd")
        #if file.open(QIODevice.WriteOnly | QIODevice.Text) is False:
        #    return
        #out = QTextStream(file)
        #out << schema
        #file.close()

        self.clear()
        self.addData(schema)

        self.readNext() # StartDocument
        self.readNext()

        if self.name() != "schema":
            print "Configurator was fed with illegal XSD"
            return False

        self.processMainElementTag(twAttributeEditorPage)
        return True


    def processMainElementTag(self, twAttributeEditorPage):
    
        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()

            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "element":
                    self.processSimpleElements(twAttributeEditorPage)
            elif tokenType == QXmlStreamReader.EndElement:
                break


    def processSimpleElements(self, twAttributeEditorPage):

        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()

            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "element" :
                    name, type, defaultValue, minOccurs, maxOccurs = self.processSimpleElementAttributes()
                    # following tags
                    self.processFollowingElements(twAttributeEditorPage,
                                                  name, type, defaultValue, minOccurs, maxOccurs)
            elif tokenType == QXmlStreamReader.EndElement:
                break


    def processSimpleElementAttributes(self):
        name = self.attributes().value("name").toString()

        if self.attributes().hasAttribute("type"):
            type = self.attributes().value("type").toString()
        else:
            type = None

        if self.attributes().hasAttribute("default"):
            defaultValue = self.attributes().value("default").toString()
        else:
            defaultValue = None

        if self.attributes().hasAttribute("minOccurs"):
            minOccurs = self.attributes().value("minOccurs").toString()
        else:
            minOccurs = None

        if self.attributes().hasAttribute("maxOccurs"):
            maxOccurs = self.attributes().value("maxOccurs").toString()
        else:
            maxOccurs = None

        return [name, type, defaultValue, minOccurs, maxOccurs]


    def processFollowingElements(self, twAttributeEditorPage,
                                    name="", type="", defaultValue="",
                                    minOccurs="", maxOccurs="",
                                    parentItem=None, isSequenceElement=False):

        description = ""
        displayedName = ""
        expertLevel = ""
        default = ""
        unitName = ""
        unitSymbol = ""
        accessType = ""
        displayType = ""
        allowedStates = []
        restrictionBase = ""
        minInclusive = None
        maxInclusive = None
        enumeration = []

        complexItem = None
        while self.atEnd() == False :
            tokenType = self.readNext()
            tagName = self.name()

            if tokenType == QXmlStreamReader.StartElement :
                if tagName == "annotation":
                    description, displayedName, expertLevel, default, unitName, unitSymbol, accessType, displayType, allowedStates = self.processAnnotationTag()
                elif tagName == "simpleType":
                    restrictionBase, minInclusive, maxInclusive, enumeration = self.processSimpleTypeTag()
                elif tagName == "complexType":
                    
                    if (self.__type is NavigationItemTypes.CLASS) and (accessType == AccessMode.READONLY):
                        complexItem = None
                    else:
                        # Some complex types need special treatment
                        if displayType == "Slot":
                            complexItem = SlotTreeWidgetItem(name, twAttributeEditorPage, parentItem)
                            if self.__type is NavigationItemTypes.DEVICE:
                                complexItem.enabled = True
                            complexItem.classAlias = "Command"
                        elif displayType == "Image":
                            complexItem = ImageTreeWidgetItem(name, twAttributeEditorPage, parentItem)
                            complexItem.classAlias = "Image View"
                        else:
                            complexItem = AttributeTreeWidgetItem(name, twAttributeEditorPage, parentItem)
                        
                        complexItem.allowedStates = allowedStates
                        if minOccurs == "1" :
                            complexItem.setColorMandatory()

                        itemName = ""
                        if displayedName == "":
                            itemName = name
                        else:
                            itemName = displayedName

                        complexItem.displayText = itemName
                        complexItem.defaultValue = default

                        complexItem.accessType = accessType

                        if len(expertLevel) > 0 :
                            complexItem.expertLevel = int(expertLevel)
                    
                    self.setUpdateStatus(parentItem, complexItem)
                    
                    self.processComplexTypeTag(twAttributeEditorPage, complexItem, isSequenceElement)
            elif tokenType == QXmlStreamReader.EndElement and tagName == "element":
                break

        # create simple element
        if complexItem is None:
                if (self.__type is NavigationItemTypes.CLASS) and (accessType == AccessMode.READONLY):
                    return None
                
                if isSequenceElement == False :
                    attributeItem = AttributeTreeWidgetItem(name, twAttributeEditorPage, parentItem)
                    attributeItem.accessType = accessType
                    attributeItem.allowedStates = allowedStates

                    if minOccurs == "1" :
                        attributeItem.setColorMandatory()

                    itemName = ""
                    if len(displayedName) < 0 :
                        itemName = name
                    else :
                        itemName = displayedName
                    attributeItem.displayText = itemName
                    
                    if len(expertLevel) > 0 :
                        attributeItem.expertLevel = int(expertLevel)

                    #attributeItem.unitSymbol = unitSymbol

                    self.setUpdateStatus(parentItem, attributeItem)
                    
                    createEditableComponent = True
                    if self.__type is NavigationItemTypes.DEVICE:
                        if (accessType == AccessMode.READONLY) or (accessType == AccessMode.INITONLY):
                            self.setItemIcon(attributeItem, type, restrictionBase)
                            createEditableComponent = False
                    
                    if type == "xs:string" or restrictionBase == "xs:string" :
                        attributeItem.valueType = "string"
                        if displayType == "Path":
                            attributeItem.classAlias = "File Path"
                            if createEditableComponent is False:
                                return complexItem
                            
                            attributeItem.defaultValue = default
                            
                            if self.__type is NavigationItemTypes.CLASS:
                                editableComponent = EditableNoApplyComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=attributeItem.defaultValue, valueType=attributeItem.valueType, unitSymbol=unitSymbol)
                            else:
                                editableComponent = EditableApplyLaterComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=None, valueType=attributeItem.valueType, unitSymbol=unitSymbol)
                                editableComponent.signalApplyChanged.connect(twAttributeEditorPage.onApplyChanged)
                            
                            attributeItem.setIcon(0, QIcon(":path"))
                            attributeItem.setEditableComponent(editableComponent)
                        else:
                            attributeItem.defaultValue = default
                            
                            if len(enumeration) < 1 :
                                attributeItem.classAlias = "Text Field"
                                if createEditableComponent is False:
                                    return complexItem
                                
                                if self.__type is NavigationItemTypes.CLASS:
                                    editableComponent = EditableNoApplyComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=attributeItem.defaultValue, valueType=attributeItem.valueType, unitSymbol=unitSymbol)
                                else:
                                    editableComponent = EditableApplyLaterComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=None, valueType=attributeItem.valueType, unitSymbol=unitSymbol)
                                    editableComponent.signalApplyChanged.connect(twAttributeEditorPage.onApplyChanged)
                                    
                                attributeItem.setIcon(0, QIcon(":string"))
                                attributeItem.setEditableComponent(editableComponent)
                            else :
                                attributeItem.classAlias = "Selection Field"
                                if createEditableComponent is False:
                                    return complexItem
                                
                                if self.__type is NavigationItemTypes.CLASS:
                                    editableComponent = EditableNoApplyComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=attributeItem.defaultValue, valueType=attributeItem.valueType, unitSymbol=unitSymbol, enumeration=enumeration)
                                else:
                                    editableComponent = EditableApplyLaterComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=None, valueType=attributeItem.valueType, unitSymbol=unitSymbol, enumeration=enumeration)
                                    editableComponent.signalApplyChanged.connect(twAttributeEditorPage.onApplyChanged)
                                
                                attributeItem.setIcon(0, QIcon(":enum"))
                                attributeItem.setEditableComponent(editableComponent)
                    elif (type == "xs:int" or restrictionBase == "xs:int" or type == "xs:unsignedInt" or restrictionBase == "xs:unsignedInt"
                                                                          or type == "xs:unsignedShort" or restrictionBase == "xs:unsignedShort"
                                                                          or type == "xs:unsignedLong" or restrictionBase == "xs:unsignedLong"
                                                                          or type == "xs:unsignedByte" or restrictionBase == "xs:unsignedByte"):
                        attributeItem.valueType = "int"
                        if default:
                            attributeItem.defaultValue = int(default)
                        
                        if len(enumeration) < 1 :
                            attributeItem.classAlias = "Integer Field"
                            if createEditableComponent is False:
                                return complexItem
                            
                            if self.__type is NavigationItemTypes.CLASS:
                                editableComponent = EditableNoApplyComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=attributeItem.defaultValue, valueType=attributeItem.valueType, unitSymbol=unitSymbol)
                            else:
                                editableComponent = EditableApplyLaterComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=None, valueType=attributeItem.valueType, unitSymbol=unitSymbol)
                                editableComponent.signalApplyChanged.connect(twAttributeEditorPage.onApplyChanged)
                                
                            if minInclusive and len(minInclusive) > 0:
                                editableComponent.addParameters(minimum=int(minInclusive))
                            if maxInclusive and len(maxInclusive) > 0:
                                editableComponent.addParameters(maximum=int(maxInclusive))
                            attributeItem.setIcon(0, QIcon(":int"))
                            attributeItem.setEditableComponent(editableComponent)
                        else :
                            attributeItem.classAlias = "Selection Field"
                            if createEditableComponent is False:
                                return complexItem
                            
                            if self.__type is NavigationItemTypes.CLASS:
                                editableComponent = EditableNoApplyComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=attributeItem.defaultValue, valueType=attributeItem.valueType, unitSymbol=unitSymbol, enumeration=enumeration)
                            else:
                                editableComponent = EditableApplyLaterComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=None, valueType=attributeItem.valueType, unitSymbol=unitSymbol, enumeration=enumeration)
                                editableComponent.signalApplyChanged.connect(twAttributeEditorPage.onApplyChanged)
                            
                            attributeItem.setIcon(0, QIcon(":enum"))
                            attributeItem.setEditableComponent(editableComponent)
                    elif type == "xs:float" or restrictionBase == "xs:float" :
                        attributeItem.valueType = "float"
                        if default:
                            attributeItem.defaultValue = float(default)
                        
                        if len(enumeration) < 1 :
                            attributeItem.classAlias = "Float Field"
                            if createEditableComponent is False:
                                return complexItem
                            
                            if self.__type is NavigationItemTypes.CLASS:
                                editableComponent = EditableNoApplyComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=attributeItem.defaultValue, valueType=attributeItem.valueType, unitSymbol=unitSymbol)
                            else:
                                editableComponent = EditableApplyLaterComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=None, valueType=attributeItem.valueType, unitSymbol=unitSymbol)
                                editableComponent.signalApplyChanged.connect(twAttributeEditorPage.onApplyChanged)
                            
                            if minInclusive and len(minInclusive) > 0:
                                editableComponent.addParameters(minimum=float(minInclusive))
                            if maxInclusive and len(maxInclusive) > 0:
                                editableComponent.addParameters(maximum=float(maxInclusive))
                            attributeItem.setIcon(0, QIcon(":float"))
                            attributeItem.setEditableComponent(editableComponent)
                        else :
                            attributeItem.classAlias = "Selection Field"
                            if createEditableComponent is False:
                                return complexItem
                            
                            if self.__type is NavigationItemTypes.CLASS:
                                editableComponent = EditableNoApplyComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=attributeItem.defaultValue, valueType=attributeItem.valueType, unitSymbol=unitSymbol, enumeration=enumeration)
                            else:
                                editableComponent = EditableApplyLaterComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=None, valueType=attributeItem.valueType, unitSymbol=unitSymbol, enumeration=enumeration)
                                editableComponent.signalApplyChanged.connect(twAttributeEditorPage.onApplyChanged)
                            
                            attributeItem.setIcon(0, QIcon(":enum"))
                            attributeItem.setEditableComponent(editableComponent)
                    elif type == "xs:double" or restrictionBase == "xs:double" :
                        attributeItem.valueType = "double"
                        if default:
                            attributeItem.defaultValue = float(default)
                        
                        if len(enumeration) < 1 :
                            attributeItem.classAlias = "Float Field"
                            if createEditableComponent is False:
                                return complexItem
                            
                            if self.__type is NavigationItemTypes.CLASS:
                                editableComponent = EditableNoApplyComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=attributeItem.defaultValue, valueType=attributeItem.valueType, unitSymbol=unitSymbol)
                            else:
                                editableComponent = EditableApplyLaterComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=None, valueType=attributeItem.valueType, unitSymbol=unitSymbol)
                                editableComponent.signalApplyChanged.connect(twAttributeEditorPage.onApplyChanged)
                            
                            if minInclusive and len(minInclusive) > 0:
                                editableComponent.addParameters(minimum=float(minInclusive))
                            if maxInclusive and len(maxInclusive) > 0:
                                editableComponent.addParameters(maximum=float(maxInclusive))
                            attributeItem.setIcon(0, QIcon(":float"))
                            attributeItem.setEditableComponent(editableComponent)
                        else :
                            attributeItem.classAlias = "Selection Field"
                            if createEditableComponent is False:
                                return complexItem
                            
                            if self.__type is NavigationItemTypes.CLASS:
                                editableComponent = EditableNoApplyComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=attributeItem.defaultValue, valueType=attributeItem.valueType, unitSymbol=unitSymbol, enumeration=enumeration)
                            else:
                                editableComponent = EditableApplyLaterComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=None, valueType=attributeItem.valueType, unitSymbol=unitSymbol, enumeration=enumeration)
                                editableComponent.signalApplyChanged.connect(twAttributeEditorPage.onApplyChanged)
                                
                            attributeItem.setIcon(0, QIcon(":enum"))
                            attributeItem.setEditableComponent(editableComponent)
                    elif type == "xs:boolean" or restrictionBase == "xs:boolean" :
                        attributeItem.valueType = "bool"
                        attributeItem.classAlias = "Toggle Field"
                        if createEditableComponent is False:
                            return complexItem
                        attributeItem.defaultValue = default
                        
                        if self.__type is NavigationItemTypes.CLASS:
                            editableComponent = EditableNoApplyComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=attributeItem.defaultValue, valueType=attributeItem.valueType, unitSymbol=unitSymbol)
                        else:
                            editableComponent = EditableApplyLaterComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=None, valueType=attributeItem.valueType, unitSymbol=unitSymbol)
                            editableComponent.signalApplyChanged.connect(twAttributeEditorPage.onApplyChanged)
                        
                        attributeItem.setIcon(0, QIcon(":boolean"))
                        attributeItem.setEditableComponent(editableComponent)
                    elif type == "xs:anyURI" or restrictionBase == "xs:anyURI" :
                        # soon not supported anymore: look for string
                        attributeItem.valueType = "string"
                        attributeItem.classAlias = "File Path"
                        if createEditableComponent is False:
                            return complexItem
                        attributeItem.defaultValue = default
                        
                        if self.__type is NavigationItemTypes.CLASS:
                            editableComponent = EditableNoApplyComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=attributeItem.defaultValue, valueType=attributeItem.valueType, unitSymbol=unitSymbol)
                        else:
                            editableComponent = EditableApplyLaterComponent(attributeItem.classAlias, key=attributeItem.internalKey, value=None, valueType=attributeItem.valueType, unitSymbol=unitSymbol)
                            editableComponent.signalApplyChanged.connect(twAttributeEditorPage.onApplyChanged)
                        
                        attributeItem.setIcon(0, QIcon(":path"))
                        attributeItem.setEditableComponent(editableComponent)
                else:
                    # Vector element...
                    if parentItem is None:
                        return None
                    
                    parentItem.classAlias = "Histogram"
                    parentItem.setIcon(0, QIcon(":enum"))
                    
                    if self.__type is NavigationItemTypes.DEVICE:
                        if (parentItem.accessType == AccessMode.READONLY) or (parentItem.accessType == AccessMode.INITONLY):
                            return complexItem
                    
                    default = parentItem.defaultValue
                    defaultVec = []
                    if default:
                         defaultVec = str(default).split(',')
                    default = []
                    if type == "xs:float" or restrictionBase == "xs:float":
                        parentItem.valueType = "float"
                        for index in defaultVec:
                            default.append(float(index))
                    elif (type == "xs:int" or restrictionBase == "xs:int" or type == "xs:unsignedInt" or restrictionBase == "xs:unsignedInt"
                                                                          or type == "xs:unsignedShort" or restrictionBase == "xs:unsignedShort"
                                                                          or type == "xs:unsignedLong" or restrictionBase == "xs:unsignedLong"
                                                                          or type == "xs:unsignedByte" or restrictionBase == "xs:unsignedByte"):
                        parentItem.valueType = "int"
                        for index in defaultVec:
                            default.append(int(index))
                    elif type == "xs:string" or restrictionBase == "xs:string":
                        parentItem.valueType = "string"
                        for index in defaultVec:
                            default.append(str(index))
                    
                    parentItem.defaultValue = default
                    parentItem.setIcon(0, QIcon(":enum"))
                    
                    if self.__type is NavigationItemTypes.CLASS:
                        parentItem.setEditableComponent(EditableNoApplyComponent(parentItem.classAlias, key=parentItem.internalKey, value=parentItem.defaultValue, valueType=parentItem.valueType, unitSymbol=unitSymbol))
                    else:
                        editableComponent = EditableApplyLaterComponent(parentItem.classAlias, key=parentItem.internalKey, value=None, valueType=parentItem.valueType, unitSymbol=unitSymbol)
                        editableComponent.signalApplyChanged.connect(twAttributeEditorPage.onApplyChanged)
                        parentItem.setEditableComponent(editableComponent)
        
        return complexItem


    def processAnnotationTag(self):

        description = None
        displayedName = None
        expertLevel = ""
        default = None
        unitName = None
        unitSymbol = None
        accessType = None
        displayType = None
        allowedStates = None

        while self.atEnd() == False :
            tokenType = self.readNext()
            tagName = self.name()

            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "description":
                    self.readNext()
                    description = self.text().toString()
                elif tagName == "displayedName":
                    self.readNext()
                    displayedName = self.text().toString()
                elif tagName == "expertLevel":
                    self.readNext()
                    expertLevel = self.text().toString()
                elif tagName == "default":
                    self.readNext()
                    default = self.text().toString()
                elif tagName == "unitName":
                    self.readNext()
                    unitName = self.text().toString()
                elif tagName == "unitSymbol":
                    self.readNext()
                    unitSymbol = self.text().toString()
                elif tagName == "accessType":
                    self.readNext()
                    accessType = self.text().toString()
                elif tagName == "displayType":
                    self.readNext()
                    displayType = self.text().toString()
                elif tagName == "allowedStates":
                    self.readNext()
                    allowedStates = list(self.text().toString().split(','))
            elif tokenType == QXmlStreamReader.EndElement and tagName == "annotation":
                break

        return [description, displayedName, expertLevel, default, unitName, unitSymbol, accessType, displayType, allowedStates]


    def processSimpleTypeTag(self):

        restrictionBase = None
        minInclusive = None
        maxInclusive = None
        enumeration = []

        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()

            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "restriction":
                    restrictionBase = self.attributes().value("base").toString()
                elif tagName == "minInclusive":
                    minInclusive = self.attributes().value("value").toString()
                elif tagName == "maxInclusive":
                    maxInclusive = self.attributes().value("value").toString()
                elif tagName == "enumeration":
                    enumeration.append(self.attributes().value("value").toString())
            elif tokenType == QXmlStreamReader.EndElement and tagName == "simpleType":
                break

        return [restrictionBase, minInclusive, maxInclusive, enumeration]


    def processComplexTypeTag(self, twAttributeEditorPage, parentItem, isSequenceElement=False):

        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()

            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "choice":
                    self.processChoiceTag(twAttributeEditorPage, parentItem, isSequenceElement)
                elif tagName == "element":
                    name, type, defaultValue, minOccurs, maxOccurs = self.processSimpleElementAttributes()
                    # process children
                    self.processFollowingElements(twAttributeEditorPage, name, type, defaultValue, minOccurs, maxOccurs, parentItem)
                elif tagName == "sequence":
                    self.processSequenceTag(twAttributeEditorPage, parentItem)
                elif tagName == "attribute":
                    name = self.attributes().value("name").toString()
                    type = self.attributes().value("type").toString()
                    default = self.attributes().value("default").toString()
            elif tokenType == QXmlStreamReader.EndElement and tagName == "complexType":
                break


    def processChoiceTag(self, twAttributeEditorPage, parentItem, isSequenceElement=False):
        choiceComponent = None
        
        defaultDataFromParent = None
        if isSequenceElement == False:
            parentItem.isChoiceElement = True
            parentItem.classAlias = "Choice Element"
            # Choiceelements can not have strings as arguments
            parentItem.defaultValue = Hash(str(parentItem.defaultValue))
            
            if self.__type is NavigationItemTypes.CLASS:
                choiceComponent = EditableNoApplyComponent(parentItem.classAlias, key=parentItem.internalKey, value=parentItem.defaultValue, valueType=parentItem.valueType)
            else:
                if parentItem.accessType == AccessMode.READONLY or parentItem.accessType == AccessMode.INITONLY:
                    choiceComponent = ChoiceComponent(parentItem.classAlias, key=parentItem.internalKey, value=None, valueType=parentItem.valueType)
                else:
                    choiceComponent = EditableApplyLaterComponent(parentItem.classAlias, key=parentItem.internalKey, value=None, valueType=parentItem.valueType)
                    choiceComponent.signalApplyChanged.connect(twAttributeEditorPage.onApplyChanged)
            defaultDataFromParent = parentItem.defaultValue
        else:
            listParentItem = parentItem.parent()
            listParentItem.removeChild(parentItem)
            
            parentItem = listParentItem
            parentItem.isListElement = True
            parentItem.updatedNeeded = False
            parentItem.classAlias = "List Element Field"
            
            if self.__type is NavigationItemTypes.CLASS:
                choiceComponent = EditableNoApplyComponent(parentItem.classAlias, key=parentItem.internalKey, value=parentItem.defaultValue, valueType=parentItem.valueType, isDevIns=False)
            else:
                if parentItem.accessType == AccessMode.READONLY or parentItem.accessType == AccessMode.INITONLY:
                    choiceComponent = ChoiceComponent(parentItem.classAlias, key=parentItem.internalKey, value=None, valueType=parentItem.valueType, isDevIns=True)
                else:
                    choiceComponent = EditableApplyLaterComponent(parentItem.classAlias, key=parentItem.internalKey, value=None, valueType=parentItem.valueType, isDevIns=True)
                    choiceComponent.signalApplyChanged.connect(twAttributeEditorPage.onApplyChanged)

        parentItem.setExpanded(True)
        parentItem.setEditableComponent(choiceComponent)

        while self.atEnd() == False :
            tokenType = self.readNext()
            tagName = self.name()

            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "element":
                    name, type, defaultValue, minOccurs, maxOccurs = self.processSimpleElementAttributes()

                    childItem = self.processFollowingElements(twAttributeEditorPage,
                                                              name, type, defaultValue,
                                                              minOccurs, maxOccurs, parentItem, isSequenceElement)

                    if (childItem is not None) and (childItem.text(0) != defaultDataFromParent):
                        childItem.setHidden(True)

                    choiceComponent.addParameters(itemToBeAdded=childItem)

            elif tokenType == QXmlStreamReader.EndElement and tagName == "choice":
                break

        parentItem.onSetToDefault()


    def processSequenceTag(self, twAttributeEditorPage, parentItem):

        while self.atEnd() == False :
            tokenType = self.readNext()
            tagName = self.name()

            if tokenType == QXmlStreamReader.StartElement :
                if tagName == "element" :
                    name, type, defaultValue, minOccurs, maxOccurs = self.processSimpleElementAttributes()

                self.processFollowingElements(twAttributeEditorPage, name, type, defaultValue,
                                              minOccurs, maxOccurs, parentItem, True)

            elif tokenType == QXmlStreamReader.EndElement and tagName == "sequence" :
                break


    def setItemIcon(self, item, type, restrictionBase):
        icon = QIcon(":undefined")
        if type == "xs:string" or restrictionBase == "xs:string":
            icon = QIcon(":string")
        if (type == "xs:int" or restrictionBase == "xs:int" or type == "xs:unsignedInt" or restrictionBase == "xs:unsignedInt"
                                                            or type == "xs:unsignedShort" or restrictionBase == "xs:unsignedShort"
                                                            or type == "xs:unsignedLong" or restrictionBase == "xs:unsignedLong"
                                                            or type == "xs:unsignedByte" or restrictionBase == "xs:unsignedByte"):
            icon = QIcon(":int")
        if type == "xs:float" or restrictionBase == "xs:float":
            icon = QIcon(":float")
        if type == "xs:double" or restrictionBase == "xs:double":
            icon = QIcon(":float")
        if type == "xs:boolean" or restrictionBase == "xs:boolean":
            icon = QIcon(":boolean")
        if type == "xs:anyURI" or restrictionBase == "xs:anyURI":
            icon = QIcon(":path")
        item.setIcon(0, icon)


    def setUpdateStatus(self, parentItem, item):
        if (parentItem is None) or (item is None):
            return

        if parentItem.updateNeeded == True:
            if parentItem.isChoiceElement == True:
                if parentItem.defaultValue == item.text(0):
                    item.updateNeeded = True
                else :
                    item.updateNeeded = False
            else:
                item.updateNeeded = True
        else :
            item.updateNeeded = False

