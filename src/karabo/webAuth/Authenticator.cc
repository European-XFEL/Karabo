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
#include "soapH.h"
#include "AuthenticationPortBinding.nsmap"
#include <signal.h>		/* defines SIGPIPE */

#include <unistd.h>		/* defines _POSIX_THREADS if pthreads are available */
#if defined(_POSIX_THREADS) || defined(_SC_THREADS)
#include <pthread.h>
#endif

using namespace std;

int CRYPTO_thread_setup();
void CRYPTO_thread_cleanup();
void sigpipe_handle(int);

namespace karabo {
    namespace webAuth {


        Authenticator::Authenticator(const std::string& username, const std::string& password, const std::string& provider,
                                     const std::string& ipAddress, const std::string& brokerHostname, const int brokerPortNumber,
                                     const std::string& brokerTopic)
            : m_username(username),
            m_password(password),
            m_provider(provider),
            m_ipAddress(ipAddress),
            m_brokerHostname(brokerHostname),
            m_brokerPortNumber(karabo::util::toString(brokerPortNumber)),
            m_brokerTopic(brokerTopic),
            m_software(KARABO_SOFTWARE_DESC),
            //m_service(new AuthenticationPortBindingProxy),
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

            // Function that CREATE the SSL connections and variables with the server!
            setSslService();
        }


        void Authenticator::cleanup() {
            CRYPTO_thread_cleanup();
        }


        void Authenticator::setSslService() {
            m_soap = boost::shared_ptr<soap>(new soap());
            soap_ssl_init();
            if (CRYPTO_thread_setup()) {
                fprintf(stderr, "Cannot setup thread mutex for OpenSSL\n");
                exit(1);
            }

            // Init gSOAP context
            soap_init(m_soap.get());

            if (soap_ssl_client_context(m_soap.get(),
                                        /* SOAP_SSL_NO_AUTHENTICATION, */ /* for encryption w/o authentication */
                                        /* SOAP_SSL_DEFAULT | SOAP_SSL_SKIP_HOST_CHECK, */ /* if we don't want the host name checks since these will change from machine to machine */
                                        SOAP_SSL_NO_AUTHENTICATION, /* use SOAP_SSL_DEFAULT in production code */
                                        NULL, /* keyfile (cert+key): required only when client must authenticate to server (see SSL docs to create this file) */
                                        NULL, /* password to read the keyfile */
                                        NULL, /* optional cacert file to store trusted certificates, use cacerts.pem for all public certificates issued by common CAs */
                                        NULL, /* optional capath to directory with trusted certificates */
                                        NULL /* if randfile!=NULL: use a file with random data to seed randomness */
                                        )) {
                soap_print_fault(m_soap.get(), stderr);
                exit(1);
            }

            // Set timeouts in order to avoid blocking threads during 60 seconds per request (default value)
            m_soap.get()->connect_timeout = 3; /* try to connect for 3 seconds */
            m_soap.get()->send_timeout = m_soap.get()->recv_timeout = 1; /* if I/O stalls, then timeout after 1 seconds */
            m_soap.get()->accept_timeout = 5; /* if without answer, timeout after 5 seconds */

            // Create SSL Service from the parameters changed in the previous lines of code
            m_service = boost::shared_ptr<AuthenticationPortBindingProxy> (new AuthenticationPortBindingProxy(m_soap.get()));
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

            nsLogin.username = &m_username;
            nsLogin.password = &m_password;
            nsLogin.provider = &m_provider;
            nsLogin.ipAddress = &m_ipAddress;
            nsLogin.brokerHostname = &m_brokerHostname;
            nsLogin.brokerPortNumber = &m_brokerPortNumber;
            nsLogin.brokerTopic = &m_brokerTopic;
            nsLogin.nonce = &m_nonce;
            nsLogin.software = &m_software;

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
         * Auxiliary functions
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
        const std::string& Authenticator::getDefaultAccessLevelDesc() const {
            return m_defaultAccessLevelDesc;
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
            m_defaultAccessLevelId = defaultAccessLevelId;
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
            m_softwareId = softwareId;
        }


        void Authenticator::setUserId(const long long userId) {
            m_userId = userId;
        }


        void Authenticator::setWelcomeMessage(const std::string* welcomeMessage) {
            if (welcomeMessage)
                m_welcomeMessage = *welcomeMessage;
            else
                m_welcomeMessage = "";
        }

    } // namespace packageName
} // namespace karabo



