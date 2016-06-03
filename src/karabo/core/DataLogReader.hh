/* 
 * File:   DataLogReader.hh
 * Author: Sergey Esenov serguei.essenov@xfel.eu
 *
 * Created on January 29, 2015, 11:04 AM
 */

#ifndef DATALOGREADER_HH
#define	DATALOGREADER_HH

#include <boost/filesystem.hpp>
#include "Device.hh"
#include "OkErrorFsm.hh"
#include "DataLogUtils.hh"


/**
 * The main karabo namespace
 */
namespace karabo {

    /**
     * Namespace for package core
     */
    namespace core {

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
        
        
        struct PropFileInfo {
            typedef boost::shared_ptr<PropFileInfo> Pointer;
            boost::mutex filelock;
            size_t filesize;
            time_t lastwrite;
            std::vector<std::string> properties;
            PropFileInfo() : filelock(), filesize(0), lastwrite(0), properties() {}
        };
        
        
        class IndexBuilderService {
        public:
            // Needed for 'Pointer' and KARABO_LOG_FRAMEWORK
            KARABO_CLASSINFO(IndexBuilderService, "IndexBuilderService", "1.4")

        private:
            static Pointer m_instance;
            
            boost::shared_ptr<boost::asio::io_service> m_svc;
            
            boost::asio::io_service::work m_work;
            
            std::set<std::string> m_cache;
            
            boost::thread m_thread;
            
            boost::mutex m_mutex;
        
            IndexBuilderService();
            
        public:
            
            static Pointer getInstance();
           
            // Virtual destructor needed since KARABO_CLASSINFO adds virtual methods:
            virtual ~IndexBuilderService();

            void buildIndexFor(const std::string& commandLineArguments);
            
        private:
            
            void build(const std::string& args);
            
        };
        

        
        class DataLogReader : public karabo::core::Device<karabo::core::OkErrorFsm> {
        public:

            KARABO_CLASSINFO(DataLogReader, "DataLogReader", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            DataLogReader(const karabo::util::Hash& input);

            virtual ~DataLogReader();

        private: // Functions

            void okStateOnEntry();

            void slotGetPropertyHistory(const std::string& deviceId, const std::string& property, const karabo::util::Hash& params);

            void slotGetConfigurationFromPast(const std::string& deviceId, const std::string& timepoint);

            DataLoggerIndex findLoggerIndexTimepoint(const std::string& deviceId, const std::string& timepoint);

            /// Find logger closest index from archive_index.txt file that is before/after (according to 'before')
            /// 'timepoint'. If there is none before (after) but that is asked for, take the one just after (before).
            DataLoggerIndex findNearestLoggerIndex(const std::string& deviceId, const karabo::util::Epochstamp& timepoint, const bool before);

            int getFileIndex(const std::string& deviceId);

            MetaSearchResult navigateMetaRange(const std::string& deviceId, size_t startnum, size_t endnum, const std::string& path,
                                               const karabo::util::Epochstamp& from, const karabo::util::Epochstamp& to);

            /// Find index of that MetaData::Record in 'f' (between indices 'left' and 'right')
            /// that matches the Epochstamp 'stamp'. In case no exact match (within 1 ms) is found,
            /// 'preferBefore' decides whether the index with a smaller or larger time stamp is returned.
            size_t findPositionOfEpochstamp(std::ifstream& f, double stamp, size_t left, size_t right, bool preferBefore);
            
        private:
            
            static boost::mutex m_propFileInfoMutex;
            static std::map<std::string, PropFileInfo::Pointer > m_mapPropFileInfo;
            IndexBuilderService::Pointer m_ibs;
        };  
    }
}

#endif	/* DATALOGREADER_HH */

