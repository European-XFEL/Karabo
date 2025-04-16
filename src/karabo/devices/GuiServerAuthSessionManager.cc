/*
 * GuiServerAuthSessionManager.cc
 *
 * Manages GUI Server user-authenticated sessions.
 *
 * Created on January, 24, 2024.
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

#include "GuiServerAuthSessionManager.hh"

#include <boost/asio/placeholders.hpp>
#include <chrono>
#include <vector>

#include "karabo/data/time/TimeDuration.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/util/MetaTools.hh"

using namespace std::chrono;
using namespace karabo::data;
using namespace karabo::util;
using namespace karabo::net;
using namespace std::placeholders;

namespace karabo::devices {

    GuiServerAuthSessionManager::GuiServerAuthSessionManager(const std::string& topic, const std::string& authServerUrl,
                                                             unsigned int sessionDurationSeconds,
                                                             unsigned int sessionEndNoticeSeconds,
                                                             EminentExpirationHandler onEminentExpiration,
                                                             ExpirationHandler onExpiration)
        : m_topic(topic),
          m_authClient(authServerUrl),
          m_sessionDurationSecs(sessionDurationSeconds),
          m_sessionEndNoticeSecs{TimeDuration(sessionEndNoticeSeconds, 0ULL)},
          m_eminentExpirationHandler(std::move(onEminentExpiration)),
          m_expirationHandler(std::move(onExpiration)),
          m_checkExpirationsTimer(EventLoop::getIOService()),
          m_expirationTimerWaiting(false) {}

    void GuiServerAuthSessionManager::beginSession(const std::string& sessionToken,
                                                   const BeginSessionHandler& onBeginSession) {
        m_authClient.authorizeOneTimeToken(
              sessionToken, m_topic,
              bind_weak(&GuiServerAuthSessionManager::onTokenAuthorizeResult, this, sessionToken, onBeginSession, _1));
    }

    void GuiServerAuthSessionManager::scheduleNextExpirationsCheck() {
        if (m_sessions.size() > 0 && !m_expirationTimerWaiting.exchange(true)) {
            m_checkExpirationsTimer.expires_after(seconds(CHECK_SESSION_EXPIRATION_INTERVAL_SECS));
            m_checkExpirationsTimer.async_wait(bind_weak(&GuiServerAuthSessionManager::checkSessionsExpirations, this,
                                                         boost::asio::placeholders::error));
        }
    }

    void GuiServerAuthSessionManager::onTokenAuthorizeResult(const std::string& sessionToken,
                                                             const BeginSessionHandler& onBeginSession,
                                                             const OneTimeTokenAuthorizeResult& authResult) {
        BeginSessionResult result;
        result.success = authResult.success;
        result.accessLevel = authResult.accessLevel;
        result.userId = authResult.userId;
        result.errMsg = authResult.errMsg;
        result.sessionToken = sessionToken;
        result.sessionDurationSecs = m_sessionDurationSecs;
        if (authResult.success) {
            Epochstamp currTime;
            Epochstamp expiresAt = currTime + TimeDuration(m_sessionDurationSecs, 0ULL);
            if (result.accessLevel > MAX_SESSION_ACCESS_LEVEL) {
                // The access level returned by the authorize token operation is more privileged
                // than the one set to be used for the session level - "truncate" it.
                result.accessLevel = MAX_SESSION_ACCESS_LEVEL;
                // Note: if the access level returned by the authorize token operation is less privileged, keep it as
                // the higher level. As the authorize token operation takes into account the LDAP groups
                // memberships of the user, it shouldn't be simply ignored.
            }
            std::lock_guard<std::mutex> lock(m_sessionsMutex);
            m_sessions.emplace(sessionToken, expiresAt);
            scheduleNextExpirationsCheck();
        }
        onBeginSession(result);
    }

    EndSessionResult GuiServerAuthSessionManager::endSession(const std::string& sessionToken) {
        EndSessionResult result;
        result.sessionToken = sessionToken;
        std::lock_guard<std::mutex> lock(m_sessionsMutex);
        auto it = m_sessions.find(sessionToken);
        if (it == m_sessions.end()) {
            result.success = false;
            result.errMsg = "Temporary Session token not found";
        } else {
            result.success = true;
            result.errMsg = "";
            m_sessions.erase(it);
            {
                std::lock_guard<std::mutex> lock(m_endNoticesMutex);
                if (m_endNoticesSent.contains(sessionToken)) {
                    m_endNoticesSent.erase(sessionToken);
                }
            }
        }
        return result;
    }

    bool GuiServerAuthSessionManager::isSessionExpiring(const std::string& sessionToken) {
        Epochstamp sessionExpiration;
        {
            std::lock_guard<std::mutex> lock(m_sessionsMutex);
            auto it = m_sessions.find(sessionToken);
            if (it != m_sessions.end()) {
                sessionExpiration = it->second;
            } else {
                throw KARABO_PARAMETER_EXCEPTION("'" + sessionToken + "' does not correspond to a known session");
            }
        }
        TimeDuration sessionRemaining = sessionExpiration - Epochstamp();
        bool isExpiring = sessionRemaining.getTotalSeconds() < m_sessionEndNoticeSecs.getTotalSeconds();

        return isExpiring;
    }

    void GuiServerAuthSessionManager::checkSessionsExpirations(const boost::system::error_code& error) {
        m_expirationTimerWaiting = false;
        if (error) {
            // Timer has been cancelled.
            return;
        }
        std::vector<ExpiredSessionInfo> expiredInfos;
        std::vector<EminentExpirationInfo> eminentInfos;
        {
            std::lock_guard<std::mutex> lock(m_sessionsMutex);
            std::vector<std::string> tokens;
            tokens.reserve(m_sessions.size());
            for (const auto& [token, expTime] : m_sessions) {
                tokens.emplace_back(token);
            }
            Epochstamp currentTime;
            for (const std::string& token : tokens) {
                if (currentTime >= m_sessions[token]) {
                    // Temporary session has expired
                    expiredInfos.emplace_back(ExpiredSessionInfo{token, m_sessions[token]});
                    m_sessions.erase(token);
                    {
                        std::lock_guard<std::mutex> lock(m_endNoticesMutex);
                        if (m_endNoticesSent.contains(token)) {
                            m_endNoticesSent.erase(token);
                        }
                    }
                } else if (currentTime >= m_sessions[token] - m_sessionEndNoticeSecs) {
                    // Temporary session expiration occurs inside the eminent expiration time window
                    eminentInfos.emplace_back(EminentExpirationInfo{token, m_sessions[token] - currentTime});
                }
            }
        }
        scheduleNextExpirationsCheck();
        for (const ExpiredSessionInfo& expired : expiredInfos) {
            m_expirationHandler(expired);
        }
        {
            std::lock_guard<std::mutex> lock(m_endNoticesMutex);
            for (const EminentExpirationInfo& eminent : eminentInfos) {
                if (!m_endNoticesSent.contains(eminent.aboutToExpireToken)) {
                    // No EndNotice has yet been sent for the session - send it
                    m_eminentExpirationHandler(eminent);
                    m_endNoticesSent.emplace(eminent.aboutToExpireToken);
                }
            }
        }
    }


} // namespace karabo::devices
