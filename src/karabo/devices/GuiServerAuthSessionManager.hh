/*
 * GuiServerAuthSessionManager.hh
 *
 * Manages GUI Server user-authenticated sessions.
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

#ifndef GUI_SERVER_AUTH_SESSION_MANAGER_HH
#define GUI_SERVER_AUTH_SESSION_MANAGER_HH

#include <atomic>
#include <boost/asio/deadline_timer.hpp>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

#include "karabo/data/time/Epochstamp.hh"
#include "karabo/data/time/TimeDuration.hh"
#include "karabo/data/types/Schema.hh"
#include "karabo/net/UserAuthClient.hh"

namespace karabo::devices {

    // The most privileged access level to be associated with a session right after the login.
    inline constexpr karabo::data::Schema::AccessLevel MAX_LOGIN_ACCESS_LEVEL =
          karabo::data::Schema::AccessLevel::EXPERT;

    inline constexpr karabo::data::Schema::AccessLevel MAX_SESSION_ACCESS_LEVEL =
          karabo::data::Schema::AccessLevel::EXPERT;

    inline constexpr unsigned int CHECK_SESSION_EXPIRATION_INTERVAL_SECS = 5U;

    // A beginSession is basically a OneTimeTokenAuthorization operation plus
    // some internal housekeeping from the Manager, hence the inheritance.
    struct BeginSessionResult : public karabo::net::OneTimeTokenAuthorizeResult {
        std::string sessionToken;
        // Session duration in seconds
        unsigned int sessionDurationSecs{5 * 60};
        karabo::data::Epochstamp expiresAt{0ul, 0ul};
    };

    // Result of an end session triggered by an external request.
    struct EndSessionResult {
        bool success;
        std::string sessionToken;
        std::string errMsg;
    };

    struct ExpiredSessionInfo {
        std::string expiredToken;
        karabo::data::Epochstamp expirationTime;
    };

    struct EminentExpirationInfo {
        std::string aboutToExpireToken;
        karabo::data::TimeDuration timeForExpiration;
    };

    using BeginSessionHandler = std::function<void(const BeginSessionResult&)>;

    /** Handler for expired session events. */
    using ExpirationHandler = std::function<void(const ExpiredSessionInfo&)>;

    /** Handler for "session about to expire" events. */
    using EminentExpirationHandler = std::function<void(const EminentExpirationInfo&)>;

    /**
     * @brief Manages user-authentication GUI Sessions.
     *
     * Takes care of authorizing one-time session tokens to start sessions and of communicating
     * sessions about to expire or already expired.
     */
    class GuiServerAuthSessionManager : public std::enable_shared_from_this<GuiServerAuthSessionManager> {
       public:
        /**
         * @brief Construct a new Gui Server Session Manager object
         *
         * @param topic the Karabo topic against which session tokens will be authorized.
         * @param authServerUrl the URL of the authentication server to use for authorizing one-time temporary
         * session tokens.
         * @param sessionDurationSeconds the duration, in seconds, to be enforced for sessions.
         * @param sessionEndNoticeSeconds the time in advance, in seconds, to communicate about an eminent
         * end of session event.
         * @param onEminentExpiration handler for sessions about to expire.
         * @param onExpiration handler for expired sessions.
         */
        GuiServerAuthSessionManager(const std::string& topic, const std::string& authServerUrl,
                                    unsigned int sessionDurationSeconds, unsigned int sessionEndNoticeSeconds,
                                    EminentExpirationHandler onEminentExpiration, ExpirationHandler onExpiration);

        /**
         * @brief Asynchronously starts a new session for a given one-time session token.
         *
         * @param sessionToken the one-time session token to be authorized and bound to the started
         * session.
         * @param onBeginSession handler for begin session events (either successful or failed).
         *
         * @note Calls the registered BeginSessionHandler with the results of the beginSession
         * operation.
         */
        void beginSession(const std::string& sessionToken, const BeginSessionHandler& onBeginSession);

        /**
         * @brief Synchronously terminates a session referenced by a given session token.
         *
         * @param sessionToken the one-time session token bound to the session to be terminated.
         *
         * @returns a structure with the endSession operation results.
         *
         * @note an error due to a beginSession token not found isn't necessarily an error from the GUI client
         * point of view. In the unlikely scenario of an endSession request that reaches the GUI server while
         * the expiration check that will detect the expiration of the same token is already running, the end
         * session request will "fail" with a "token not found" message. It is up to the GUI client to decide what
         * to do in such cases - maybe keep track of an "over the wire" end session request token and ignore any
         * error related to it if an expiration notification is received for that token between the request dispatch
         * and the arrival of its response.
         **/
        EndSessionResult endSession(const std::string& sessionToken);

        /**
         * @brief Checks whether a session is "about" to expire - current time
         * is between the expiration notice and expiration timepoints for the
         * session
         *
         * @param sessionToken the one-time token bound to the session to be
         * checked
         *
         * @throws KARABO_PARAMETER_EXCEPTION if sessionToken doesn't correspond
         * to a known session.
         */
        bool isSessionExpiring(const std::string& sessionToken);

       private:
        /**
         * @brief Checks the currently active sessions removing the expired ones after invoking the registered
         * expiration handlers for each of them.
         *
         * @param error an error code sent by boost::asio that if different from 0 indicates that the timer pulse that
         * should invoke this check at some future point has been cancelled.
         */
        void checkSessionsExpirations(const boost::system::error_code& error);

        /**
         * @brief Handles the result of a session token authorization request, updating the internal state of
         * the manager and communicating the outcome of the begin session request to the external requestor.
         *
         * @param sessionToken the one-time session token whose authorization was requested.
         * @param onBeginSession handler for begin session events (either successful or failed).
         * @param authResult the result of the authorization of the session token provided by the external
         * caller of the begin session operation.
         */
        void onTokenAuthorizeResult(const std::string& sessionToken, const BeginSessionHandler& onBeginSession,
                                    const karabo::net::OneTimeTokenAuthorizeResult& authResult);

        /**
         * @brief Schedules the next expiration check if there's any session to be checked.
         *
         * @note this method must be called with the m_sessionsMutex locked.
         */
        void scheduleNextExpirationsCheck();

        std::string m_topic;
        karabo::net::UserAuthClient m_authClient;
        unsigned int m_sessionDurationSecs;
        karabo::data::TimeDuration m_sessionEndNoticeSecs;
        EminentExpirationHandler m_eminentExpirationHandler;
        ExpirationHandler m_expirationHandler;
        boost::asio::steady_timer m_checkExpirationsTimer;
        std::atomic<bool> m_expirationTimerWaiting;
        std::map<std::string /* sessionToken */, karabo::data::Epochstamp /* expiration time */> m_sessions;
        std::mutex m_sessionsMutex;
        std::unordered_set<std::string> m_endNoticesSent;
        std::mutex m_endNoticesMutex;
    };

} // namespace karabo::devices


#endif
