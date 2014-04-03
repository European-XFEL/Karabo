__author__="luis.maia@xfel.eu"
__date__ ="April 2, 2014"

from suds.client import Client

class PyAuthenticator:
    
    url = 'https://exfl-tb04.desy.de:8181/XFELWebAuth/Authentication?WSDL'
    client = Client(url)
    
    #
    
    def __init__(self, username, password, provider, currentIpAddress, brokerHostname, brokerPortNumber, brokerTopic):
        self.username = username
        self.password = password
        self.provider = provider
        self.currentIpAddress = currentIpAddress
        self.brokerHostname = brokerHostname
        self.brokerPortNumber = str(brokerPortNumber)
        self.brokerTopic = brokerTopic
        self.softwareDesc = 'karabo'
        
        self.update_instance(None)
        
        
    #**************************************************************
    #*                        Login                               *
    #**************************************************************
    def login(self):
        webService = PyAuthenticator.client.service
        
        getUserNonceResponse = webService.getUserNonce(username=self.username, provider=self.provider, ipAddress=self.currentIpAddress)
        userNonceToken = getUserNonceResponse.sessionToken
        
        loginResponse = webService.login(username=self.username, password=self.password, provider=self.provider, 
        ipAddress=self.currentIpAddress, brokerHostname=self.brokerHostname, brokerPortNumber=self.brokerPortNumber, 
        brokerTopic=self.brokerTopic, nonce=userNonceToken, software=self.softwareDesc)
        
        self.update_instance(loginResponse)
        
        return bool(self.operationSuccess)
    
    
    #**************************************************************
    #*                        Login                               *
    #**************************************************************
    def logout(self):
        webService = PyAuthenticator.client.service
        
        logoutResponse = webService.logout(username=self.username, provider=self.provider, sessionToken=self.sessionToken)
        
        self.update_instance(logoutResponse)
        
        return logoutResponse
        
        
    def update_instance(self, resultHash):
        self.accessList = getattr(resultHash, 'accessList', '')
        self.defaultAccessLevelDesc = getattr(resultHash, 'defaultAccessLevelDesc', '')
        self.defaultAccessLevelId = getattr(resultHash, 'defaultAccessLevelId', '-100')
        self.operationResultMsg = getattr(resultHash, 'operationResultMsg', '')
        self.operationSuccess = getattr(resultHash, 'operationSuccess', '')
        self.sessionToken = getattr(resultHash, 'sessionToken', '')
        self.softwareId = getattr(resultHash, 'softwareId', '-100')
        self.userId = getattr(resultHash, 'userId', '-100')
        self.welcomeMessage = getattr(resultHash, 'welcomeMessage', '')


    def getUsername(self):
        return self.username

    def getProvider(self):
        return self.provider

    def getIpAddress(self):
        return self.currentIpAddress

    def getBrokerHostname(self):
        return self.brokerHostname

    def getBrokerPortNumber(self):
        return self.brokerPortNumber

    def getBrokerTopic(self):
        return self.brokerTopic

    def getSoftware(self):
        return self.softwareDesc


    # Additional variables
    def getDefaultAccessLevelDesc(self):
        return self.defaultAccessLevelDesc
    
    def getWelcomeMessage(self):
        return self.welcomeMessage
    
    def getSessionToken(self):
        return self.sessionToken
    
    def getDefaultAccessLevelId(self):
        return self.defaultAccessLevelId
    
    def getSoftwareId(self):
        return self.softwareId
    
    def getUserId(self):
        return self.userId
        
    
    #accessList
    #operationResultMsg
    #operationSuccess
    