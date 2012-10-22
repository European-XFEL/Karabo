# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="esenov"
__date__ ="$Sep 5, 2012 4:34:33 PM$"

class FsmException(Exception): pass
class ParameterException(Exception): pass
class LogicException(Exception): pass
class TransitionError(FsmException): pass
