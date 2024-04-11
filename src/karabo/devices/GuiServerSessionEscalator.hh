/*
 * GuiServerSessionEscalator.hh
 *
 * Manages temporary access level escalations for user-authenticated GUI Server
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

#ifndef GUI_SERVER_SESSION_ESCALATOR_HH
#define GUI_SERVER_SESSION_ESCALATOR_HH

#include <atomic>
#include <boost/asio/deadline_timer.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <map>
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

    inline constexpr karabo::util::Schema::AccessLevel MAX_ESCALATED_ACCESS_LEVEL =
          karabo::util::Schema::AccessLevel::ADMIN;

    inline constexpr unsigned int CHECK_ESCALATE_EXPIRATION_INTERVAL_SECS = 10;


    // An escalate is basically a OneTimeTokenAuthorization operation plus
    // some internal housekeeping from the Escalator, hence the inheritance.
    struct EscalateResult : public karabo::net::OneTimeTokenAuthorizeResult {
        std::string escalationToken;
        // Escalation duration in seconds
        unsigned int escalationDurationSecs{5 * 60}; // Default for maxEscalationTime of the GUI Server
        karabo::util::Epochstamp expiresAt{0ul, 0ul};
    };

    // Result of a deescalation triggered by an external request.
    struct DeescalationResult {
        bool success;
        std::string escalationToken;
        std::string errMsg;
    };

    struct ExpiredEscalationInfo {
        std::string expiredToken;
        karabo::util::Epochstamp expirationTime;
    };

    struct EminentExpirationInfo {
        std::string aboutToExpireToken;
        karabo::util::TimeDuration timeForExpiration;
    };

    using EscalationHandler = boost::function<void(const EscalateResult&)>;

    /** Handler for expired escalations events. */
    using ExpirationHandler = boost::function<void(const ExpiredEscalationInfo&)>;

    /** Handler for "escalation about to expire" events. */
    using EminentExpirationHandler = boost::function<void(const EminentExpirationInfo&)>;

    /**
     * @brief Manages temporary privilege escalations for user-authenticated GUI Sessions.
     *
     * Takes care of authorizing one-time escalation tokens to start temporary escalations and of communicating
     * escalations about to expire or already expired.
     */
    class GuiServerSessionEscalator : public boost::enable_shared_from_this<GuiServerSessionEscalator> {
       public:
        /**
         * @brief Construct a new Gui Server Session Escalator object
         *
         * @param topic the Karabo topic against which escalation tokens will be authorized.
         * @param authServerUrl the URL of the authentication server to use for authorizing one-time escalation
         * tokens.
         * @param escalationDurationSeconds the duration, in seconds, to be enforced for escalations.
         * @param escalationEndNoticeSeconds the time in advance, in seconds, to communicate about an eminent
         * end of escalation event.
         * @param onEminentExpiration handler for escalation sessions about to expire.
         * @param onExpiration handler for expired escalation sessions.
         */
        GuiServerSessionEscalator(const std::string& topic, const std::string& authServerUrl,
                                  unsigned int escalationDurationSeconds, unsigned int escalationEndNoticeSeconds,
                                  EminentExpirationHandler onEminentExpiration, ExpirationHandler onExpiration);

        /**
         * @brief Assynchronously starts a new escalated session for a given one-time escalation token.
         *
         * @param escalationToken the one-time escalation token to be authorized and bound to the started escalated
         * session.
         * @param onEscalation handler for escalation events (either successful or failed escalations).
         *
         * @note Calls the registered EscalationHandler with the results of the escalate operation.
         */
        void escalate(const std::string& escalationToken, const EscalationHandler& onEscalation);

        /**
         * @brief Synchronously terminates an escalation session referenced by a given escalation token.
         *
         * @param escalationToken the one-time escalation token bound to the escalated session to be terminated.
         *
         * @returns a structure with the deescalation operation results.
         *
         * @note an error due to an escalation token not found isn't necessarily an error from the GUI client
         * point of view. In the unlikely scenario of a deescalation request that reaches the GUI server while the
         * expiration check that will detect the expiration of the same escalation token is already running, the
         * deescalate request will "fail" with a "token not found" message. It is up to the GUI client to decide what to
         * do in such cases - maybe keep track of an "over the wire" deescalation request token and ignore any error
         * related to it if an expiration notification is received for that token between the request dispatch and the
         * arrival of its reponse.
         **/
        DeescalationResult deescalate(const std::string& escalationToken);

       private:
        /**
         * @brief Checks the currently active escalations removing the expired ones after invoking the registered
         * expiration handlers for each of them.
         *
         * @param error an error code sent by boost::asio that if different from 0 indicates that the timer pulse that
         * should invoke this check at some future point has been cancelled.
         */
        void checkEscalationsExpirations(const boost::system::error_code& error);

        /**
         * @brief Handles the result of an escalation token authorization request, updating the internal state of
         * the escalator and communicating the outcome of the escalation request to the external requestor.
         *
         * @param escalationToken the one-time escalation token whose authorization was requested.
         * @param onEscalation handler for escalation events (either successful or failed escalations).
         * @param authResult the result of the authorization of the escalation token provided by the external caller
         * of the escalation operation.
         */
        void onTokenAuthorizeResult(const std::string& escalationToken, const EscalationHandler& onEscalation,
                                    const karabo::net::OneTimeTokenAuthorizeResult& authResult);

        /**
         * @brief Schedules the next expiration check if there's any esalation to be checked.
         *
         * @note this method must be called with the m_escalationsMutex locked.
         */
        void scheduleNextExpirationsCheck();

        std::string m_topic;
        karabo::net::UserAuthClient m_authClient;
        unsigned int m_escalationDurationSecs;
        karabo::util::TimeDuration m_escalationEndNoticeSecs;
        EminentExpirationHandler m_eminentExpirationHandler;
        ExpirationHandler m_expirationHandler;
        boost::asio::deadline_timer m_checkExpirationsTimer;
        std::atomic<bool> m_expirationTimerWaiting;
        std::map<std::string /* escalationToken */, karabo::util::Epochstamp /* expiration time */> m_escalations;
        std::mutex m_escalationsMutex;
    };

} // namespace karabo::devices


#endif
