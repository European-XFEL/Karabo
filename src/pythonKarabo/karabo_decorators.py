# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="esenov"
__date__ ="$Aug 2, 2012 10:39:43 AM$"

from libkarabo import Schema, AccessType, INIT, READ, WRITE

def schemamethod(f):

    def deco(cls):
        master = Schema()
        expected = master.initParameterDescription(cls.__name__, AccessType(INIT|READ|WRITE), "")
        f(expected)
        return master

    deco = classmethod(deco)
    return deco