/******************************************************************************\
 *
 *	OpenSSL
 *
\******************************************************************************/
#ifdef WITH_OPENSSL

#if defined(WIN32)
#define MUTEX_TYPE		HANDLE
#define MUTEX_SETUP(x)		(x) = CreateMutex(NULL, FALSE, NULL)
#define MUTEX_CLEANUP(x)	CloseHandle(x)
#define MUTEX_LOCK(x)		WaitForSingleObject((x), INFINITE)
#define MUTEX_UNLOCK(x)	ReleaseMutex(x)
#define THREAD_ID		GetCurrentThreadId()
#elif defined(_POSIX_THREADS) || defined(_SC_THREADS)
#define MUTEX_TYPE		pthread_mutex_t
#define MUTEX_SETUP(x)		pthread_mutex_init(&(x), NULL)
#define MUTEX_CLEANUP(x)	pthread_mutex_destroy(&(x))
#define MUTEX_LOCK(x)		pthread_mutex_lock(&(x))
#define MUTEX_UNLOCK(x)	pthread_mutex_unlock(&(x))
#define THREAD_ID		pthread_self()
#else
#error "You must define mutex operations appropriate for your platform"
#error	"See OpenSSL /threads/th-lock.c on how to implement mutex on your platform"
#endif


struct CRYPTO_dynlock_value {


    MUTEX_TYPE mutex;
};

static MUTEX_TYPE *mutex_buf;


static struct CRYPTO_dynlock_value *dyn_create_function(const char *file, int line) {
    struct CRYPTO_dynlock_value *value;
    value = (struct CRYPTO_dynlock_value*) malloc(sizeof (struct CRYPTO_dynlock_value));
    if (value)
        MUTEX_SETUP(value->mutex);
    return value;
}


static void dyn_lock_function(int mode, struct CRYPTO_dynlock_value *l, const char *file, int line) {
    if (mode & CRYPTO_LOCK)
        MUTEX_LOCK(l->mutex);
    else
        MUTEX_UNLOCK(l->mutex);
}


static void dyn_destroy_function(struct CRYPTO_dynlock_value *l, const char *file, int line) {
    MUTEX_CLEANUP(l->mutex);
    free(l);
}


void locking_function(int mode, int n, const char *file, int line) {
    if (mode & CRYPTO_LOCK)
        MUTEX_LOCK(mutex_buf[n]);
    else
        MUTEX_UNLOCK(mutex_buf[n]);
}


unsigned long id_function() {
    return (unsigned long) THREAD_ID;
}


int CRYPTO_thread_setup() {
    int i;
    mutex_buf = (MUTEX_TYPE*) malloc(CRYPTO_num_locks() * sizeof (pthread_mutex_t));
    if (!mutex_buf)
        return SOAP_EOM;
    for (i = 0; i < CRYPTO_num_locks(); i++)
        MUTEX_SETUP(mutex_buf[i]);
    CRYPTO_set_id_callback(id_function);
    CRYPTO_set_locking_callback(locking_function);
    CRYPTO_set_dynlock_create_callback(dyn_create_function);
    CRYPTO_set_dynlock_lock_callback(dyn_lock_function);
    CRYPTO_set_dynlock_destroy_callback(dyn_destroy_function);
    return SOAP_OK;
}


void CRYPTO_thread_cleanup() {
    int i;
    if (!mutex_buf)
        return;
    CRYPTO_set_id_callback(NULL);
    CRYPTO_set_locking_callback(NULL);
    CRYPTO_set_dynlock_create_callback(NULL);
    CRYPTO_set_dynlock_lock_callback(NULL);
    CRYPTO_set_dynlock_destroy_callback(NULL);
    for (i = 0; i < CRYPTO_num_locks(); i++)
        MUTEX_CLEANUP(mutex_buf[i]);
    free(mutex_buf);
    mutex_buf = NULL;
}

#else


/* OpenSSL not used, e.g. GNUTLS is used */

int CRYPTO_thread_setup() {
    return SOAP_OK;
}


void CRYPTO_thread_cleanup() {
}

#endif


/******************************************************************************\
 *
 *	SIGPIPE
 *
\******************************************************************************/
void sigpipe_handle(int x) {
}

