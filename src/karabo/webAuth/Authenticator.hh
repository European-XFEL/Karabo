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
#include "soapStub.h"

#ifndef KARABO_DEFAULT_ACCESS_LEVEL
//#define KARABO_DEFAULT_ACCESS_LEVEL karabo::util::Schema::OBSERVER // Inside XFEL
#define KARABO_DEFAULT_ACCESS_LEVEL karabo::util::Schema::ADMIN // Outside XFEL
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
            boost::shared_ptr<soap> m_soap;

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
                          const std::string& ipAddress, const std::string& brokerHostname, const int brokerPortNumber,
                          const std::string& brokerTopic);

            /**
             * Taking into consideration the Class parameters, authenticate the user in the WebAuth service
             *  
             * @return True or False depending on the success/unsuccess of the authentication
             */
            bool login();

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
                cleanup();
            }

            /**
             * Gets the Broker Hostname
             * 
             * @return string with brokerHostname
             */
            inline const std::string& getBrokerHostname() const {
                return m_brokerHostname;
            }

            /**
             * Gets the Broker Port Number
             * 
             * @return templated brokerPortNumber
             */
            const int getBrokerPortNumber() const {
                return boost::lexical_cast<int>(m_brokerPortNumber);
            }

            /**
             * Gets the Broker Topic
             * 
             * @return string with brokerTopic
             */
            inline const std::string& getBrokerTopic() const {
                return m_brokerTopic;
            }

            /**
             * Gets the User Default Access Level description
             * 
             * @return string with defaultAccessLevelDesc
             */
            const std::string& getDefaultAccessLevelDesc() const;

            /**
             * Gets the User Default Access Level identifier
             * 
             * @return string with defaultAccessLevelId
             */
            inline const int getDefaultAccessLevelId() const {
                return m_defaultAccessLevelId;
            }

            /**
             * Gets the User Family name
             * 
             * @return string with familyName
             */
            inline const std::string& getFamilyName() const {
                return m_familyName;
            }

            /**
             * Gets the User First name
             * 
             * @return string with firstName
             */
            inline const std::string& getFirstName() const {
                return m_firstName;
            }

            /**
             * Gets the Current login (connection) Ip address
             * 
             * @return string with ipAddress
             */
            inline const std::string& getIpAddress() const {
                return m_ipAddress;
            }

            /**
             * Gets a connection (new) Nonce that is valid for the following 5 minutes (default configuration)
             * 
             * @return string with nonce
             */
            inline const std::string& getNonce() const {
                return m_nonce;
            }

            /**
             * Gets user encrypted password (excluding the salt)
             * 
             * @return string with password
             */
            inline const std::string& getPassword() const {
                return m_password;
            }

            /**
             * Gets Provider to be used to validate user credentials
             * 
             * @return string with provider
             */
            inline const std::string& getProvider() const {
                return m_provider;
            }

            /**
             * Gets user's current session (connection) token that can be used for guarantee user authenticity without reintroduce it's password
             * 
             * @return string with sessionToken
             */
            inline const std::string& getSessionToken() const {
                return m_sessionToken;
            }

            /**
             * Gets Software Identifier to which the user wants to authenticate
             * 
             * @return string with software identifier
             */
            inline const std::string& getSoftware() const {
                return m_software;
            }

            /**
             * Gets Software Description to which the user wants to authenticate
             * 
             * @return string with software Description
             */
            inline const std::string& getSoftwareDesc() const {
                return m_softwareDesc;
            }

            /**
             * Gets Software Identifier to which the user wants to authenticate
             * 
             * @return long long with software identifier
             */
            inline const long long getSoftwareId() const {
                return m_softwareId;
            }

            /**
             * Gets authenticated User Identifier
             * 
             * @return long long with user identifier
             */
            inline const long long getUserId() const {
                return m_userId;
            }

            /**
             * Gets authenticated User username
             * 
             * @return string with user username
             */
            inline const std::string& getUsername() const {
                return m_username;
            }

            /**
             * Gets authenticated User personalized welcome message
             * 
             * @return string with welcomeMessage
             */
            inline const std::string& getWelcomeMessage() const {
                return m_welcomeMessage;
            }

            /**
             * Gets the connection to the Web Service Server
             * 
             * @return boost::shared_ptr<AuthenticationPortBindingProxy> the server information
             */
            inline const boost::shared_ptr<AuthenticationPortBindingProxy> getService() const {
                return m_service;
            }

            /**
             * Gets the hash that contains all the exceptions to the default access level
             * 
             * @return karabo::util::Hash with access exceptions
             */
            inline const karabo::util::Hash& getAccessHash() const {
                return m_accessHash;
            }

        private:

            /**
             * Function to be call when destroying the object to clean Threads
             */
            void cleanup();

            /**
             * Create and configure soap "variable" taking into consideration SSL parameters
             */
            void setSslService();

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
             * Getters
             */
            inline const std::string& getAccessList() const {
                return m_accessList;
            }

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

