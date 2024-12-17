/*
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
#include "RollingWindowStatistics.hh"

namespace karabo {
    namespace util {


        RollingWindowStatistics::RollingWindowStatistics(unsigned int evalInterval)
            : m_meanEstimate(0.),
              m_evalInterval(evalInterval),
              m_nvals(0),
              m_s(0.),
              m_s2(0.),
              m_vals(evalInterval, 0.){};


        RollingWindowStatistics::~RollingWindowStatistics() {}


        double RollingWindowStatistics::getRollingWindowVariance() const {
            std::shared_lock<std::shared_mutex> lock(m_updateMutex);
            unsigned long long n = std::min(m_evalInterval, m_nvals);
            return (m_s2 - m_s * m_s / n) / (n - 1);
        }


        double RollingWindowStatistics::getRollingWindowMean() const {
            std::shared_lock<std::shared_mutex> lock(m_updateMutex);
            unsigned long long n = std::min(m_evalInterval, m_nvals);
            return m_s / n + m_meanEstimate;
        }


        void RollingWindowStatistics::update(double v) {
            const double varBeforeUpdate = getRollingWindowVariance();
            std::unique_lock<std::shared_mutex> lock(m_updateMutex);
            if (m_nvals == 0) {
                m_meanEstimate = v;
            }
            const unsigned int index = m_nvals % m_evalInterval;
            const double vOldest = m_vals[index];
            m_vals[index] = v;
            const double diffCurrent = v - m_meanEstimate;
            const double diffOldest = (m_nvals >= m_evalInterval ? vOldest - m_meanEstimate : 0.);
            m_s += (diffCurrent - diffOldest);
            m_s2 += (diffCurrent * diffCurrent - diffOldest * diffOldest);
            ++m_nvals;

            lock.unlock();
            const double currentMean = getRollingWindowMean();
            if ((currentMean - m_meanEstimate) * (currentMean - m_meanEstimate) / varBeforeUpdate > 25)
                updateEstimate(currentMean); // we update if we are 5sigma off
        }


        void RollingWindowStatistics::updateEstimate(const double currentMean) {
            // we need to go through all data in current estimate
            std::unique_lock<std::shared_mutex> lock(m_updateMutex);
            m_s = 0.;
            m_s2 = 0.;
            m_meanEstimate = currentMean;
            const unsigned int n = std::min(m_evalInterval, m_nvals);
            for (unsigned int i = 0; i < n; i++) {
                const double v = m_vals[i];
                const double diff = v - m_meanEstimate;
                m_s += diff;
                m_s2 += diff * diff;
            }
        }

        unsigned long long RollingWindowStatistics::getInterval() const {
            return m_evalInterval;
        }


    } // namespace util
} // namespace karabo
