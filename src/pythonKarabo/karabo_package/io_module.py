# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$Sep 11, 2013 3:29:29 PM$"


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("Input", "1.0")
class Input(object):
    
    def __init__(self, configuration):
        super(Input, self).__init__()
        self.instanceId = None
        self.ioEventHandler = None
        self.endOfStreamEventHandler = None
        
    @abstractmethod
    def read(self, data, idx):
        pass
    
    @abstractmethod
    def size(self):
        return 0;
    
    def reconfigure(self, hashInput):
        pass
    
    def setInstanceId(self, id):
        self.instanceId = id
        
    def getInstanceId(self):
        return self.instanceId
    
    def registerIOEventHandler(self, ioEventHandler):
        self.ioEventHandler = ioEventHandler
        
    def registerEndOfStreamEventHandler(self, endOfStreamEventHandler):
        self.endOfStreamEventHandler = endOfStreamEventHandler
        
    def needsDeviceConnection(self):
        return False;
    
    def getConnectedOutputChannels(self):
        return []
    
    def connectNow(self, outputChannelInfo):
        pass
    
    def canCompute(self):
        return True
    
    def update(self):
        pass
    
    def setEndOfStream(self):
        pass
    
    def _triggerIOEvent(self):
        if self.ioEventHandler is not None:
            self.ioEventHandler(self)
            
    def _triggerEndOfStreamEvent(self):
        if self.endOfStreamEventHandler is not None:
            self.endOfStreamEventHandler()
            
            
@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("Output", "1.0")
class Output(object):
    
    def __init__(self, configuration):
        super(Output, self).__init__()
        self.appendModeEnabled = configuration.get("enableAppendMode")
        self.instanceId = None
        self.ioEventHandler = None
        
    @staticmethod
    def expectedParameters(selfexpected):
        (
        BOOL_ELEMENT(expected).key("enableAppendMode")
                .description("NOTE: Has no effect on Output-Network. If set to true a different internal structure is used, which buffers consecutive "
                             "calls to write(). The update() function must then be called to trigger final outputting "
                             "of the accumulated sequence of data.")
                .displayedName("Enable append mode")
                .init()
                .assignmentOptional().defaultValue(false)
                .commit()
                ,
        )
        
    @abstractmethod
    def write(self, o):
        pass
        
    def setInstanceId(self, id):
        self.instanceId = id
        
    def getInstanceId(self):
        return self.instanceId
    
    def registerIOEventHandler(self, handler):
        self.ioEventHandler = handler
        
    def getInformation(self):
        return Hash()
    
    def update(self):
        pass
    
    def signalEndOfStream(self):
        pass
    
    def canCompute(self):
        return True
    
    def _triggerIOEvent(self):
        if self.ioEventHandler is not None:
            self.ioEventHandler(self)
    
    
@KARABO_CLASSINFO("TextFileInput", "1.0")
class TextFileInput(Input):
    pass


@KARABO_CLASSINFO("BinaryFileInput", "1.0")
class BinaryFileInput(Input):
    pass


@KARABO_CLASSINFO("Hdf5FileInput", "1.0")
class Hdf5FileInput(Input):
    pass


@KARABO_CLASSINFO("TextFileOutput", "1.0")
class TextFileOutput(Output):
    pass


@KARABO_CLASSINFO("BinaryFileOutput", "1.0")
class BinaryFileOutput(Output):
    pass


@KARABO_CLASSINFO("Hdf5FileOutput", "1.0")
class Hdf5FileOutput(Output):
    pass


