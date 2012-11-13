#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 30, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a factory class to create
   editable widgets.
"""

__all__ = ["EditableWidget"]


import os
import sys
import editablewidgets

from PyQt4.QtCore import *
from PyQt4.QtGui import *


# helper function to get correct plugin classes without importing them
# moduleClassNameTuple contains (moduleName,className)
def importClass(moduleClassNameTuple):
    module = __import__(moduleClassNameTuple[0],fromlist=moduleClassNameTuple[1])
    module = getattr(module, moduleClassNameTuple[1])
    return module


class EditableWidget(QObject):
    # Signals
    signalEditingFinished = pyqtSignal(str, object) # key, value
    
    categoryToAliases = dict() # <alias, category>
    aliasToCategory = dict() # <alias, category>
    aliasConcreteClass = dict() #<alias, (moduleName,className)>
    concreteFactories = dict()  #<className, className.Maker()>

    # helper function to get all subclasses
    @staticmethod
    def registerAvailableWidgets():
        path="editablewidgets" #"modules/editablewidgets"
        
        for root, dirs, files in os.walk(path):
            for name in files:
                if name.endswith(".py") and not name.startswith("__"):
                    path = os.path.join(root, name)

                    if sys.platform == 'win32':
                        modulename = path.rsplit('.', 1)[0].replace('\\', '.')
                    else:
                        modulename = path.rsplit('.', 1)[0].replace('/', '.')
                    
                    # Import module
                    __import__(modulename)
                    
                    # Get alias and class name as tuple (moduleName,className)
                    category, alias, className = eval(modulename + '.getCategoryAliasClassName()')
                    # Register widget info in dictionary (<category,[aliasList]>) => 1 to n connection
                    if not EditableWidget.categoryToAliases.has_key(category):
                        EditableWidget.categoryToAliases[category] = [alias]
                    else:
                        EditableWidget.categoryToAliases[category].append(alias)
                    
                    # Register widget info in dictionary (<alias,category>) => 1 to 1 connection
                    if not EditableWidget.aliasToCategory.has_key(alias):
                        EditableWidget.aliasToCategory[alias] = category
                    
                    # Register widget info in dictionary (<alias, (moduleName,className)>)
                    if not EditableWidget.aliasConcreteClass.has_key(alias):
                        EditableWidget.aliasConcreteClass[alias] = (modulename,className)


    # A Template Method:
    @staticmethod
    def create(classAlias, **params):
        classAlias = str(classAlias)
        if not EditableWidget.aliasConcreteClass.has_key(classAlias):
            raise KeyError, "Widget alias '%s' not found!" % classAlias

        # Get module and class name as tuple (moduleName,className)
        moduleClassNameTuple = EditableWidget.aliasConcreteClass[classAlias]

        className = moduleClassNameTuple[1]
        if not EditableWidget.concreteFactories.has_key(className):
            EditableWidget.concreteFactories[className] = importClass(moduleClassNameTuple).Maker()

        params['classAlias'] = classAlias
        return EditableWidget.concreteFactories[className].make(**params)


    # Returns all available aliases for passed category
    @staticmethod
    def getAliasesViaCategory(self, category):
        return EditableWidget.categoryToAliases[str(category)]


    # Returns category for passed alias
    @staticmethod
    def getCategoryViaAlias(self, alias):
        return EditableWidget.aliasToCategory[str(alias)]


    def __init__(self, **params):
        super(EditableWidget, self).__init__()


    def _getCategory(self):
        raise NotImplementedError, "EditableWidget._getCategory"
    category = property(fget=_getCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        raise NotImplementedError, "EditableWidget._getWidget"
    widget = property(fget=_getWidget)


    def _getMinMaxAssociatedKeys(self):
        raise NotImplementedError, "EditableWidget._getMinMaxAssociatedKeys"
    minMaxAssociatedKeys = property(fget=_getMinMaxAssociatedKeys)


    def _getKeys(self):
        raise NotImplementedError, "EditableWidget._getKeys"
    keys = property(fget=_getKeys)


    def _getValue(self):
        raise NotImplementedError, "EditableWidget._getValue"
    value = property(fget=_getValue)


    def addParameters(self, **params):
        raise NotImplementedError, "EditableWidget.addParameters"


    def addKeyValue(self, key, value):
        raise NotImplementedError, "EditableWidget.addKeyValue"


    def removeKeyValue(self, key):
        raise NotImplementedError, "EditableWidget.removeKeyValue"


    def valueChanged(self, key, value, timestamp):
        raise NotImplementedError, "EditableWidget.valueChanged"


    # Called from sub-classes if value was edited
    def valueEditingFinished(self, key, value):
        self.signalEditingFinished.emit(str(key), value)

