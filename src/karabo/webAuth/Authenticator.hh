/*
 *
 * File:   Authenticator.hh
 * Author: <luis.maia@xfel.eu>
 *
 * Created on April 12, 2013, 4:31 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef _KARABO_WEBAUTH_AUTHENTICATOR_HH
#define	_KARABO_WEBAUTH_AUTHENTICATOR_HH

#include <karabo/util/Configurator.hh>
#include <karabo/util/Timestamp.hh>
#include "soapStub.h"

#ifndef KARABO_DEFAULT_ACCESS_LEVEL
#define KARABO_DEFAULT_ACCESS_LEVEL 4
#endif

#define KARABO_SOFTWARE_DESC "Karabo"
#define KARABO_INVALID_ID -100

// Forward
class AuthenticationPortBindingProxy;

namespace karabo {
    namespace webAuth {

        class Authenticator {

            std::string m_username;
            std::string m_password;
            std::string m_provider;
            std::string m_ipAddress;
            std::string m_brokerHostname;
            std::string m_brokerPortNumber;
            std::string m_brokerTopic;
            std::string m_software;
            //
            boost::shared_ptr<AuthenticationPortBindingProxy> m_service;

            // Information returned when login is made
            std::string m_nonce;
            //
            std::string m_firstName;
            std::string m_familyName;
            long long m_userId;
            //
            long long m_softwareId;
            std::string m_softwareDesc;
            //
            int m_defaultAccessLevelId;
            std::string m_defaultAccessLevelDesc;
            std::string m_accessList;
            //
            std::string m_sessionToken;
            std::string m_welcomeMessage;

            karabo::util::Hash m_accessHash;

        public:

            KARABO_CLASSINFO(Authenticator, "Authenticator", "1.0");

            /**
             * Empty constructor
             */
            Authenticator() {
            }

            /**
             * Constructor (all parameters are initialized)
             * 
             * @param username
             * @param password
             * @param provider
             * @param ipAddress
             * @param brokerHostname
             * @param brokerPortNumber
             * @param brokerTopic
             */
            Authenticator(const std::string& username, const std::string& password, const std::string& provider,
                          const std::string& ipAddress, const std::string& brokerHostname, const std::string& brokerPortNumber,
                          const std::string& brokerTopic);

            /**
             * Taking into consideration the Class parameters, authenticate the user in the WebAuth service
             *  
             * @return True or False depending on the success/unsuccess of the authentication
             */
            bool login(); //bool login(const karabo::util::Timestamp& timestamp = karabo::util::Timestamp());

            /**
             * Taking into consideration the Class parameters, logout the user in the WebAuth service
             *  
             * @return True or False depending on the success/unsuccess of the logout
             */
            bool logout();

            /**
             * Gets the SessionToken associated with current class properties for a specified IP Address
             * 
             * @param ipAddress
             * @return sessionToken (if user has a valid session for the specified IP Address)
             */
            std::string getSingleSignOn(const std::string ipAddress);

            /**
             * Class destructor
             */
            virtual ~Authenticator() {
            }


            /*
             * Getters
             */
            const std::string& getAccessList() const;
            const std::string& getBrokerHostname() const;
            const std::string& getBrokerPortNumber() const;
            const std::string& getBrokerTopic() const;
            const std::string& getDefaultAccessLevelDesc() const;
            const int getDefaultAccessLevelId() const;
            const std::string& getFamilyName() const;
            const std::string& getFirstName() const;
            const std::string& getIpAddress() const;
            const std::string& getNonce() const;
            const std::string& getPassword() const;
            const std::string& getProvider() const;
            const std::string& getSessionToken() const;
            const std::string& getSoftware() const;
            const std::string& getSoftwareDesc() const;
            const long long getSoftwareId() const;
            const long long getUserId() const;
            const std::string& getUsername() const;
            const std::string& getWelcomeMessage() const;
            //
            const boost::shared_ptr<AuthenticationPortBindingProxy> getService() const;
            const karabo::util::Hash& getAccessHash() const;

        private:

            /**
             * Get a NONCE for the current user (username, provider) and IP Address to allow him to authenticate in the WebAuth Service
             * 
             * @return Class returned from the WebService
             */
            ns1__getUserNonceResponse getUserNonce();

            /**
             * Tries to authenticate the current user (username, provider) in the WebAuth Service
             * 
             * @return Class returned from the WebService
             */
            ns1__loginResponse authenticate();


            /**
             * Prints the loginResponse data received from the server
             * 
             * @param nsLoginResp
             */
            void printObject(ns1__loginResponse nsLoginResp);

            /**
             * Prints the SOAP error message when the SOAP message is not OK
             * 
             * @param soap
             * @return Error information
             */
            std::string soapMessageNotOk(struct soap *soap);


            /*
             * Setters
             */
            void setAccessList(const std::string* accessList);
            void setDefaultAccessLevelDesc(const std::string* defaultAccessLevelDesc);
            void setDefaultAccessLevelId(const int defaultAccessLevelId);
            void setFamilyName(const std::string* familyName);
            void setFirstName(const std::string* firstName);
            void setNonce(const std::string* nonce);
            void setSessionToken(const std::string* sessionToken);
            void setSoftwareDesc(const std::string* softwareDesc);
            void setSoftwareId(const long long softwareId);
            void setUserId(const long long userId);
            void setWelcomeMessage(const std::string* welcomeMessage);

        };
    }
}

#endif	/* AUTHENTICATOR_HH */

