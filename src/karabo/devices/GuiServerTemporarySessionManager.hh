/*
 * GuiServerTemporarySessionManager.hh
 *
 * Manages temporary sessions created on top of user-authenticated GUI Server
 * sessions.
 *
 * Created on January, 22, 2024.
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef GUI_SERVER_TEMPORARY_SESSION_MANAGER_HH
#define GUI_SERVER_TEMPORARY_SESSION_MANAGER_HH

#include <atomic>
#include <boost/asio/deadline_timer.hpp>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "karabo/net/UserAuthClient.hh"
#include "karabo/util/Epochstamp.hh"
#include "karabo/util/Schema.hh"
#include "karabo/util/TimeDuration.hh"

namespace karabo::devices {

    // The most priviliged access level to be associated with a session right after the login.
    inline constexpr karabo::util::Schema::AccessLevel MAX_LOGIN_ACCESS_LEVEL =
          karabo::util::Schema::AccessLevel::ADMIN;

    inline constexpr karabo::util::Schema::AccessLevel MAX_TEMPORARY_SESSION_ACCESS_LEVEL =
          karabo::util::Schema::AccessLevel::ADMIN;

    inline constexpr unsigned int CHECK_TEMPSESSION_EXPIRATION_INTERVAL_SECS = 5U;


    // A beginTemporarySession is basically a OneTimeTokenAuthorization operation plus
    // some internal housekeeping from the Manager, hence the inheritance.
    struct BeginTemporarySessionResult : public karabo::net::OneTimeTokenAuthorizeResult {
        std::string temporarySessionToken;
        // Temporary Session duration in seconds
        unsigned int temporarySessionDurationSecs{5 * 60}; // Default for maxTemporarySessionTime of the GUI Server
        karabo::util::Epochstamp expiresAt{0ul, 0ul};
    };

    // Result of an end temporary session triggered by an external request.
    struct EndTemporarySessionResult {
        bool success;
        std::string temporarySessionToken;
        std::string errMsg;
    };

    struct ExpiredTemporarySessionInfo {
        std::string expiredToken;
        karabo::util::Epochstamp expirationTime;
    };

    struct EminentExpirationInfo {
        std::string aboutToExpireToken;
        karabo::util::TimeDuration timeForExpiration;
    };

    using BeginTemporarySessionHandler = std::function<void(const BeginTemporarySessionResult&)>;

    /** Handler for expired temporary session events. */
    using ExpirationHandler = std::function<void(const ExpiredTemporarySessionInfo&)>;

    /** Handler for "temporary session about to expire" events. */
    using EminentExpirationHandler = std::function<void(const EminentExpirationInfo&)>;

    /**
     * @brief Manages temporary sessions on top of user-authenticated GUI Sessions.
     *
     * Takes care of authorizing one-time temporary session tokens to start temporary sessions and of communicating
     * temporary sessions about to expire or already expired.
     */
    class GuiServerTemporarySessionManager : public std::enable_shared_from_this<GuiServerTemporarySessionManager> {
       public:
        /**
         * @brief Construct a new Gui Server Temporary Session Manager object
         *
         * @param topic the Karabo topic against which temporary session tokens will be authorized.
         * @param authServerUrl the URL of the authentication server to use for authorizing one-time temporary
         * session tokens.
         * @param temporarySessionDurationSeconds the duration, in seconds, to be enforced for temporary sessions.
         * @param temporarySessionEndNoticeSeconds the time in advance, in seconds, to communicate about an eminent
         * end of temporary session event.
         * @param onEminentExpiration handler for temporary sessions about to expire.
         * @param onExpiration handler for expired temporary sessions.
         */
        GuiServerTemporarySessionManager(const std::string& topic, const std::string& authServerUrl,
                                         unsigned int temporarySessionDurationSeconds,
                                         unsigned int temporarySessionEndNoticeSeconds,
                                         EminentExpirationHandler onEminentExpiration, ExpirationHandler onExpiration);

        /**
         * @brief Assynchronously starts a new temporary session for a given one-time temporary session token.
         *
         * @param temporarySessionToken the one-time temporary session token to be authorized and bound to the started
         * temporary session.
         * @param onBeginTemporarySession handler for begin temporary session events (either successful or failed).
         *
         * @note Calls the registered BeginTemporarySessionHandler with the results of the beginTemporarySession
         * operation.
         */
        void beginTemporarySession(const std::string& temporarySessionToken,
                                   const BeginTemporarySessionHandler& onBeginTemporarySession);

        /**
         * @brief Synchronously terminates a temporary session referenced by a given temporary session token.
         *
         * @param temporarySessionToken the one-time temporary session token bound to the session to be terminated.
         *
         * @returns a structure with the endTemporarySession operation results.
         *
         * @note an error due to a beginTemporarySession token not found isn't necessarily an error from the GUI client
         * point of view. In the unlikely scenario of an endTemporarySession request that reaches the GUI server while
         * the expiration check that will detect the expiration of the same token is already running, the end temporary
         * session request will "fail" with a "token not found" message. It is up to the GUI client to decide what to
         * do in such cases - maybe keep track of an "over the wire" end temporary session request token and ignore any
         * error related to it if an expiration notification is received for that token between the request dispatch and
         * the arrival of its reponse.
         **/
        EndTemporarySessionResult endTemporarySession(const std::string& temporarySessionToken);

       private:
        /**
         * @brief Checks the currently active temporary sessions removing the expired ones after invoking the registered
         * expiration handlers for each of them.
         *
         * @param error an error code sent by boost::asio that if different from 0 indicates that the timer pulse that
         * should invoke this check at some future point has been cancelled.
         */
        void checkTemporarySessionsExpirations(const boost::system::error_code& error);

        /**
         * @brief Handles the result of a temporary session token authorization request, updating the internal state of
         * the manager and communicating the outcome of the begin temporary session request to the external requestor.
         *
         * @param temporarySessionToken the one-time temporary session token whose authorization was requested.
         * @param onBeginTemporarySession handler for begin temporary session events (either successful or failed).
         * @param authResult the result of the authorization of the temporary session token provided by the external
         * caller of the begin temporary session operation.
         */
        void onTokenAuthorizeResult(const std::string& temporarySessionToken,
                                    const BeginTemporarySessionHandler& onBeginTemporarySession,
                                    const karabo::net::OneTimeTokenAuthorizeResult& authResult);

        /**
         * @brief Schedules the next expiration check if there's any esalation to be checked.
         *
         * @note this method must be called with the m_tempSessionsMutex locked.
         */
        void scheduleNextExpirationsCheck();

        std::string m_topic;
        karabo::net::UserAuthClient m_authClient;
        unsigned int m_temporarySessionDurationSecs;
        karabo::util::TimeDuration m_temporarySessionEndNoticeSecs;
        EminentExpirationHandler m_eminentExpirationHandler;
        ExpirationHandler m_expirationHandler;
        boost::asio::steady_timer m_checkExpirationsTimer;
        std::atomic<bool> m_expirationTimerWaiting;
        std::map<std::string /* temporarySessionToken */, karabo::util::Epochstamp /* expiration time */>
              m_tempSessions;
        std::mutex m_tempSessionsMutex;
    };

} // namespace karabo::devices


#endif
