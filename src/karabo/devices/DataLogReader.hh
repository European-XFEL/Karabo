/*
 * File:   DataLogReader.hh
 * Author: Sergey Esenov serguei.essenov@xfel.eu
 *
 * Created on January 29, 2015, 11:04 AM
 */

#ifndef DATALOGREADER_HH
#define	DATALOGREADER_HH

#include <boost/filesystem.hpp>

#include "karabo/core/Device.hh"
#include "karabo/core/OkErrorFsm.hh"
#include "karabo/util/DataLogUtils.hh"
#include "karabo/net/Strand.hh"



/**
 * The main karabo namespace
 */
namespace karabo {

    /**
     * Namespace for package core
     */
    namespace devices {

        /**
         * @struct DataLoggerIndex
         * @brief A compound for representing indexes in logged data
         */
        struct DataLoggerIndex {

            std::string m_event;
            karabo::util::Epochstamp m_epoch;
            unsigned long long m_train;
            long m_position;
            std::string m_user;
            int m_fileindex;

            DataLoggerIndex()
                : m_event()
                , m_epoch(0, 0)
                , m_train(0)
                , m_position(-1)
                , m_user(".")
                , m_fileindex(-1) {
            }
        };

        /**
         * @struct PropFileInfo
         * @brief A compound structure holding data on an logger archive file
         */
        struct PropFileInfo {

            typedef boost::shared_ptr<PropFileInfo> Pointer;
            boost::mutex filelock;
            size_t filesize;
            time_t lastwrite;
            std::vector<std::string> properties;

            PropFileInfo() : filelock(), filesize(0), lastwrite(0), properties() {
            }
        };


        /**
         * @class IndexBuilderService
         * @brief A singleton class for building logger indices from logger files. It calls
         *    karabo-idxbuild with a list of command line arguments 
         */
        class IndexBuilderService : public boost::enable_shared_from_this<IndexBuilderService> {

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
            boost::mutex m_mutex;
            karabo::net::Strand::Pointer m_idxBuildStrand;
        };

        /**
         * @class DataLogReader
         * @brief DataLogReader devices read archived information from the Karabo data loggers
         * 
         * DataLogReader devices read archived information from the Karabo data loggers. They
         * are managed by karabo::devices::DataLoggerManager devices. Calls to them should usually
         * not happen directly, but rather through a karabo::core::DeviceClient and it's 
         * karabo::core::DeviceClient::getPropertyHistory and karabo::core::DeviceClient::getConfigurationFromPast
         * methods.
         * 
         */
        class DataLogReader : public karabo::core::Device<karabo::core::OkErrorFsm> {

        public:

            KARABO_CLASSINFO(DataLogReader, "DataLogReader", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            DataLogReader(const karabo::util::Hash& input);

            virtual ~DataLogReader();

        private: // Functions

            void okStateOnEntry();

            /**
             * Use this slot to get the history of a given property
             * @param deviceId for which to get the history for
             * @param property path to the property for which to get the history from
             * @param params Hash containing optional limitations for the query:
             *   - from: iso8601 timestamp indicating the start of the interval to get history from
             *   - to: iso8601 timestamp indicating the end of the interval to get history from
             *   - maxNumData: maximum number of data points to retrieve starting from start
             * 
             * The slot replies a vector of Hashes where each entry consists of a Hash with a key "v" holding the value
             * of the property at timepoint signified in "v"'s attributes using a format compatible with
             * karabo::util::Timestamp::fromHashAttributes
             */
            void slotGetPropertyHistory(const std::string& deviceId, const std::string& property, const karabo::util::Hash& params);

            /**
             * Request the configuration Hash and schema of a device at a given point at time. The configuration
             * closes archived closest to timepoint is returned.
             * @param deviceId of the device to get the configuration from
             * @param timepoint in iso8601 format for which to get the information
             * 
             * The slot replies a std::pair of Hash and Schema objects containing the configuration Hash and the 
             * corresponding device schema for timepoint.
             */
            void slotGetConfigurationFromPast(const std::string& deviceId, const std::string& timepoint);

            DataLoggerIndex findLoggerIndexTimepoint(const std::string& deviceId, const std::string& timepoint);

            /// Find logger closest index from archive_index.txt file that is before/after (according to 'before')
            /// 'timepoint'. If there is none before (after) but that is asked for, take the one just after (before).
            DataLoggerIndex findNearestLoggerIndex(const std::string& deviceId, const karabo::util::Epochstamp& timepoint, const bool before);

            int getFileIndex(const std::string& deviceId);

            karabo::util::MetaSearchResult navigateMetaRange(const std::string& deviceId, size_t startnum, size_t endnum, const std::string& path,
                                                             const karabo::util::Epochstamp& from, const karabo::util::Epochstamp& to);

            /// Find index of that MetaData::Record in 'f' (between indices 'left' and 'right')
            /// that matches the Epochstamp 'stamp'. In case no exact match (within 1 ms) is found,
            /// 'preferBefore' decides whether the index with a smaller or larger time stamp is returned.
            size_t findPositionOfEpochstamp(std::ifstream& f, double stamp, size_t left, size_t right, bool preferBefore);

            /// Helper to extract DataLoggerIndex values out of the tail of a line in archive_index.txt.
            /// The tail is everything after event, timestampAsIso8061 and timestampAsDouble.
            /// The entry has to be partly filled (m_event and m_epoch) and partly serves as output
            /// (m_train, m_position, m_user and m_fileindex).
            /// Works for lines written to archive_index.txt by >= 1.5
            void extractTailOfArchiveIndex(const std::string& tail, DataLoggerIndex& entry) const;

        private:

            static boost::mutex m_propFileInfoMutex;
            static std::map<std::string, PropFileInfo::Pointer > m_mapPropFileInfo;
            IndexBuilderService::Pointer m_ibs;
            karabo::io::TextSerializer<karabo::util::Hash>::Pointer m_serializer;
            karabo::io::TextSerializer<karabo::util::Schema>::Pointer m_schemaSerializer;
            static const boost::regex m_lineRegex;
            static const boost::regex m_indexLineRegex;
            static const boost::regex m_indexTailRegex;
        };
    }
}

#endif	/* DATALOGREADER_HH */

