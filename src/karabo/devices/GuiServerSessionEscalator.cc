/*
 * GuiServerSessionEscalator.cc
 *
 * Manages temporary access level escalations for user-authenticated GUI Server
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

#include "GuiServerSessionEscalator.hh"

#include <boost/asio/placeholders.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <vector>

#include "karabo/net/EventLoop.hh"
#include "karabo/util/MetaTools.hh"
#include "karabo/util/TimeDuration.hh"

using namespace karabo::util;
using namespace karabo::net;
using namespace boost::placeholders;

namespace karabo::devices {

    GuiServerSessionEscalator::GuiServerSessionEscalator(const std::string& topic, const std::string& authServerUrl,
                                                         unsigned int escalationDurationSeconds,
                                                         unsigned int escalationEndNoticeSeconds,
                                                         EminentExpirationHandler onEminentExpiration,
                                                         ExpirationHandler onExpiration)
        : m_topic(topic),
          m_authClient(authServerUrl),
          m_escalationDurationSecs(escalationDurationSeconds),
          m_escalationEndNoticeSecs{TimeDuration(escalationEndNoticeSeconds, 0ULL)},
          m_eminentExpirationHandler(std::move(onEminentExpiration)),
          m_expirationHandler(std::move(onExpiration)),
          m_checkExpirationsTimer(EventLoop::getIOService()),
          m_expirationTimerWaiting(false) {}

    void GuiServerSessionEscalator::escalate(const std::string& escalationToken,
                                             const EscalationHandler& onEscalation) {
        m_authClient.authorizeOneTimeToken(
              escalationToken, m_topic,
              bind_weak(&GuiServerSessionEscalator::onTokenAuthorizeResult, this, escalationToken, onEscalation, _1));
    }

    void GuiServerSessionEscalator::scheduleNextExpirationsCheck() {
        if (m_escalations.size() > 0 && !m_expirationTimerWaiting.exchange(true)) {
            m_checkExpirationsTimer.expires_from_now(
                  boost::posix_time::seconds(CHECK_ESCALATE_EXPIRATION_INTERVAL_SECS));
            m_checkExpirationsTimer.async_wait(bind_weak(&GuiServerSessionEscalator::checkEscalationsExpirations, this,
                                                         boost::asio::placeholders::error));
        }
    }

    void GuiServerSessionEscalator::onTokenAuthorizeResult(const std::string& escalationToken,
                                                           const EscalationHandler& onEscalation,
                                                           const OneTimeTokenAuthorizeResult& authResult) {
        EscalateResult escalateResult;
        escalateResult.success = authResult.success;
        escalateResult.accessLevel = authResult.accessLevel;
        escalateResult.userId = authResult.userId;
        escalateResult.errMsg = authResult.errMsg;
        escalateResult.escalationToken = escalationToken;
        escalateResult.escalationDurationSecs = m_escalationDurationSecs;
        if (authResult.success) {
            Epochstamp currTime;
            Epochstamp expiresAt = currTime + TimeDuration(m_escalationDurationSecs, 0ULL);
            if (escalateResult.accessLevel > MAX_ESCALATED_ACCESS_LEVEL) {
                // The access level returned by the authorize token operation is more privileged
                // than the one set to be used for the escalation level - "truncate" it.
                escalateResult.accessLevel = MAX_ESCALATED_ACCESS_LEVEL;
                // Note: if the access level returned by the authorize token operation is less privileged, keep it as
                // the escalated level. As the authorize token operation takes into account the LDAP groups
                // memberships of the user, it shouldn't be simply ignored.
            }
            std::lock_guard<std::mutex> lock(m_escalationsMutex);
            m_escalations.emplace(escalationToken, expiresAt);
            scheduleNextExpirationsCheck();
        }
        onEscalation(escalateResult);
    }

    DeescalationResult GuiServerSessionEscalator::deescalate(const std::string& escalationToken) {
        DeescalationResult result;
        result.escalationToken = escalationToken;
        std::lock_guard<std::mutex> lock(m_escalationsMutex);
        auto it = m_escalations.find(escalationToken);
        if (it == m_escalations.end()) {
            result.success = false;
            result.errMsg = "Escalation token not found";
        } else {
            result.success = true;
            result.errMsg = "";
            m_escalations.erase(it);
        }
        return result;
    }

    void GuiServerSessionEscalator::checkEscalationsExpirations(const boost::system::error_code& error) {
        m_expirationTimerWaiting = false;
        if (error) {
            // Timer has been cancelled.
            return;
        }
        std::vector<ExpiredEscalationInfo> expiredInfos;
        std::vector<EminentExpirationInfo> eminentInfos;
        {
            std::lock_guard<std::mutex> lock(m_escalationsMutex);
            std::vector<std::string> tokens;
            tokens.reserve(m_escalations.size());
            for (const auto& [token, expTime] : m_escalations) {
                tokens.emplace_back(token);
            }
            Epochstamp currentTime;
            for (const std::string& token : tokens) {
                if (currentTime >= m_escalations[token]) {
                    // Escalation has expired
                    expiredInfos.emplace_back(ExpiredEscalationInfo{token, m_escalations[token]});
                    m_escalations.erase(token);
                } else if (currentTime >= m_escalations[token] - m_escalationEndNoticeSecs) {
                    // Escalation expiration occurs inside the eminent expiration time window
                    eminentInfos.emplace_back(EminentExpirationInfo{token, m_escalations[token] - currentTime});
                }
            }
        }
        scheduleNextExpirationsCheck();
        for (const ExpiredEscalationInfo& expired : expiredInfos) {
            m_expirationHandler(expired);
        }
        for (const EminentExpirationInfo& eminent : eminentInfos) {
            m_eminentExpirationHandler(eminent);
        }
    }


} // namespace karabo::devices
