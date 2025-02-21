/*
 * GuiServerTemporarySessionManager.cc
 *
 * Manages temporary sessions for user-authenticated GUI Server
 * sessions.
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

#include "GuiServerTemporarySessionManager.hh"

#include <boost/asio/placeholders.hpp>
#include <chrono>
#include <vector>

#include "karabo/net/EventLoop.hh"
#include "karabo/util/MetaTools.hh"
#include "karabo/util/TimeDuration.hh"

using namespace std::chrono;
using namespace karabo::util;
using namespace karabo::net;
using namespace std::placeholders;

namespace karabo::devices {

    GuiServerTemporarySessionManager::GuiServerTemporarySessionManager(const std::string& topic,
                                                                       const std::string& authServerUrl,
                                                                       unsigned int temporarySessionDurationSeconds,
                                                                       unsigned int temporarySessionEndNoticeSeconds,
                                                                       EminentExpirationHandler onEminentExpiration,
                                                                       ExpirationHandler onExpiration)
        : m_topic(topic),
          m_authClient(authServerUrl),
          m_temporarySessionDurationSecs(temporarySessionDurationSeconds),
          m_temporarySessionEndNoticeSecs{TimeDuration(temporarySessionEndNoticeSeconds, 0ULL)},
          m_eminentExpirationHandler(std::move(onEminentExpiration)),
          m_expirationHandler(std::move(onExpiration)),
          m_checkExpirationsTimer(EventLoop::getIOService()),
          m_expirationTimerWaiting(false) {}

    void GuiServerTemporarySessionManager::beginTemporarySession(
          const std::string& temporarySessionToken, const BeginTemporarySessionHandler& onBeginTemporarySession) {
        m_authClient.authorizeOneTimeToken(temporarySessionToken, m_topic,
                                           bind_weak(&GuiServerTemporarySessionManager::onTokenAuthorizeResult, this,
                                                     temporarySessionToken, onBeginTemporarySession, _1));
    }

    void GuiServerTemporarySessionManager::scheduleNextExpirationsCheck() {
        if (m_tempSessions.size() > 0 && !m_expirationTimerWaiting.exchange(true)) {
            m_checkExpirationsTimer.expires_after(seconds(CHECK_TEMPSESSION_EXPIRATION_INTERVAL_SECS));
            m_checkExpirationsTimer.async_wait(
                  bind_weak(&GuiServerTemporarySessionManager::checkTemporarySessionsExpirations, this,
                            boost::asio::placeholders::error));
        }
    }

    void GuiServerTemporarySessionManager::onTokenAuthorizeResult(
          const std::string& temporarySessionToken, const BeginTemporarySessionHandler& onBeginTemporarySession,
          const OneTimeTokenAuthorizeResult& authResult) {
        BeginTemporarySessionResult tempSessionResult;
        tempSessionResult.success = authResult.success;
        tempSessionResult.accessLevel = authResult.accessLevel;
        tempSessionResult.userId = authResult.userId;
        tempSessionResult.errMsg = authResult.errMsg;
        tempSessionResult.temporarySessionToken = temporarySessionToken;
        tempSessionResult.temporarySessionDurationSecs = m_temporarySessionDurationSecs;
        if (authResult.success) {
            Epochstamp currTime;
            Epochstamp expiresAt = currTime + TimeDuration(m_temporarySessionDurationSecs, 0ULL);
            if (tempSessionResult.accessLevel > MAX_TEMPORARY_SESSION_ACCESS_LEVEL) {
                // The access level returned by the authorize token operation is more privileged
                // than the one set to be used for the temporary session level - "truncate" it.
                tempSessionResult.accessLevel = MAX_TEMPORARY_SESSION_ACCESS_LEVEL;
                // Note: if the access level returned by the authorize token operation is less privileged, keep it as
                // the higher level. As the authorize token operation takes into account the LDAP groups
                // memberships of the user, it shouldn't be simply ignored.
            }
            std::lock_guard<std::mutex> lock(m_tempSessionsMutex);
            m_tempSessions.emplace(temporarySessionToken, expiresAt);
            scheduleNextExpirationsCheck();
        }
        onBeginTemporarySession(tempSessionResult);
    }

    EndTemporarySessionResult GuiServerTemporarySessionManager::endTemporarySession(
          const std::string& temporarySessionToken) {
        EndTemporarySessionResult result;
        result.temporarySessionToken = temporarySessionToken;
        std::lock_guard<std::mutex> lock(m_tempSessionsMutex);
        auto it = m_tempSessions.find(temporarySessionToken);
        if (it == m_tempSessions.end()) {
            result.success = false;
            result.errMsg = "Temporary Session token not found";
        } else {
            result.success = true;
            result.errMsg = "";
            m_tempSessions.erase(it);
        }
        return result;
    }

    void GuiServerTemporarySessionManager::checkTemporarySessionsExpirations(const boost::system::error_code& error) {
        m_expirationTimerWaiting = false;
        if (error) {
            // Timer has been cancelled.
            return;
        }
        std::vector<ExpiredTemporarySessionInfo> expiredInfos;
        std::vector<EminentExpirationInfo> eminentInfos;
        {
            std::lock_guard<std::mutex> lock(m_tempSessionsMutex);
            std::vector<std::string> tokens;
            tokens.reserve(m_tempSessions.size());
            for (const auto& [token, expTime] : m_tempSessions) {
                tokens.emplace_back(token);
            }
            Epochstamp currentTime;
            for (const std::string& token : tokens) {
                if (currentTime >= m_tempSessions[token]) {
                    // Temporary session has expired
                    expiredInfos.emplace_back(ExpiredTemporarySessionInfo{token, m_tempSessions[token]});
                    m_tempSessions.erase(token);
                } else if (currentTime >= m_tempSessions[token] - m_temporarySessionEndNoticeSecs) {
                    // Temporary session expiration occurs inside the eminent expiration time window
                    eminentInfos.emplace_back(EminentExpirationInfo{token, m_tempSessions[token] - currentTime});
                }
            }
        }
        scheduleNextExpirationsCheck();
        for (const ExpiredTemporarySessionInfo& expired : expiredInfos) {
            m_expirationHandler(expired);
        }
        for (const EminentExpirationInfo& eminent : eminentInfos) {
            m_eminentExpirationHandler(eminent);
        }
    }


} // namespace karabo::devices
