/*
 *
 * File:   Authenticator.cc
 * Author: <luis.maia@xfel.eu>
 *
 * Created on April 12, 2013, 4:31 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <karabo/log/Logger.hh>
#include "Authenticator.hh"
#include "soapAuthenticationPortBindingProxy.h"
#include "AuthenticationPortBinding.nsmap"
#include <signal.h>		/* defines SIGPIPE */

using namespace std;

namespace karabo {
    namespace webAuth {


        Authenticator::Authenticator(const std::string& username, const std::string& password, const std::string& provider,
                                     const std::string& ipAddress, const std::string& brokerHostname, const std::string& brokerPortNumber,
                                     const std::string& brokerTopic)
        : m_username(username),
        m_password(password),
        m_provider(provider),
        m_ipAddress(ipAddress),
        m_brokerHostname(brokerHostname),
        m_brokerPortNumber(brokerPortNumber),
        m_brokerTopic(brokerTopic),
        m_software(KARABO_SOFTWARE_DESC),
        m_service(new AuthenticationPortBindingProxy),
        // Variables initialized with defaults (otherwise primitive types get whatever arbitrary junk happened to be at that memory location previously)
        m_nonce(""),
        m_firstName(""),
        m_familyName(""),
        m_userId(KARABO_INVALID_ID),
        m_softwareId(KARABO_INVALID_ID),
        m_softwareDesc(""),
        m_defaultAccessLevelId(KARABO_INVALID_ID),
        m_defaultAccessLevelDesc(""),
        m_accessList(""),
        m_sessionToken(""),
        m_welcomeMessage("") {
            // TODO: Transform the String List into a karabo Hash
            karabo::util::Hash a;
            m_accessHash = a;
        }


        /*
         * Specific logical functions
         */
        bool Authenticator::login() {

            ns1__getUserNonceResponse nsUserNonceResp;
            ns1__loginResponse nsLoginResp;

            // Get the nonce that must be used in Authentication method
            nsUserNonceResp = getUserNonce();
            if (*(nsUserNonceResp.return_->operationSuccess) == 0) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Error: " << string(nsUserNonceResp.return_->operationResultMsg->c_str());
                return false;
            }

            // Store nonce generated to this user/provider/ipAddress
            setNonce(nsUserNonceResp.return_->sessionToken);

