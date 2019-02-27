from karabo.bound import PythonDevice, KARABO_CLASSINFO


@KARABO_CLASSINFO("ConfigurationExampleDevice", "2.0")
class ConfigurationExampleDevice(PythonDevice):

    @staticmethod
    def expectedParameters(expected):
        """ This static method is needed as a part of the factory/configuration
        system.
        This is intentionally left blank, see
        schema_test.Schema_TestCase.test_schemaInjection
        """

    def __init__(self, configuration):
        super(ConfigurationExampleDevice, self).__init__(configuration)
        self.registerInitialFunction(self.initialization)

    def initialization(self):
        """ This method will be called after the constructor.
        This is intentionally left blank, see
        schema_test.Schema_TestCase.test_schemaInjection
        """
