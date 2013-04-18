/* 
 * File:   Authenticator.hh
 * Author: <luis.maia@xfel.eu>
 *
 * Created on April 12, 2013, 4:31 PM
 */

#ifndef _KARABO_WEBAUTH_AUTHENTICATOR_HH
#define	_KARABO_WEBAUTH_AUTHENTICATOR_HH

#include <karabo/util/Configurator.hh>
#include <karabo/util/Time.hh>
#include "soapStub.h"

// Forward
class AuthenticationPortBindingProxy;

namespace karabo {
    namespace webAuth {

        class Authenticator {
            std::string m_username;
            std::string m_password;
            std::string m_provider;
            std::string m_ipAddress;
            std::string m_hostname;
            std::string m_portNumber;
            std::string m_software;
            std::string m_nonce;
            std::string m_sessionToken;

            boost::shared_ptr<AuthenticationPortBindingProxy> m_service;

        public:

            KARABO_CLASSINFO(Authenticator, "Authenticator", "1.0");

            Authenticator(const std::string& username, const std::string& password, const std::string& provider,
                    const std::string& ipAddress, const std::string& hostname, const std::string& portNumber,
                    const std::string& software);

            bool login(const karabo::util::Timestamp& timestamp = karabo::util::Timestamp());

            bool logout();
            
            std::string getSingleSignOn(const std::string ipAddress);

            virtual ~Authenticator() {
            };

        private:

            ns1__loginResponse authenticate(const karabo::util::Timestamp& timestamp);

            ns1__getUserNonceResponse getUserNonce();

            void setSessionToken(const std::string& newSessionToken);

            void printObject(ns1__loginResponse nsLoginResp);

            std::string soapMessageNotOk(struct soap *soap);

        };
    }
}


#endif	/* AUTHENTICATOR_HH */

