/* 
 * File:   Authenticator.cc
 * Author: <luis.maia@xfel.eu>
 * 
 * Created on April 12, 2013, 4:31 PM
 */

#include "Authenticator.hh"
#include "Authenticator.hh"
#include <karabo/log/Logger.hh>
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

            nsUserNonceResp = getUserNonce();
            if (*(nsUserNonceResp.return_->operationSuccess) == 0) {
                //cout << "Nonce is shit" << endl;
                return false;
            }

            m_nonce = nsUserNonceResp.return_->sessionToken->c_str();

            nsLoginResp = authenticate(timestamp);
            if (*(nsLoginResp.return_->operationSuccess)) {
//                cout << "Login success" << endl;
//
//                printf("Information received: \n");
//                printf("firstName: %s\n", nsLoginResp.return_->firstName->c_str());
//                printf("familyName: %s\n", nsLoginResp.return_->familyName->c_str());
//                printf("username: %s\n", nsLoginResp.return_->username->c_str());
//                printf("provider: %s\n", nsLoginResp.return_->provider->c_str());
//                printf("roleDesc: %s\n", nsLoginResp.return_->roleDesc->c_str());
//                printf("softwareDesc: %s\n", nsLoginResp.return_->softwareDesc->c_str());
//                printf("sessionToken: %s\n", nsLoginResp.return_->sessionToken->c_str());
//                printf("welcomeMessage: %s\n", nsLoginResp.return_->welcomeMessage->c_str());
//                printf("operationSuccess: %d\n", *(nsLoginResp.return_->operationSuccess));

                return true;
            } else {
                //cout << "Login failed" << endl;
                return false;
            }
        }


        ns1__getUserNonceResponse Authenticator::getUserNonce() {
            ns1__getUserNonce nsUserNonce;
            ns1__getUserNonceResponse nsUserNonceResp;

            nsUserNonce.username = &m_username;
            nsUserNonce.provider = &m_provider;
            nsUserNonce.ipAddress = &m_ipAddress;

            // If obtain successfully answer from Web Service it print message returned!
            if (m_service->getUserNonce(&nsUserNonce, &nsUserNonceResp) == SOAP_OK) {
                //std::cout << &(nsUserNonceResp.return_) << std::endl;
                if (*(nsUserNonceResp.return_->operationSuccess) == 1) {
                    //printf("The nonce was: %s\n", nsUserNonceResp.return_->sessionToken->c_str());
                } else {
                    //printf("Error message: %s\n", nsUserNonceResp.return_->errorMsg->c_str());
                }
            } else {
                soap_print_fault(m_service->soap, stderr);
            }

            return nsUserNonceResp;
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
                //std::cout << &(nsLoginResp.return_) << std::endl;
                if (*(nsLoginResp.return_->operationSuccess) == 1) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "The SessionToken is:" << string(nsLoginResp.return_->sessionToken->c_str());
                } else {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Error message: " << string(nsLoginResp.return_->errorMsg->c_str());
                }
            } else {
                throw KARABO_NETWORK_EXCEPTION("Problem authenticating"); // TODO fetch this as std::string : soap_print_fault(service.soap, stderr);
            }

            return nsLoginResp;
        }

    }
}


