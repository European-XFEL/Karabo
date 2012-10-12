#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 11, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a factory class to create
   vacuum widgets.
"""

__all__ = ["VacuumWidget"]


import os
import sys
import vacuumwidgets

from PyQt4.QtCore import *
from PyQt4.QtGui import *


# helper function to get correct plugin classes without importing them
# moduleClassNameTuple contains (moduleName,className)
def importClass(moduleClassNameTuple):
    module = __import__(moduleClassNameTuple[0],fromlist=moduleClassNameTuple[1])
    module = getattr(module, moduleClassNameTuple[1])
    return module


class VacuumWidget(QObject):
    categoryToAliases = dict() # <category, [alias1,alias2,..]>
    aliasToCategory = dict() # <alias, category>
    aliasConcreteClass = dict() #<alias, (moduleName,className)>
    concreteFactories = dict()  #<className, className.Maker()>

    # helper function to get all subclasses
    @staticmethod
    def registerAvailableWidgets():
        path="vacuumwidgets"
        
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
                    if not VacuumWidget.categoryToAliases.has_key(category):
                        VacuumWidget.categoryToAliases[category] = [alias]
                    else:
                        VacuumWidget.categoryToAliases[category].append(alias)
                        
                    # Register widget info in dictionary (<alias,category>) => 1 to 1 connection
                    if not VacuumWidget.aliasToCategory.has_key(alias):
                        VacuumWidget.aliasToCategory[alias] = category
                    else:
                        print "+++ VacuumWidget.aliasToCategory.has_key", alias

                    if not VacuumWidget.aliasConcreteClass.has_key(alias):
                        VacuumWidget.aliasConcreteClass[alias] = (modulename,className)


    # A Template Method:
    @staticmethod
    def create(classAlias, **params):
        classAlias = str(classAlias)
        if not VacuumWidget.aliasConcreteClass.has_key(classAlias):
            raise KeyError, "Widget alias '%s' not found!" % classAlias

        # Get module and class name as tuple (moduleName,className)
        moduleClassNameTuple = VacuumWidget.aliasConcreteClass[classAlias]

        className = moduleClassNameTuple[1]
        if not VacuumWidget.concreteFactories.has_key(className):
            VacuumWidget.concreteFactories[className] = importClass(moduleClassNameTuple).Maker()
        
        params['classAlias'] = classAlias
        return VacuumWidget.concreteFactories[className].make(**params)
    

    # Returns all available aliases for passed category
    @staticmethod
    def getAliasesViaCategory(self, category):
        return VacuumWidget.categoryToAliases[str(category)]


    # Returns category for passed alias
    @staticmethod
    def getCategoryViaAlias(self, alias):
        return VacuumWidget.aliasToCategory[str(alias)]


    def __init__(self, **params):
        super(VacuumWidget, self).__init__()


    def _getCategory(self):
        raise NotImplementedError, "VacuumWidget._getCategory"
    category = property(fget=_getCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        raise NotImplementedError, "VacuumWidget._getWidget"
    widget = property(fget=_getWidget)


    def _getMinMaxAssociatedKeys(self):
        raise NotImplementedError, "VacuumWidget._getMinMaxAssociatedKeys"
    minMaxAssociatedKeys = property(fget=_getMinMaxAssociatedKeys)


    def _getKeys(self):
        raise NotImplementedError, "VacuumWidget._getKeys"
    keys = property(fget=_getKeys)


    def _getValue(self):
        raise NotImplementedError, "VacuumWidget._getValue"
    def _setValue(self, value):
        raise NotImplementedError, "VacuumWidget._setValue"
    value = property(fget=_getValue, fset=_setValue)


    def setErrorState(self, isError):
        raise NotImplementedError, "VacuumWidget._getWidget"


    def addKeyValue(self, key, value):
        raise NotImplementedError, "VacuumWidget.addKeyValue"


    def removeKeyValue(self, key):
        raise NotImplementedError, "VacuumWidget.removeKeyValue"


    def valueChanged(self, key, value, timestamp):
        raise NotImplementedError, "VacuumWidget.valueChanged"

