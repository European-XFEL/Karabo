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
            //
            // Information returned when login is made
            unsigned long long int m_userId;
            unsigned long long int m_softwareId;
            unsigned long long int m_roleId;
            std::string m_nonce;
            std::string m_sessionToken;
            std::string m_welcomeMessage;
            std::string m_roleDesc;

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
            }

            std::string getSessionToken() const;
            std::string getSoftware() const;
            std::string getPortNumber() const;
            std::string getHostname() const;
            std::string getIpAddress() const;
            std::string getProvider() const;
            std::string getPassword() const;
            std::string getUsername() const;
            std::string getRoleDesc() const;
            std::string getWelcomeMessage() const;
            
            unsigned long long int getRoleId() const;
            unsigned long long int getSoftwareId() const;
            unsigned long long int getUserId() const;
            
        private:

            ns1__loginResponse authenticate(const karabo::util::Timestamp& timestamp);

            ns1__getUserNonceResponse getUserNonce();

            void setSessionToken(const std::string& newSessionToken);
            void setRoleDesc(const std::string& roleDesc);
            void setWelcomeMessage(const std::string& welcomeMessage);
            void setRoleId(const unsigned long long int roleId);
            void setSoftwareId(const unsigned long long int softwareId);
            void setUserId(const unsigned long long int userId);

            void printObject(ns1__loginResponse nsLoginResp);

            std::string soapMessageNotOk(struct soap *soap);

        };
    }
}


#endif	/* AUTHENTICATOR_HH */

