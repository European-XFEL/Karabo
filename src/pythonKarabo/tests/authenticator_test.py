# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest
from libkarathon import *
import time

class  Authenticator_TestCase(unittest.TestCase):
    #def setUp(self):
    #    self.foo = Authenticator_()
    #

    #def tearDown(self):
    #    self.foo.dispose()
    #    self.foo = None

    def test_authenticator_correct_login(self):
        
        # Variables definition
        username = "guest";
        password = "guest";
        provider = "LOCAL";
        ipAddress = "PythonUnitTestsIpAddress";
        hostname = "127.0.0.1";
        portNumber = "44444";
        software = "Karabo";
        #
        timeStr = "20130120T122059.259188123";
        #karabo::util::Timestamp time = karabo::util::Timestamp(timeStr);        
        
        # Helper variables
        emptyString = ""
        functionName = "test_authenticator_correct_login"
        
        # Check properties of empty Authenticator
        try:
            a = Authenticator(username, password, provider, ipAddress, hostname, portNumber, software)
            
            # Variables that should be correctly assigned
            self.assertEqual(a.getUsername(), username, "getUsername() don't match")
            self.assertEqual(a.getProvider(), provider, "getProvider() don't match")
            self.assertEqual(a.getIpAddress(), ipAddress, "getIpAddress() don't match")
            self.assertEqual(a.getHostname(), hostname, "getHostname() don't match")
            self.assertEqual(a.getPortNumber(), portNumber, "getPortNumber() don't match")
            self.assertEqual(a.getSoftware(), software, "getSoftware() don't match")
            
            # Variables that should be empty
            self.assertEqual(a.getRoleDesc(), emptyString, "getRoleDesc() is not Empty")
            self.assertEqual(a.getWelcomeMessage(), emptyString, "getWelcomeMessage() is not Empty")
            self.assertEqual(a.getSessionToken(), emptyString, "getSessionToken() is not Empty")
            #
            self.assertEqual(a.getRoleId(), -100, "getRoleId() is not undifined")
            self.assertEqual(a.getSoftwareId(), -100, "getSoftwareId() is not undifined")
            self.assertEqual(a.getUserId(), -100, "getUserId() is not undifined")
        except Exception, e:
            self.fail(functionName + " creation exception before LOGIN: " + str(e))
        
        # Execute Login
        try:
            self.assertTrue(a.login())
            
            # Variables that should be correctly assigned
            self.assertEqual(a.getUsername(), username, "getUsername() don't match")
            self.assertEqual(a.getProvider(), provider, "getProvider() don't match")
            self.assertEqual(a.getIpAddress(), ipAddress, "getIpAddress() don't match")
            self.assertEqual(a.getHostname(), hostname, "getHostname() don't match")
            self.assertEqual(a.getPortNumber(), portNumber, "getPortNumber() don't match")
            self.assertEqual(a.getSoftware(), software, "getSoftware() don't match")
            
            # Variables that should be empty
            self.assertEqual(a.getRoleDesc(), "GUEST_USER")
            self.assertNotEqual(a.getWelcomeMessage(), emptyString, "getWelcomeMessage() is Empty")
            self.assertNotEqual(a.getSessionToken(), emptyString, "getSessionToken() is Empty")
            #
            self.assertEqual(a.getRoleId(), 3, "getRoleId() is not undifined")
            self.assertEqual(a.getSoftwareId(), 1, "getSoftwareId() is not undifined")
            self.assertEqual(a.getUserId(), -1, "getUserId() is not undifined")
        except Exception, e:
            self.fail(functionName + " exception after LOGIN: " + str(e))
        
        # Execute Logout
        try:
            self.assertTrue(a.logout())
            
            # Variables that should be correctly assigned
            self.assertEqual(a.getUsername(), username, "getUsername() don't match")
            self.assertEqual(a.getProvider(), provider, "getProvider() don't match")
            self.assertEqual(a.getIpAddress(), ipAddress, "getIpAddress() don't match")
            self.assertEqual(a.getHostname(), hostname, "getHostname() don't match")
            self.assertEqual(a.getPortNumber(), portNumber, "getPortNumber() don't match")
            self.assertEqual(a.getSoftware(), software, "getSoftware() don't match")
            
            # Variables that should be empty
            self.assertEqual(a.getRoleDesc(), emptyString, "getRoleDesc() is not Empty")
            self.assertEqual(a.getWelcomeMessage(), emptyString, "getWelcomeMessage() is not Empty")
            self.assertEqual(a.getSessionToken(), emptyString, "getSessionToken() is not Empty")
            #
            self.assertEqual(a.getRoleId(), -100, "getRoleId() is not undifined")
            self.assertEqual(a.getSoftwareId(), -100, "getSoftwareId() is not undifined")
            self.assertEqual(a.getUserId(), -100, "getUserId() is not undifined")
        except Exception, e:
            self.fail(functionName + " exception after LOGOUT: " + str(e))


    def test_authenticator_incorrect_login(self):
        
        # Variables definition
        username = "guest";
        password = "guest2";
        provider = "LOCAL";
        ipAddress = "PythonUnitTestsIpAddress";
        hostname = "127.0.0.1";
        portNumber = "4444";
        software = "Karabo";
        #
        timeStr = "20130120T122059.259188123";
        #karabo::util::Timestamp time = karabo::util::Timestamp(timeStr);        
        
        # Helper variables
        emptyString = ""
        functionName = "test_authenticator_incorrect_login"
        
        # Check properties of empty Authenticator
        try:
            a = Authenticator(username, password, provider, ipAddress, hostname, portNumber, software)
            
            # Variables that should be correctly assigned
            self.assertEqual(a.getUsername(), username, "getUsername() don't match")
            self.assertEqual(a.getProvider(), provider, "getProvider() don't match")
            self.assertEqual(a.getIpAddress(), ipAddress, "getIpAddress() don't match")
            self.assertEqual(a.getHostname(), hostname, "getHostname() don't match")
            self.assertEqual(a.getPortNumber(), portNumber, "getPortNumber() don't match")
            self.assertEqual(a.getSoftware(), software, "getSoftware() don't match")
            
            # Variables that should be empty
            self.assertEqual(a.getRoleDesc(), emptyString, "getRoleDesc() is not Empty")
            self.assertEqual(a.getWelcomeMessage(), emptyString, "getWelcomeMessage() is not Empty")
            self.assertEqual(a.getSessionToken(), emptyString, "getSessionToken() is not Empty")
            #
            self.assertEqual(a.getRoleId(), -100, "getRoleId() is not undifined")
            self.assertEqual(a.getSoftwareId(), -100, "getSoftwareId() is not undifined")
            self.assertEqual(a.getUserId(), -100, "getUserId() is not undifined")
        except Exception, e:
            self.fail(functionName + " creation exception before LOGIN: " + str(e))
        
        # Execute Login
        try:
            self.assertFalse(a.login())
            
            # Variables that should be correctly assigned
            self.assertEqual(a.getUsername(), username, "getUsername() don't match")
            self.assertEqual(a.getProvider(), provider, "getProvider() don't match")
            self.assertEqual(a.getIpAddress(), ipAddress, "getIpAddress() don't match")
            self.assertEqual(a.getHostname(), hostname, "getHostname() don't match")
            self.assertEqual(a.getPortNumber(), portNumber, "getPortNumber() don't match")
            self.assertEqual(a.getSoftware(), software, "getSoftware() don't match")
            
            # Variables that should be empty
            self.assertEqual(a.getRoleDesc(), emptyString, "getRoleDesc() is not Empty")
            self.assertEqual(a.getWelcomeMessage(), emptyString, "getWelcomeMessage() is not Empty")
            self.assertEqual(a.getSessionToken(), emptyString, "getSessionToken() is not Empty")
            #
            self.assertEqual(a.getRoleId(), -100, "getRoleId() is not undifined")
            self.assertEqual(a.getSoftwareId(), -100, "getSoftwareId() is not undifined")
            self.assertEqual(a.getUserId(), -100, "getUserId() is not undifined")
        except Exception, e:
            self.fail(functionName + " exception after LOGIN: " + str(e))


    def test_authenticator_single_sign_on(self):
        
        # Variables definition
        username = "guest";
        password = "guest";
        provider = "LOCAL";
        ipAddress = "PythonUnitTestsIpAddress";
        ipAddressWrong = "PythonUnitTestsIpAddressXXXXXXXXXXX";
        hostname = "127.0.0.1";
        portNumber = "4444";
        software = "Karabo";
        #
        timeStr = "20130120T122059.259188123";
        #karabo::util::Timestamp time = karabo::util::Timestamp(timeStr);        
        
        # Helper variables
        emptyString = ""
        functionName = "test_authenticator_single_sign_on"
        
        # Check properties of empty Authenticator (already validated in previous functions)
        try:
            a = Authenticator(username, password, provider, ipAddress, hostname, portNumber, software)
        except Exception, e:
            self.fail(functionName + " creation exception before LOGIN: " + str(e))
        
        # Execute Login (already validated in previous functions)
        try:
            self.assertTrue(a.login())
        except Exception, e:
            self.fail(functionName + " exception on LOGIN: " + str(e))

        
        # Validate session with SAME machine name (from where I'm logged in) => Should Return
        try:
            self.assertNotEqual(a.getSingleSignOn(ipAddress), "", "This machine should return information")
        except Exception, e:
            self.fail(functionName + " exception on SINGLE SIGN ON (with correct machine name): " + str(e))
        
        # Validate session with DIFFERENT machine name (from where I'm logged in) => Should NOT Return
        try:
            self.assertEqual(a.getSingleSignOn(ipAddressWrong), "", "This machine should return information")
        except Exception, e:
            self.fail(functionName + " exception on SINGLE SIGN ON (with wrong machine name): " + str(e))
        
        # Execute Logout (already validated in previous functions)
        try:
            self.assertTrue(a.logout())
        except Exception, e:
            self.fail(functionName + " exception on LOGOUT: " + str(e))
  
        # Validate session with SAME machine name (from where I just logged out) => Should NOT Return (because user made Logout)
        try:
            self.assertEqual(a.getSingleSignOn(ipAddressWrong), "", "This machine should return information")
        except Exception, e:
            self.fail(functionName + " exception on SINGLE SIGN ON (with wrong machine name): " + str(e))


if __name__ == '__main__':
    unittest.main()

