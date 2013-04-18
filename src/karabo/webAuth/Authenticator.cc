/* 
 * File:   Authenticator.cc
 * Author: <luis.maia@xfel.eu>
 * 
 * Created on April 12, 2013, 4:31 PM
 */

#include <karabo/log/Logger.hh>
#include "Authenticator.hh"
#include "Authenticator.hh"
#include "soapAuthenticationPortBindingProxy.h"
#include "AuthenticationPortBinding.nsmap"

using namespace std;

namespace karabo {
    namespace webAuth {

        Authenticator::Authenticator(const std::string& username, const std::string& password, const std::string& provider,
                const std::string& ipAddress, const std::string& hostname, const std::string& portNumber,
                const std::string& software)
        : m_username(username), m_password(password), m_provider(provider), m_ipAddress(ipAddress), m_hostname(hostname), m_portNumber(portNumber), m_software(software), m_service(new AuthenticationPortBindingProxy) {
        }

        bool Authenticator::login(const karabo::util::Timestamp& timestamp) {

            ns1__getUserNonceResponse nsUserNonceResp;
            ns1__loginResponse nsLoginResp;

            // Get the nonce that must be used in Authentication method
            nsUserNonceResp = getUserNonce();
            if (*(nsUserNonceResp.return_->operationSuccess) == 0) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Error: " << string(nsLoginResp.return_->errorMsg->c_str());
                return false;
            }

            // Store nonce generated to this user/domain/ipAddress
            m_nonce = nsUserNonceResp.return_->sessionToken->c_str();

            // Try authenticate the user
            nsLoginResp = authenticate(timestamp);
            if (*(nsLoginResp.return_->operationSuccess) == 0) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Error: " << string(nsLoginResp.return_->errorMsg->c_str());
                return false;
            } else {
                setSessionToken(*(nsLoginResp.return_->sessionToken));
                KARABO_LOG_FRAMEWORK_DEBUG << "Debug: The sessionToken is " << string(nsLoginResp.return_->sessionToken->c_str());
                return true;
            }
        }

        bool Authenticator::logout() {

            ns1__logout nsLogout;
            ns1__logoutResponse nsLogoutResp;

            nsLogout.username = &m_username;
            nsLogout.provider = &m_provider;
            nsLogout.sessionToken = &m_sessionToken;

            // If obtain successfully answer from Web Service the logic proceeds!
            if (m_service->logout(&nsLogout, &nsLogoutResp) == SOAP_OK) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Debug: SOAP message is OK";
                if (*(nsLogoutResp.return_) == 0) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Error: Logout didn't succeed";
                    return false;
                } else {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Debug: Logout did succeed";
                }
            } else {
                throw KARABO_NETWORK_EXCEPTION("Error: Problem with SOAP message: " + soapMessageNotOk(m_service->soap));
            }

            return true; // If no problems happened it returns true
        }

        ns1__getUserNonceResponse Authenticator::getUserNonce() {
            ns1__getUserNonce nsUserNonce;
            ns1__getUserNonceResponse nsUserNonceResp;

            nsUserNonce.username = &m_username;
            nsUserNonce.provider = &m_provider;
            nsUserNonce.ipAddress = &m_ipAddress;

            // If obtain successfully answer from Web Service the logic proceeds!
            if (m_service->getUserNonce(&nsUserNonce, &nsUserNonceResp) == SOAP_OK) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Debug: SOAP message is OK";
                if (*(nsUserNonceResp.return_->operationSuccess) == 0) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Error: " << string(nsUserNonceResp.return_->errorMsg->c_str());
                } else {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Debug: The userNonce is " << string(nsUserNonceResp.return_->sessionToken->c_str());
                }
            } else {
                throw KARABO_NETWORK_EXCEPTION("Error: Problem with SOAP message: " + soapMessageNotOk(m_service->soap));
            }

            return nsUserNonceResp; // If no problems happened it returns new userNonceResponse information
        }

        ns1__loginResponse Authenticator::authenticate(const karabo::util::Timestamp& timestamp) {
            ns1__login nsLogin;
            ns1__loginResponse nsLoginResp;

            nsLogin.username = &m_username;
            nsLogin.password = &m_password;
            nsLogin.provider = &m_provider;
            nsLogin.ipAddress = &m_ipAddress;
            nsLogin.hostname = &m_hostname;
            nsLogin.portNumber = &m_portNumber;
            nsLogin.nonce = &m_nonce;
            nsLogin.software = &m_software;

            // TODO implement timestamp.getDBString())
            string time = "20130410145159257";
            nsLogin.time = &time;

            // If obtain successfully answer from Web Service it print message returned!
            if (m_service->login(&nsLogin, &nsLoginResp) == SOAP_OK) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Debug: SOAP message is OK";
                if (*(nsLoginResp.return_->operationSuccess) == 0) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Error: " << string(nsLoginResp.return_->errorMsg->c_str());
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
            
            std::string xpto = string(ipAddress);
            nsSingleSignOn.ipAddress = &xpto;
            //nsSingleSignOn.ipAddress = &m_ipAddress;

            // If obtain successfully answer from Web Service it print message returned!
            if (m_service->singleSignOn(&nsSingleSignOn, &nsSingleSignOnResp) == SOAP_OK) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Debug: SOAP message is OK";
                if (*(nsSingleSignOnResp.return_->operationSuccess) == 0) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Error: " << string(nsSingleSignOnResp.return_->errorMsg->c_str());
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
        void Authenticator::setSessionToken(const std::string& newSessionToken) {
            m_sessionToken = newSessionToken;
        }

        void Authenticator::printObject(ns1__loginResponse nsLoginResp) {
            KARABO_LOG_FRAMEWORK_DEBUG << "Information received: \n";
            KARABO_LOG_FRAMEWORK_DEBUG << "firstName: " << string(nsLoginResp.return_->firstName->c_str()) << "\n";
            KARABO_LOG_FRAMEWORK_DEBUG << "familyName: " << string(nsLoginResp.return_->familyName->c_str()) << "\n";
            KARABO_LOG_FRAMEWORK_DEBUG << "username: " << string(nsLoginResp.return_->username->c_str()) << "\n";
            KARABO_LOG_FRAMEWORK_DEBUG << "provider: " << string(nsLoginResp.return_->provider->c_str()) << "\n";
            KARABO_LOG_FRAMEWORK_DEBUG << "roleDesc: " << string(nsLoginResp.return_->roleDesc->c_str()) << "\n";
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
            std::string errorMsg;

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
                //errorMsg = errorMsg + std::string(soap->version ? (int) soap->version : soap->error);
                errorMsg = errorMsg + " fault: ";
                errorMsg = errorMsg + *c;
                errorMsg = errorMsg + " [";
                //errorMsg = errorMsg + v ? v : "no subcode";
                errorMsg = errorMsg + " ]\n\"";
                //errorMsg = errorMsg + s ? s : "[no reason]";
                errorMsg = errorMsg + "\"\nDetail: ";
                //errorMsg = errorMsg + d ? d : "[no detail]";
                errorMsg = errorMsg + "\n";
            }
            return errorMsg;
        }

    }
}


