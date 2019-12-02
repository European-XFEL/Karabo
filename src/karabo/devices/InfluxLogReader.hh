/*
 * File:   FileLogReader.hh
 * Author: <raul.costa@xfel.eu>
 *
 * Created on November 8, 2019, 3:40 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef INFLUXLOGREADER_HH
#define	INFLUXLOGREADER_HH

#include <string>

#include <boost/regex.hpp>
#include <boost/thread/mutex.hpp>

#include "karabo/util/ClassInfo.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/Schema.hh"

#include "DataLogReader.hh"

namespace karabo {

    namespace devices {

        class InfluxLogReader : public DataLogReader {

        public:

            KARABO_CLASSINFO(InfluxLogReader, "InfluxLogReader", "1.0")

            static void expectedParameters(karabo::util::Schema &expected);

            InfluxLogReader(const karabo::util::Hash &input);

            virtual ~InfluxLogReader();

        protected:

            virtual void slotGetPropertyHistory(const std::string &deviceId,
                                                const std::string &property,
                                                const karabo::util::Hash &params) override;

            virtual void slotGetConfigurationFromPast(const std::string &deviceId,
                                                      const std::string &timepoint) override;

        private:

        };

    } // namespace devices

} // namespace karabo

#endif	/* INFLUXLOGREADER_HH */
