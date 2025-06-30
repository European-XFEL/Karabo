/*
 * File:   FileLogReader.hh
 *
 * Created on November 8, 2019, 3:40 AM
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

#ifndef FILELOGREADER_HH
#define FILELOGREADER_HH

#include <boost/regex.hpp>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <string>

#include "DataLogReader.hh"
#include "karabo/data/io/TextSerializer.hh"
#include "karabo/data/types/ClassInfo.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/Schema.hh"
#include "karabo/util/Version.hh"

namespace karabo {

    namespace devices {

        using namespace std::chrono_literals;

        /**
         * @struct FileLoggerIndex
         * @brief A compound for representing indexes in text file logged data.
         */
        struct FileLoggerIndex {
            std::string m_event;
            karabo::data::Epochstamp m_epoch;
            unsigned long long m_train;
            long m_position;
            std::string m_user;
            int m_fileindex;

            FileLoggerIndex() : m_event(), m_epoch(0, 0), m_train(0), m_position(-1), m_user("."), m_fileindex(-1) {}
        };

        /**
         * @struct PropFileInfo
         * @brief A compound structure holding data on an logger archive file
         */
        struct PropFileInfo {
            typedef std::shared_ptr<PropFileInfo> Pointer;
            std::mutex filelock;
            size_t filesize;
            std::filesystem::file_time_type lastwrite;
            std::vector<std::string> properties;

            PropFileInfo() : filelock(), filesize(0), lastwrite(0h), properties() {}
        };

        /**
         * @class IndexBuilderService
         * @brief A singleton class for building logger indices from logger files. It calls
         *    karabo-idxbuild with a list of command line arguments
         */
        class IndexBuilderService : public std::enable_shared_from_this<IndexBuilderService> {
           public:
            // Needed for 'Pointer' and KARABO_LOG_FRAMEWORK
            KARABO_CLASSINFO(IndexBuilderService, "IndexBuilderService", "1.4")

           private:
            IndexBuilderService();

           public:
            /**
             * Return a pointer to a singleton instance of IndexBuilderService.
             * If no instance exists one is created.
             * @return
             */
            static Pointer getInstance();

            // Virtual destructor needed since KARABO_CLASSINFO adds virtual methods:
            virtual ~IndexBuilderService();

            /**
             * Build an index by calling karabo-idxbuild with the supplied command line arguments
             * @param commandLineArguments
             */
            void buildIndexFor(const std::string& commandLineArguments);

           private:
            void build(const std::string& args);

            /**
             * This device may not be locked
             * @return false
             */
            bool allowLock() const {
                return false;
            }

            static Pointer m_instance;
            std::set<std::string> m_cache;
            std::mutex m_mutex;
            karabo::net::Strand::Pointer m_idxBuildStrand;
        };


        /**
         * A reader for data logs stored in text files by the class
         * karabo::devices::FileDataLogger.
         */
        class FileLogReader : public DataLogReader {
           public:
            KARABO_CLASSINFO(FileLogReader, "FileLogReader", "karabo-" + karabo::util::Version::getVersion())

            static void expectedParameters(karabo::data::Schema& expected);

            FileLogReader(const karabo::data::Hash& input);

            virtual ~FileLogReader();

           protected:
            virtual void slotGetPropertyHistoryImpl(const std::string& deviceId, const std::string& property,
                                                    const karabo::data::Hash& params) override;

            virtual void slotGetConfigurationFromPastImpl(const std::string& deviceId,
                                                          const std::string& timepoint) override;

            void getConfigurationFromPast(const std::string& deviceId, const std::string& timepoint,
                                          SignalSlotable::AsyncReply& reply);

           private:
            /**
             * Internal helper:
             * Place 'value' interpreted as 'type' (and with given 'timestamp') into 'hashOut' at 'path'.
             */
            void readToHash(karabo::data::Hash& hashOut, const std::string& path,
                            const karabo::data::Timestamp& timestamp, const std::string& type,
                            const std::string& value) const;

            /**
             * Retrieves, from the logger index, the event of type "device became online" that is closest, but not after
             * a given timepoint. The retrieved logger index event can be used as a starting point for sweeping the
             * device log for the last known given configuration at that timepoint.
             *
             * @param deviceId the device whose logger index event should be retrieved.
             * @param timepoint the timepoint that will be used as the reference to find the logger index event.
             * @return a pair whose 'first' is a boolean that indicates whether configuration was active at the
             * timepoint (true) or whether it is a configuration from the most recent activation of the device prior
             * to the timepoint because the device was not active logging at the timepoint. The pair's 'second' value
             * is the logger index of the given device that is the closest "device became online" event that is not
             * after the given timepoint.
             */
            std::pair<bool, FileLoggerIndex> findLoggerIndexTimepoint(const std::string& deviceId,
                                                                      const std::string& timepoint);

            /// Find logger closest index from archive_index.txt file that is before/after (according to 'before')
            /// 'timepoint'. If there is none before (after) but that is asked for, take the one just after (before).
            FileLoggerIndex findNearestLoggerIndex(const std::string& deviceId,
                                                   const karabo::data::Epochstamp& timepoint, const bool before);

            int getFileIndex(const std::string& deviceId);

            karabo::util::MetaSearchResult navigateMetaRange(const std::string& deviceId, size_t startnum,
                                                             size_t endnum, const std::string& path,
                                                             const karabo::data::Epochstamp& from,
                                                             const karabo::data::Epochstamp& to);

            /// Find index of that MetaData::Record in 'f' (between indices 'left' and 'right')
            /// that matches the Epochstamp 'stamp'. In case no exact match (within 1 ms) is found,
            /// 'preferBefore' decides whether the index with a smaller or larger time stamp is returned.
            size_t findPositionOfEpochstamp(std::ifstream& f, double stamp, size_t left, size_t right,
                                            bool preferBefore);

            /// Helper to extract DataLoggerIndex values out of the tail of a line in archive_index.txt.
            /// The tail is everything after event, timestampAsIso8061 and timestampAsDouble.
            /// The entry has to be partly filled (m_event and m_epoch) and partly serves as output
            /// (m_train, m_position, m_user and m_fileindex).
            /// Works for lines written to archive_index.txt by >= 1.5
            void extractTailOfArchiveIndex(const std::string& tail, FileLoggerIndex& entry) const;

            static std::mutex m_propFileInfoMutex;
            static std::map<std::string, PropFileInfo::Pointer> m_mapPropFileInfo;
            IndexBuilderService::Pointer m_ibs;
            static const boost::regex m_lineRegex;
            static const boost::regex m_lineLogRegex;
            static const boost::regex m_indexLineRegex;
            static const boost::regex m_indexTailRegex;
            std::string m_ltype;
            karabo::data::TextSerializer<karabo::data::Hash>::Pointer m_serializer;
            karabo::data::TextSerializer<karabo::data::Schema>::Pointer m_schemaSerializer;
        };

    } // namespace devices

} // namespace karabo

#endif /* FILELOGREADER_HH */