            // Try authenticate the user
            nsLoginResp = authenticate();
            if (*(nsLoginResp.return_->operationSuccess) == 0) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Error: " << string(nsLoginResp.return_->operationResultMsg->c_str());
                return false;
            } else {
                KARABO_LOG_FRAMEWORK_DEBUG << "Debug: The sessionToken is " << string(nsLoginResp.return_->sessionToken->c_str());
                // Populate object with session related information
                setFirstName(nsLoginResp.return_->firstName);
                setFamilyName(nsLoginResp.return_->familyName);
                //
                setSoftwareDesc(nsLoginResp.return_->softwareDesc);
                //
                setDefaultAccessLevelDesc(nsLoginResp.return_->defaultAccessLevelDesc);
                setAccessList(nsLoginResp.return_->accessList);
                //
                setSessionToken(nsLoginResp.return_->sessionToken);
                setWelcomeMessage(nsLoginResp.return_->welcomeMessage);

                // Validated that the pointer content exists (is not NULL)
                if (nsLoginResp.return_->userId) setUserId(*(nsLoginResp.return_->userId));
                if (nsLoginResp.return_->softwareId) setSoftwareId(*(nsLoginResp.return_->softwareId));
                if (nsLoginResp.return_->defaultAccessLevelId) setDefaultAccessLevelId(*(nsLoginResp.return_->defaultAccessLevelId));
                //setDefaultAccessLevelId(nsLoginResp.return_->defaultAccessLevelId);


                // Clear m_nonce value
                setNonce(NULL);

                return true;
            }
        }


        bool Authenticator::logout() {

            ns1__logout nsLogout;
            ns1__logoutResponse nsLogoutResp;

            //            const char end_point[] = "https://exflpcx18262:8181/XFELAuthWebService/Authentication_soap";
            //            const char soap_action[] = "#logout";

            nsLogout.username = &m_username;
            nsLogout.provider = &m_provider;
            nsLogout.sessionToken = &m_sessionToken;

            // If obtain successfully answer from Web Service the logic proceeds!
            if (m_service->logout(/*end_point, NULL,*/ &nsLogout, &nsLogoutResp) == SOAP_OK) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Debug: SOAP message is OK";
                if (*(nsLogoutResp.return_) == 0) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Error: Logout didn't succeed";
                    return false;
                } else {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Debug: Logout did succeed";
                    // Clean from object session related information
                    setFirstName(NULL);
                    setFamilyName(NULL);
                    setUserId(KARABO_INVALID_ID);
                    //
                    setSoftwareId(KARABO_INVALID_ID);
                    setSoftwareDesc(NULL);
                    //
                    setDefaultAccessLevelId(KARABO_INVALID_ID);
                    setDefaultAccessLevelDesc(NULL);
                    setAccessList(NULL);
                    //
                    setSessionToken(NULL);
                    setWelcomeMessage(NULL);
                }
            } else {
                throw KARABO_NETWORK_EXCEPTION("Error: Problem with SOAP message: " + soapMessageNotOk(m_service->soap));
            }

            return true; // If no problems happened it returns true
        }


        ns1__getUserNonceResponse Authenticator::getUserNonce() {
            ns1__getUserNonce nsUserNonce;
            ns1__getUserNonceResponse nsUserNonceResp;

            //const char end_point[] = "https://exflpcx18262:8181/XFELAuthWebService/Authentication";
            //const char soap_action[] = "http://server.xfelauthwebservice.xfel.eu/Authentication/loginRequest";

            //            soap_init(m_service->soap);
            //            if (soap_ssl_client_context(m_service->soap,
            //                                        SOAP_SSL_NO_AUTHENTICATION, NULL, NULL, NULL,
            //                                        NULL, NULL)) {
            //                soap_print_fault(m_service->soap, stderr);
            //                exit(1);
            //            }

            nsUserNonce.username = &m_username;
            nsUserNonce.provider = &m_provider;
            nsUserNonce.ipAddress = &m_ipAddress;

            // If obtain successfully answer from Web Service the logic proceeds!
            if (m_service->getUserNonce(&nsUserNonce, &nsUserNonceResp) == SOAP_OK) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Debug: SOAP message is OK";
                if (*(nsUserNonceResp.return_->operationSuccess) == 0) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Error: " << string(nsUserNonceResp.return_->operationResultMsg->c_str());
                } else {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Debug: The userNonce is " << string(nsUserNonceResp.return_->sessionToken->c_str());
                }
            } else {
                throw KARABO_NETWORK_EXCEPTION("Error: Problem with SOAP message: " + soapMessageNotOk(m_service->soap));
            }

            return nsUserNonceResp; // If no problems happened it returns new userNonceResponse information
        }


        ns1__loginResponse Authenticator::authenticate() {
            ns1__login nsLogin;
            ns1__loginResponse nsLoginResp;

            //const char end_point[] = "https://exflpcx18262:8181/XFELAuthWebService/Authentication";
            //const char soap_action[] = "http://server.xfelauthwebservice.xfel.eu/Authentication/loginRequest";

            //            soap_init(m_service->soap);
            //            if (soap_ssl_client_context(m_service->soap,
            //                                        SOAP_SSL_NO_AUTHENTICATION, NULL, NULL, NULL,
            //                                        NULL, NULL)) {
            //                soap_print_fault(m_service->soap, stderr);
            //                exit(1);
            //            }

            nsLogin.username = &m_username;
            nsLogin.password = &m_password;
            nsLogin.provider = &m_provider;
            nsLogin.ipAddress = &m_ipAddress;
            nsLogin.brokerHostname = &m_brokerHostname;
            nsLogin.brokerPortNumber = &m_brokerPortNumber;
            nsLogin.brokerTopic = &m_brokerTopic;
            nsLogin.nonce = &m_nonce;
            nsLogin.software = &m_software;

            // Convert received date to String to send to the WebServer
            //const karabo::util::Timestamp& timestamp
            //karabo::util::Timestamp contextTime = karabo::util::Timestamp(timestamp);
            //string contextTimeStr = contextTime.toFormattedString("%Y-%m-%d %H:%M:%S.%f");
            //nsLogin.time = &contextTimeStr;

            // If obtain successfully answer from Web Service it print message returned!
            if (m_service->login(&nsLogin, &nsLoginResp) == SOAP_OK) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Debug: SOAP message is OK";
                if (*(nsLoginResp.return_->operationSuccess) == 0) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Error: " << string(nsLoginResp.return_->operationResultMsg->c_str());
                } else {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Debug: The sessionToken is " << string(nsLoginResp.return_->sessionToken->c_str());
                }
            } else {
                throw KARABO_NETWORK_EXCEPTION("Error: Problem with SOAP message: " + soapMessageNotOk(m_service->soap));
            }

            return nsLoginResp; // If no problems happened it returns new loginResponse information
        }


        std::string Authenticator::getSingleSignOn(const std::string ipAddress) {
            ns1__singleSignOn nsSingleSignOn;
            ns1__singleSignOnResponse nsSingleSignOnResp;

            nsSingleSignOn.username = &m_username;
            nsSingleSignOn.provider = &m_provider;

            string idAddressStr = string(ipAddress);
            nsSingleSignOn.ipAddress = &idAddressStr;

            // If obtain successfully answer from Web Service it print message returned!
            if (m_service->singleSignOn(&nsSingleSignOn, &nsSingleSignOnResp) == SOAP_OK) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Debug: SOAP message is OK";
                if (*(nsSingleSignOnResp.return_->operationSuccess) == 0) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Error: " << string(nsSingleSignOnResp.return_->operationResultMsg->c_str());
                    return "";
                } else {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Debug: The sessionToken is " << string(nsSingleSignOnResp.return_->sessionToken->c_str());
                }
            } else {
                throw KARABO_NETWORK_EXCEPTION("Error: Problem with SOAP message: " + soapMessageNotOk(m_service->soap));
            }

            return string(nsSingleSignOnResp.return_->sessionToken->c_str()); // If no problems happened it returns SessionToken
        }


        /*
         * Auxiliar functions
         */
        void Authenticator::printObject(ns1__loginResponse nsLoginResp) {
            KARABO_LOG_FRAMEWORK_DEBUG << "Information received: \n";
            KARABO_LOG_FRAMEWORK_DEBUG << "firstName: " << string(nsLoginResp.return_->firstName->c_str()) << "\n";
            KARABO_LOG_FRAMEWORK_DEBUG << "familyName: " << string(nsLoginResp.return_->familyName->c_str()) << "\n";
            KARABO_LOG_FRAMEWORK_DEBUG << "username: " << string(nsLoginResp.return_->username->c_str()) << "\n";
            KARABO_LOG_FRAMEWORK_DEBUG << "provider: " << string(nsLoginResp.return_->provider->c_str()) << "\n";
            KARABO_LOG_FRAMEWORK_DEBUG << "defaultAccessLevelDesc: " << string(nsLoginResp.return_->defaultAccessLevelDesc->c_str()) << "\n";
            KARABO_LOG_FRAMEWORK_DEBUG << "accessList: " << string(nsLoginResp.return_->accessList->c_str()) << "\n";
            KARABO_LOG_FRAMEWORK_DEBUG << "softwareDesc: " << string(nsLoginResp.return_->softwareDesc->c_str()) << "\n";
            KARABO_LOG_FRAMEWORK_DEBUG << "sessionToken: " << string(nsLoginResp.return_->sessionToken->c_str()) << "\n";
            KARABO_LOG_FRAMEWORK_DEBUG << "welcomeMessage: " << string(nsLoginResp.return_->welcomeMessage->c_str()) << "\n";
            if (*(nsLoginResp.return_->operationSuccess) == 0) {
                KARABO_LOG_FRAMEWORK_DEBUG << "operationSuccess: No\n";
            } else {
                KARABO_LOG_FRAMEWORK_DEBUG << "operationSuccess: Yes\n";
            }
        }


        std::string Authenticator::soapMessageNotOk(struct soap *soap) {
            string errorMsg;

            if (soap_check_state(soap))
                errorMsg = "Error: soap struct state not initialized\n";
            else if (soap->error) {
                const char **c, *v = NULL, *s, *d;
                c = soap_faultcode(soap);
                if (!*c)
                    soap_set_fault(soap);
                if (soap->version == 2)
                    v = soap_check_faultsubcode(soap);
                s = *soap_faultstring(soap);
                d = soap_check_faultdetail(soap);

                errorMsg = soap->version ? "SOAP 1." : "Error ";
                errorMsg = errorMsg + boost::lexical_cast<std::string>(soap->version ? (int) soap->version : soap->error);
                errorMsg = errorMsg + " fault: ";
                errorMsg = errorMsg + *c;
                errorMsg = errorMsg + " [";
                errorMsg = errorMsg + boost::lexical_cast<std::string>(v ? v : "no subcode");
                errorMsg = errorMsg + " ]\n\"";
                errorMsg = errorMsg + boost::lexical_cast<std::string>(s ? s : "[no reason]");
                errorMsg = errorMsg + "\"\nDetail: ";
                errorMsg = errorMsg + boost::lexical_cast<std::string>(d ? d : "[no detail]");
                errorMsg = errorMsg + "\n";
            }
            return errorMsg;
        }


        /*
         * Getters
         */
        const karabo::util::Hash& Authenticator::getAccessHash() const {
            return m_accessHash;
        }


        const std::string& Authenticator::getAccessList() const {
            return m_accessList;
        }


        const std::string& Authenticator::getBrokerHostname() const {
            return m_brokerHostname;
        }


        const std::string& Authenticator::getBrokerPortNumber() const {
            return m_brokerPortNumber;
        }


        const std::string& Authenticator::getBrokerTopic() const {
            return m_brokerTopic;
        }


        const std::string& Authenticator::getDefaultAccessLevelDesc() const {
            return m_defaultAccessLevelDesc;
        }


        const int Authenticator::getDefaultAccessLevelId() const {
            return m_defaultAccessLevelId;
        }


        const std::string& Authenticator::getFamilyName() const {
            return m_familyName;
        }


        const std::string& Authenticator::getFirstName() const {
            return m_firstName;
        }


        const std::string& Authenticator::getIpAddress() const {
            return m_ipAddress;
        }


        const std::string& Authenticator::getNonce() const {
            return m_nonce;
        }


        const std::string& Authenticator::getPassword() const {
            return m_password;
        }


        const std::string& Authenticator::getProvider() const {
            return m_provider;
        }


        const boost::shared_ptr<AuthenticationPortBindingProxy> Authenticator::getService() const {
            return m_service;
        }


        const std::string& Authenticator::getSessionToken() const {
            return m_sessionToken;
        }


        const std::string& Authenticator::getSoftware() const {
            return m_software;
        }


        const std::string& Authenticator::getSoftwareDesc() const {
            return m_softwareDesc;
        }


        const long long Authenticator::getSoftwareId() const {
            return m_softwareId;
        }


        const long long Authenticator::getUserId() const {
            return m_userId;
        }


        const std::string& Authenticator::getUsername() const {
            return m_username;
        }


        const std::string& Authenticator::getWelcomeMessage() const {
            return m_welcomeMessage;
        }


        /*
         * Setters
         */
        void Authenticator::setAccessList(const std::string* accessList) {
            if (accessList)
                m_accessList = *accessList;
            else
                m_accessList = "";
        }


        void Authenticator::setDefaultAccessLevelDesc(const std::string* defaultAccessLevelDesc) {
            if (defaultAccessLevelDesc)
                m_defaultAccessLevelDesc = *defaultAccessLevelDesc;
            else
                m_defaultAccessLevelDesc = "";
        }


        void Authenticator::setDefaultAccessLevelId(const int defaultAccessLevelId) {
            if (defaultAccessLevelId)
                m_defaultAccessLevelId = defaultAccessLevelId;
            else
                m_defaultAccessLevelId = KARABO_INVALID_ID;
        }


        void Authenticator::setFamilyName(const std::string* familyName) {
            if (familyName)
                m_familyName = *familyName;
            else
                m_familyName = "";
        }


        void Authenticator::setFirstName(const std::string* firstName) {
            if (firstName)
                m_firstName = *firstName;
            else
                m_firstName = "";
        }


        void Authenticator::setNonce(const std::string* nonce) {
            if (nonce)
                m_nonce = *nonce;
            else
                m_nonce = "";
        }


        void Authenticator::setSessionToken(const std::string* sessionToken) {
            if (sessionToken)
                m_sessionToken = *sessionToken;
            else
                m_sessionToken = "";
        }


        void Authenticator::setSoftwareDesc(const std::string* softwareDesc) {
            if (softwareDesc)
                m_softwareDesc = *softwareDesc;
            else
                m_softwareDesc = "";
        }


        void Authenticator::setSoftwareId(const long long softwareId) {
            if (softwareId)
                m_softwareId = softwareId;
            else
                m_softwareId = KARABO_INVALID_ID;
        }


        void Authenticator::setUserId(const long long userId) {
            if (userId)
                m_userId = userId;
            else
                m_userId = KARABO_INVALID_ID;
        }


        void Authenticator::setWelcomeMessage(const std::string* welcomeMessage) {
            if (welcomeMessage)
                m_welcomeMessage = *welcomeMessage;
            else
                m_welcomeMessage = "";
        }


        /******************************************************************************\
         *
         *	SIGPIPE
         *
        \******************************************************************************/

        void sigpipe_handle(int x) {
        }

    } // namespace packageName
} // namespace karabo
