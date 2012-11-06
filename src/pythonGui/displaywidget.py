#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 30, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a factory class to create
   display widgets.
"""

__all__ = ["DisplayWidget"]


import os
import sys
import displaywidgets

from PyQt4.QtCore import *
from PyQt4.QtGui import *


# helper function to get correct plugin classes without importing them
# moduleClassNameTuple contains (moduleName,className)
def importClass(moduleClassNameTuple):
    module = __import__(moduleClassNameTuple[0],fromlist=moduleClassNameTuple[1])
    module = getattr(module, moduleClassNameTuple[1])
    return module


class DisplayWidget(QObject):
    categoryToAliases = dict() # <category, [alias1,alias2,..]>
    aliasToCategory = dict() # <alias, category>
    aliasConcreteClass = dict() #<alias, (moduleName,className)>
    concreteFactories = dict()  #<className, className.Maker()>

    # helper function to get all subclasses
    @staticmethod
    def registerAvailableWidgets():
        path="displaywidgets" #"modules/userdisplaywidgets"
        
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
                    if not DisplayWidget.categoryToAliases.has_key(category):
                        DisplayWidget.categoryToAliases[category] = [alias]
                    else:
                        DisplayWidget.categoryToAliases[category].append(alias)
                        
                    # Register widget info in dictionary (<alias,category>) => 1 to 1 connection
                    if not DisplayWidget.aliasToCategory.has_key(alias):
                        DisplayWidget.aliasToCategory[alias] = category

                    if not DisplayWidget.aliasConcreteClass.has_key(alias):
                        DisplayWidget.aliasConcreteClass[alias] = (modulename,className)


    # A Template Method:
    @staticmethod
    def create(classAlias, **params):
        classAlias = str(classAlias)
        if not DisplayWidget.aliasConcreteClass.has_key(classAlias):
            raise KeyError, "Widget alias '%s' not found!" % classAlias

        # Get module and class name as tuple (moduleName,className)
        moduleClassNameTuple = DisplayWidget.aliasConcreteClass[classAlias]

        className = moduleClassNameTuple[1]
        if not DisplayWidget.concreteFactories.has_key(className):
            DisplayWidget.concreteFactories[className] = importClass(moduleClassNameTuple).Maker()
        
        params['classAlias'] = classAlias
        return DisplayWidget.concreteFactories[className].make(**params)
    

    # Returns all available aliases for passed category
    @staticmethod
    def getAliasesViaCategory(self, category):
        return DisplayWidget.categoryToAliases[str(category)]


    # Returns category for passed alias
    @staticmethod
    def getCategoryViaAlias(self, alias):
        return DisplayWidget.aliasToCategory[str(alias)]


    def __init__(self, **params):
        super(DisplayWidget, self).__init__()


    def _getCategory(self):
        raise NotImplementedError, "DisplayWidget._getCategory"
    category = property(fget=_getCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        raise NotImplementedError, "DisplayWidget._getWidget"
    widget = property(fget=_getWidget)


    def _getMinMaxAssociatedKeys(self):
        raise NotImplementedError, "DisplayWidget._getMinMaxAssociatedKeys"
    minMaxAssociatedKeys = property(fget=_getMinMaxAssociatedKeys)


    def _getKeys(self):
        raise NotImplementedError, "DisplayWidget._getKeys"
    keys = property(fget=_getKeys)


    def _getValue(self):
        raise NotImplementedError, "DisplayWidget._getValue"
    def _setValue(self, value):
        raise NotImplementedError, "DisplayWidget._setValue"
    value = property(fget=_getValue, fset=_setValue)


    def setErrorState(self, isError):
        raise NotImplementedError, "DisplayWidget._getWidget"


    def addKeyValue(self, key, value):
        raise NotImplementedError, "DisplayWidget.addKeyValue"


    def removeKeyValue(self, key):
        raise NotImplementedError, "DisplayWidget.removeKeyValue"


    def valueChanged(self, key, value, timestamp):
        raise NotImplementedError, "DisplayWidget.valueChanged"

