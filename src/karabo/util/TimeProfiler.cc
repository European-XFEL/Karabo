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
/*
 * File:   TimeProfiler.cc
 * Author: boukhelef
 *
 * Created on June 18, 2013, 9:21 PM
 */

#include "TimeProfiler.hh"

#include "karabo/data/types/Exception.hh"
#include "karabo/data/types/Hash.hh"

using namespace std;
using karabo::data::Epochstamp;
using karabo::data::Hash;
using karabo::data::TimePeriod;

namespace karabo {
    namespace util {


        TimeProfiler::TimeProfiler(const std::string& name) : m_name(name) {}


        TimeProfiler::~TimeProfiler() {}

        TimeProfiler::TimeProfiler(const karabo::data::Hash& hash) try
            : m_name(hash.get<string>("KRB_name")), m_periods(hash) {
        } catch (karabo::data::Exception& e) {
            m_name = "Profiler";
            m_periods = hash;
        }


        TimeProfiler::operator karabo::data::Hash() {
            return m_periods;
        }


        void TimeProfiler::open() {
            m_stack = std::stack<Hash*>();
            m_periods = Hash("KRB_name", m_name, "KRB_start", "", "KRB_details", vector<Hash>());
            Hash::Attributes now;
            Epochstamp().toHashAttributes(now);
            m_periods.setAttributes("KRB_start", now);
            m_stack.push(&m_periods);
        }


        void TimeProfiler::close() {
            Hash::Attributes now;
            Epochstamp().toHashAttributes(now);
            while (!m_stack.empty()) {
                Hash* current = m_stack.top();
                vector<Hash>& details = current->get<vector<Hash> >("KRB_details");
                if (!details.back().has("KRB_stop")) {
                    details.back().set("KRB_stop", "");
                    details.back().setAttributes("KRB_stop", now);
                }
                if (!current->has("KRB_stop")) {
                    current->set("KRB_stop", "");
                    current->setAttributes("KRB_stop", now);
                }
                m_stack.pop();
            }
            m_periods.set("KRB_stop", "");
            m_periods.setAttributes("KRB_stop", now);

            compact(m_periods);
        }


        void TimeProfiler::startPeriod() {
            Hash* current = m_stack.top();
            Hash::Attributes now;
            Epochstamp().toHashAttributes(now);
            vector<Hash>& details = current->get<vector<Hash> >("KRB_details");
            if (!details.empty()) {
                details.back().set("KRB_stop", "");
                details.back().setAttributes("KRB_stop", now);
            }
            details.push_back(Hash("KRB_start", "", "KRB_details", vector<Hash>()));
            details.back().setAttributes("KRB_start", now);
        }


        void TimeProfiler::startPeriod(const std::string& periodname) {
            Hash* current = m_stack.top();
            Hash::Attributes now;
            Epochstamp().toHashAttributes(now);
            vector<Hash>& details = current->get<vector<Hash> >("KRB_details");
            if (!details.empty()) {
                details.back().set("KRB_stop", "");
                details.back().setAttributes("KRB_stop", now);
            }
            details.push_back(Hash("KRB_name", periodname, "KRB_start", "", "KRB_details", vector<Hash>()));
            Epochstamp().toHashAttributes(now);
            details.back().setAttributes("KRB_start", now);
            m_stack.push(&details.back());
        }


        void TimeProfiler::stopPeriod() {
            Hash::Attributes now;
            Epochstamp().toHashAttributes(now);
            while (!m_stack.empty()) {
                Hash* current = m_stack.top();
                vector<Hash>& details = current->get<vector<Hash> >("KRB_details");
                if (details.empty()) {
                    current->set("KRB_stop", "");
                    current->setAttributes("KRB_stop", now);
                    m_stack.pop();
                    break;
                } else {
                    if (!details.back().has("KRB_stop")) {
                        details.back().set("KRB_stop", "");
                        details.back().setAttributes("KRB_stop", now);
                        break;
                    }
                }
                m_stack.pop();
            }
        }


