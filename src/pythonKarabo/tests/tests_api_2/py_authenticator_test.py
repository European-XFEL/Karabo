# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest
from karabo.authenticator import Authenticator

from datetime import datetime

class  Authenticator_TestCase(unittest.TestCase):
    username = 'unitaryTests'
    provider = 'LOCAL'
    brokerHostname = '127.0.0.1'
    brokerPortNumber = 4444
    brokerTopic = 'topic'
    software = 'Karabo'
    ipAddress = 'PythonUnitTestsIpAddress' + datetime.utcnow().isoformat()


    def test_py_authenticator_correct_login(self):
        password = 'karaboUnitaryTestsPass'

        # Helper variables
        functionName = 'test_authenticator_correct_login'

        a = Authenticator(self.username, password, self.provider,
                          self.ipAddress, self.brokerHostname,
                          self.brokerPortNumber, self.brokerTopic)

        # Variables that should be correctly assigned
        self.assertEqual(a.username, self.username)
        self.assertEqual(a.provider, self.provider)
        self.assertEqual(a.currentIpAddress, self.ipAddress)
        self.assertEqual(a.brokerHostname, self.brokerHostname)
        self.assertEqual(int(a.brokerPortNumber), self.brokerPortNumber)
        self.assertEqual(a.brokerTopic, self.brokerTopic)
        self.assertEqual(a.softwareDesc, self.software)


        # Variables that should be empty
        self.assertEqual(a.defaultAccessLevelDesc, "")
        self.assertEqual(a.welcomeMessage, "")
        self.assertEqual(a.sessionToken, "")
        #
        self.assertEqual(a.defaultAccessLevelId, -100)
        self.assertEqual(a.softwareId, -100)
        self.assertEqual(a.userId, -100)

        self.assertTrue(a.login())

        # Variables that should be correctly assigned
        self.assertEqual(a.username, self.username)
        self.assertEqual(a.provider, self.provider)
        self.assertEqual(a.currentIpAddress, self.ipAddress)
        self.assertEqual(a.brokerHostname, self.brokerHostname)
        self.assertEqual(int(a.brokerPortNumber), self.brokerPortNumber)
        self.assertEqual(a.brokerTopic, self.brokerTopic)
        self.assertEqual(a.softwareDesc, self.software)

        # Variables that should be empty
        self.assertEqual(a.defaultAccessLevelDesc, "USER")
        self.assertNotEqual(a.welcomeMessage, "")
        self.assertNotEqual(a.sessionToken, "")
        #
        self.assertEqual(a.defaultAccessLevelId, 1)
        self.assertEqual(a.softwareId, 1)
        self.assertEqual(a.userId, -99)

        # Execute Logout
        self.assertTrue(a.logout())

        # Variables that should be correctly assigned
        self.assertEqual(a.username, self.username, "username don't match")
        self.assertEqual(a.provider, self.provider, "provider don't match")
        self.assertEqual(a.currentIpAddress, self.ipAddress,
                         "ipAddress don't match")
        self.assertEqual(a.brokerHostname, self.brokerHostname,
                         "brokerHostname don't match")
        self.assertEqual(int(a.brokerPortNumber), self.brokerPortNumber,
                         "brokerPortNumber don't match")
        self.assertEqual(a.brokerTopic, self.brokerTopic,
                         "brokerTopic don't match")
        self.assertEqual(a.softwareDesc, self.software, "software don't match")


        # Variables that should be empty
        self.assertEqual(a.defaultAccessLevelDesc, "",
                         "defaultAccessLevelDesc is not Empty")
        self.assertEqual(a.welcomeMessage, "",
                         "welcomeMessage is not Empty")
        self.assertEqual(a.sessionToken, "", "sessionToken() is not Empty")
        #
        self.assertEqual(a.defaultAccessLevelId, -100,
                         "defaultAccessLevelId is not undefined")
        self.assertEqual(a.softwareId, -100, "softwareId is not undefined")
        self.assertEqual(a.userId, -100, "userId is not undifined")


    def test_py_authenticator_incorrect_login(self):
        password = 'karaboUnitaryTestsNotPass'

        # Helper variables
        functionName = 'test_authenticator_correct_login'

        a = Authenticator(self.username, password, self.provider,
                          self.ipAddress, self.brokerHostname,
                          self.brokerPortNumber, self.brokerTopic)

        # Variables that should be correctly assigned
        self.assertEqual(a.username, self.username)
        self.assertEqual(a.provider, self.provider)
        self.assertEqual(a.currentIpAddress, self.ipAddress)
        self.assertEqual(a.brokerHostname, self.brokerHostname)
        self.assertEqual(int(a.brokerPortNumber), self.brokerPortNumber)
        self.assertEqual(a.brokerTopic, self.brokerTopic)
        self.assertEqual(a.softwareDesc, self.software)


        # Variables that should be empty
        self.assertEqual(a.defaultAccessLevelDesc, "")
        self.assertEqual(a.welcomeMessage, "")
        self.assertEqual(a.sessionToken, "")
        #
        self.assertEqual(a.defaultAccessLevelId, -100)
        self.assertEqual(a.softwareId, -100)
        self.assertEqual(a.userId, -100)

        self.assertFalse(a.login())

        # Variables that should be correctly assigned
        self.assertEqual(a.username, self.username)
        self.assertEqual(a.provider, self.provider)
        self.assertEqual(a.currentIpAddress, self.ipAddress)
        self.assertEqual(a.brokerHostname, self.brokerHostname)
        self.assertEqual(int(a.brokerPortNumber), self.brokerPortNumber)
        self.assertEqual(a.brokerTopic, self.brokerTopic)
        self.assertEqual(a.softwareDesc, self.software)

        # Variables that should be empty
        self.assertEqual(a.defaultAccessLevelDesc, "")
        self.assertEqual(a.welcomeMessage, "")
        self.assertEqual(a.sessionToken, "")
        #
        self.assertEqual(a.defaultAccessLevelId, -100)
        self.assertEqual(a.softwareId, -100)
        self.assertEqual(a.userId, -100)

        # Execute Logout
        self.assertFalse(a.logout())

        # Variables that should be correctly assigned
        self.assertEqual(a.username, self.username, "username don't match")
        self.assertEqual(a.provider, self.provider, "provider don't match")
        self.assertEqual(a.currentIpAddress, self.ipAddress,
                         "ipAddress don't match")
        self.assertEqual(a.brokerHostname, self.brokerHostname,
                         "brokerHostname don't match")
        self.assertEqual(int(a.brokerPortNumber), self.brokerPortNumber,
                         "brokerPortNumber don't match")
        self.assertEqual(a.brokerTopic, self.brokerTopic,
                         "brokerTopic don't match")
        self.assertEqual(a.softwareDesc, self.software, "software don't match")


        # Variables that should be empty
        self.assertEqual(a.defaultAccessLevelDesc, "",
                         "defaultAccessLevelDesc is not Empty")
        self.assertEqual(a.welcomeMessage, "",
                         "welcomeMessage is not Empty")
        self.assertEqual(a.sessionToken, "", "sessionToken() is not Empty")
        #
        self.assertEqual(a.defaultAccessLevelId, -100,
                         "defaultAccessLevelId is not undefined")
        self.assertEqual(a.softwareId, -100, "softwareId is not undefined")
        self.assertEqual(a.userId, -100, "userId is not undifined")


if __name__ == '__main__':
    unittest.main()

