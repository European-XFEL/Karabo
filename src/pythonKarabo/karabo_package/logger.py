
from karabo.decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from karabo.enums import Assignment
from karabo.hashtypes import String
from karabo.schema import Configurable


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO('Logger', '1.0')
class Logger(Configurable):
    priority = String(description="Default Priority",
                      displayedName="Priority",
                      options="DEBUG INFO WARN ERROR",
                      assignment=Assignment.OPTIONAL, defaultValue="INFO")

    @staticmethod
    def configure(config):
        pass

    @staticmethod
    def getLogger(id):
        return Logger()

    def INFO(self, what):
        print('INFO:', what)

    def ERROR(self, what):
        print('ERROR:', what)

    def DEBUG(self, what):
        print('DEBUG:', what)

    def WARN(self, what):
        print('WARN:', what)