        void TimeProfiler::stopPeriod(const std::string& periodname) {
            Hash::Attributes now;
            Epochstamp().toHashAttributes(now);

            while (!m_stack.empty()) {
                Hash* current = m_stack.top();
                current->set("KRB_stop", "");
                current->setAttributes("KRB_stop", now);
                vector<Hash>& details = current->get<vector<Hash> >("KRB_details");
                if (!details.empty()) {
                    details.back().set("KRB_stop", "");
                    details.back().setAttributes("KRB_stop", now);
                }
                m_stack.pop();
                if (current->get<string>("KRB_name") == periodname) break;
            }
        }


        void TimeProfiler::compact(karabo::data::Hash& period) {
            vector<Hash>& details = period.get<vector<Hash> >("KRB_details");
            if (!details.empty()) {
                karabo::data::TimeDuration td;
                for (size_t i = 0; i < details.size(); ++i) {
                    // if (details[i].get<string>("KRB_name").empty()) {
                    if (!details[i].has("KRB_name")) {
                        td += TimePeriod(details[i]).getDuration();
                    } else {
                        compact(details[i]);
                        const std::string name = details[i].get<string>("KRB_name");
                        details[i].erase("KRB_name");
                        period.set(name, details[i]);
                    }
                }
                period.set("KRB_details", Hash(td));
            } else {
                period.erase("KRB_details");
            }
            TimePeriod(period).getDuration().toHash(period.bindReference<Hash>("KRB_duration"));
        }


        const TimePeriod TimeProfiler::getPeriod(const std::string& periodname) const {
            return TimePeriod(m_periods.get<Hash>(periodname));
        }


        const TimePeriod TimeProfiler::getPeriod() const {
            return TimePeriod(m_periods);
        }


        const Hash& TimeProfiler::getPeriodAsHash(const std::string& periodname) const {
            return m_periods.get<Hash>(periodname);
        }


        const Hash& TimeProfiler::getPeriodAsHash() const {
            return m_periods;
        }

        // Serialize the profile into standard ostream


        std::string TimeProfiler::format(const std::string& fmt, int level) const {
            std::ostringstream oss;
            serialize(oss, level);
            return oss.str();
        }


        std::ostream& operator<<(std::ostream& os, const TimeProfiler& profiler) {
            profiler.serialize(os);
            return os;
        }


        void TimeProfiler::serialize(std::ostream& os, int level) const {
            os << getPeriodAsHash() << std::endl;
        }

        static int key = 0;


        std::string TimeProfiler::sql() const {
            std::ostringstream oss;
            oss << "INSERT INTO Profiler(key, parent, value, start-sec, start-frac, stop-sec, stop-frac, durree-sec, "
                   "durree-frac) VALUES";
            sql(oss, m_name, m_periods, -1);

            return oss.str();
        }


        void TimeProfiler::sql(std::ostream& os, const std::string& name, const karabo::data::Hash& period,
                               const int parent_key) {
            int current_key = key++;
            os << "\n('" << current_key << "','" << parent_key << "','" << name << "','"
               << period.get<unsigned long long>("KRB_start.seconds") << "','"
               << period.get<unsigned long long>("KRB_start.fractions") << "','"
               << period.get<unsigned long long>("KRB_stop.seconds") << "','"
               << period.get<unsigned long long>("KRB_stop.fractions") << "','"
               << period.get<unsigned long long>("KRB_duration.seconds") << "','"
               << period.get<unsigned long long>("KRB_duration.fractions") << "')";

            vector<string> keys;
            period.getKeys(keys);

            for (size_t i = 1; i < keys.size(); ++i) {
                if ((keys[i] == "KRB_start") || (keys[i] == "KRB_stop") || (keys[i] == "KRB_duration") ||
                    (keys[i] == "KRB_details"))
                    continue;
                sql(os, keys[i], period.get<Hash>(keys[i]), current_key);
            }
        }

    } // namespace util
} // namespace karabo
