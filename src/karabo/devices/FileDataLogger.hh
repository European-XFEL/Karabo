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

#ifndef KARABO_DEVICES_FILEDATALOGGER_HH
#define KARABO_DEVICES_FILEDATALOGGER_HH

#include <filesystem>

#include "DataLogger.hh"
#include "karabo/io/TextSerializer.hh"
#include "karabo/util/Version.hh"

namespace karabo {

    namespace devices {

        struct FileDeviceData : public karabo::devices::DeviceData {
            KARABO_CLASSINFO(FileDeviceData, "FileDataLoggerDeviceData", "2.6")

            FileDeviceData(const karabo::util::Hash& input);

            virtual ~FileDeviceData();

            void handleChanged(const karabo::util::Hash& config, const std::string& user) override;

            void logValue(const std::string& deviceId, const std::string& path, const karabo::util::Timestamp& ts,
                          const std::string& value, const std::string& type, size_t filePosition);

            void flushOne();

            /// Helper function to update data.m_idxprops, returns whether data.m_idxprops changed.
            bool updatePropsToIndex();

            /// Helper to ensure archive file is closed.
            /// Must only be called from functions posted on 'data.m_strand'.
            void ensureFileClosed();

            /** Helper to ensure archive file (m_configStream) is open.
             *  Must only be called from functions posted on 'data.m_strand'.
             *
             *  @return pair of * whether it is a new file (in contrast to a re-opened existing one)
             *                  * current file position, size_t(-1) tells that file could not be opened (permissions?)
             */
            std::pair<bool, size_t> ensureFileOpen();

            int determineLastIndex(const std::string& deviceId) const;

            int incrementLastIndex(const std::string& deviceId);

            void handleSchemaUpdated(const karabo::util::Schema& schema, const karabo::util::Timestamp& stamp) override;

            void setupDirectory();

            std::string m_directory;
            int m_maxFileSize;
            std::fstream m_configStream;

            unsigned int m_lastIndex;

            std::map<std::string, karabo::util::MetaData::Pointer> m_idxMap;
            std::vector<std::string> m_idxprops;
            size_t m_propsize;
            std::filesystem::file_time_type m_lasttime;

            karabo::io::TextSerializer<karabo::util::Hash>::Pointer m_serializer;
        };


        class FileDataLogger : public karabo::devices::DataLogger {
           public:
            KARABO_CLASSINFO(FileDataLogger, "FileDataLogger", "karabo-" + karabo::util::Version::getVersion())

            static void expectedParameters(karabo::util::Schema& expected);

            FileDataLogger(const karabo::util::Hash& input);

            virtual ~FileDataLogger();

           private:
            DeviceData::Pointer createDeviceData(const karabo::util::Hash& config) override;

            void flushImpl(const std::shared_ptr<SignalSlotable::AsyncReply>& aReplyPtr) override;
        };
    } // namespace devices
} // namespace karabo

#endif /* KARABO_DEVICES_FILEDATALOGGER_HH */
