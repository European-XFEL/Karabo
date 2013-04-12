# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$Aug 2, 2012 10:39:43 AM$"

from libkarathon import *
from configurator import *

def KARABO_CLASSINFO(classid, version):
    def realDecorator(theClass):
        if isinstance(theClass, type):
            theClass.__classid__ = classid
            theClass.__version__ = version
            if hasattr(theClass, "__base_classid__"):
                Configurator(theClass.__base_classid__).registerClass(theClass)
        return theClass
    return realDecorator

def KARABO_CONFIGURATION_BASE_CLASS(theClass):
    if isinstance(theClass, type):
        Configurator.registerAsBaseClass(theClass)
    return theClass
