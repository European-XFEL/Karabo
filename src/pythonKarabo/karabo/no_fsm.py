# To change this license header, choose License Headers in Project Properties.
# To change this template file, choose Tools | Templates
# and open the template in the editor.

__author__="Sergey Esenov serguei.essenov@xfel.eu"
__date__ ="$Nov 26, 2014 3:18:24 PM$"

import copy
import threading
from collections import deque

from karabo.decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS

#======================================== Worker
class Worker(threading.Thread):
    
    def __init__(self, callback = None, timeout = -1, repetition = -1):
        threading.Thread.__init__(self)
        self.callback = callback
        self.onError = None
        self.onExit  = None
        self.timeout  = timeout
        self.repetition = repetition
        self.running = False
        self.aborted = False
        self.suspended = False
        self.counter = -1
        self.cv = threading.Condition()  # cv = condition variable
        self.dq = deque()
    
    def set(self, callback, timeout = -1, repetition = -1):
        self.callback = callback
        self.timeout  = timeout
        self.repetition = repetition

    def setTimeout(self, timeout = -1):
        self.timeout = timeout

    def setRepetition(self, repetition = -1):
        self.repetition = repetition
        
    def setErrorHandler(self, handler):
        self.onError = handler
        
    def setExitHandler(self, handler):
        self.onExit = handler

    def is_running(self):
        return self.running
    
    def push(self,o):
        if self.running:
            with self.cv:
                self.dq.append(o)
                self.cv.notify()
            
    def isRepetitionCounterExpired(self):
        return self.counter == 0
    
    def run(self):
        self.running = True
        self.aborted = False
        self.suspended = False
        self.counter = self.repetition
        try:
            if not callable(self.callback):
                raise ValueError("No callback is registered in Worker")
            while not self.aborted:
                t = None
                if self.counter == 0:
                    if callable(self.onExit):
                        self.onExit()
                    break
                if not self.running:
                    break
                if self.suspended:
                    with self.cv:
                        while self.suspended:
                            self.cv.wait()
                        if self.aborted or not self.running:
                            break
                    continue
                if self.timeout < 0:
                    with self.cv:
                        while len(self.dq) == 0:
                            self.cv.wait()
                        if not self.suspended:
                            t = self.dq.popleft()
                elif self.timeout > 0:
                    with self.cv:
                        if len(self.dq) == 0:
                            self.cv.wait(float(self.timeout) / 1000)   # self.timeout in milliseconds
                        if len(self.dq) != 0 and not self.suspended:
                            t = self.dq.popleft()
                else:
                    with self.cv:
                        if len(self.dq) != 0 and not self.suspended:
                            t = self.dq.popleft()
                if self.suspended:
                    continue
                if t is not None:
                    if stopCondition(t):
                        if callable(self.onExit):
                            self.onExit()
                        break
                if self.counter > 0:
                    self.counter -= 1
                if self.running:
                    self.callback()
        except RuntimeError as e:
            if callable(self.onError):
                self.onError(e)
        except:
            if callable(self.onError):
                self.onError(sys.exc_info()[0])
            
        if self.running:
            self.running = False
            
    def stopCondition(self, obj):
        return False
    
    def start(self):
        if not self.running:
            self.suspended = False
            super(Worker, self).start()
        if self.suspended:
            with self.cv:
                self.suspended = False
                self.cv.notify()
        return self
    
    def stop(self):
        if self.running:
            with self.cv:
                self.running = False
                self.suspended = False
                self.cv.notify()
        return self
    
    def abort(self):
        self.aborted = True
        self.running = False
        if self.suspended:
            with self.cv:
                self.suspended = False
                self.cv.notify()        
        if len(self.dq) != 0:
            with self.cv:
                self.dq.clear()
        return self
    
    def pause(self):
        if not self.suspended:
            with self.cv:
                self.suspended = True
                self.cv.notify()
            

@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("NoFsm", "1.3")
class NoFsm(object):
    
    @staticmethod
    def expectedParameters(expected):
        pass

    def __init__(self, configuration):
        super(NoFsm, self).__init__()
        self.func = None
    
    def startFsm(self):
        """Start state machine"""
        #self.updateState("Changing...")
        if self.func is None:
            raise RuntimeError("No initial function defined. Please call 'initialFunc' method in the device constructor")
        self.func()    # call initial function registered in the device constructor
        
    def registerInitialFunction(self, func):
        self.func = func
        
    def stopFsm(self): pass
    
    def errorFound(self, userFriendly, detail):
        print("*** ERROR *** : {} -- {}".format(userFriendly, detail))
