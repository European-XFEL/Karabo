__author__="luis.maia@xfel.eu"
__date__ ="April 2, 2014"

from suds.client import Client

class Authenticator(object):
    url = 'https://exfl-tb04.desy.de:8181/XFELWebAuth/Authentication?WSDL'
    softwareDesc = 'Karabo'


    def __init__(self, username, password, provider, currentIpAddress,
                 brokerHostname, brokerPortNumber, brokerTopic):
        self.client = Client(self.url)
        self.username = username
        self.password = password
        self.provider = provider
        self.currentIpAddress = currentIpAddress
        self.brokerHostname = brokerHostname
        self.brokerPortNumber = str(brokerPortNumber)
        self.brokerTopic = brokerTopic

        self.update_instance(None)


    def login(self):
        webService = self.client.service

        getUserNonceResponse = webService.getUserNonce(
            username=self.username, provider=self.provider,
            ipAddress=self.currentIpAddress)
        userNonceToken = getUserNonceResponse.sessionToken

        loginResponse = webService.login(
            username=self.username, password=self.password,
            provider=self.provider, ipAddress=self.currentIpAddress,
            brokerHostname=self.brokerHostname,
            brokerPortNumber=self.brokerPortNumber,
            brokerTopic=self.brokerTopic,
            nonce=userNonceToken, software=self.softwareDesc)

        self.update_instance(loginResponse)

        return bool(self.operationSuccess)


    def logout(self):
        webService = self.client.service

        logoutResponse = webService.logout(
            username=self.username, provider=self.provider,
            sessionToken=self.sessionToken)

        self.update_instance(logoutResponse)
        return logoutResponse


    def update_instance(self, r):
        self.accessList = getattr(r, 'accessList', '')
        self.defaultAccessLevelDesc = getattr(r, 'defaultAccessLevelDesc', '')
        self.defaultAccessLevelId = getattr(r, 'defaultAccessLevelId', -100)
        self.operationResultMsg = getattr(r, 'operationResultMsg', '')
        self.operationSuccess = getattr(r, 'operationSuccess', '')
        self.sessionToken = getattr(r, 'sessionToken', '')
        self.softwareId = getattr(r, 'softwareId', -100)
        self.userId = getattr(r, 'userId', -100)
        self.welcomeMessage = getattr(r, 'welcomeMessage', '')
