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


#ifndef KARABO_LOG_CACHEAPPENDER_HH
#define KARABO_LOG_CACHEAPPENDER_HH

#include <krb_log4cpp/LayoutAppender.hh>
#include <krb_log4cpp/PatternLayout.hh>
#include <string>

#include "karabo/util/Configurator.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/Schema.hh"


namespace karabo {

    namespace log {

        class LogCache : public boost::enable_shared_from_this<LogCache> {
           public:
            // Registering as a karabo class to be able to use 'Pointer' mechanics
            KARABO_CLASSINFO(LogCache, "LogCache", "")

           private:
            LogCache(unsigned int maxMessages);
            // private static initializer to be called once under `m_instanceFlag`
            static void init(unsigned int maxMessages);

           public:
            /**
             * Return a pointer to a singleton instance of LogCache.
             * If no instance exists one is created.
             * @return
             */
            static Pointer getInstance(unsigned int maxMessages = 0);

            void setSize(unsigned int maxMessages);

            std::vector<karabo::util::Hash> getCachedContent(unsigned int numMessages);
            void append(const karabo::util::Hash& entry);

           private:
            // cache for messages
            boost::mutex m_mutex;
            std::atomic<unsigned int> m_maxMessages;
            std::vector<karabo::util::Hash> m_logCache;

            static Pointer m_instance;
            static std::once_flag m_instanceFlag;
        };

        /** This class is implements an appender that will store logs into
         *  a local static cache.
         */
        class Log4CppCacheApp : public krb_log4cpp::LayoutAppender {
           public:
            Log4CppCacheApp();

            virtual ~Log4CppCacheApp() {}

            // required from LayoutAppender/AppenderSkeleton
            virtual void close() {}
            virtual bool reopen();

           protected:
            virtual void _append(const krb_log4cpp::LoggingEvent& event);

           private:
            // a layout for each component
            krb_log4cpp::PatternLayout m_timeLayout;
            krb_log4cpp::PatternLayout m_priorityLayout;
            krb_log4cpp::PatternLayout m_categoryLayout;
            krb_log4cpp::PatternLayout m_messageLayout;
        };

        /**
         * Helper class to configure an underlying log4cpp appender.
         * NOTE: Do NOT use this class directly. It is indirectly involved by the static functions
         * of the Logger!!
         */
        class CacheAppender {
           public:
            KARABO_CLASSINFO(CacheAppender, "CacheAppender", "")

            static void expectedParameters(karabo::util::Schema& expected);

            CacheAppender(const karabo::util::Hash& input);

            virtual ~CacheAppender() {}

            krb_log4cpp::Appender* getAppender();

            // returns a vector of hashes containing the cached content
            // or an empty vector if the cache does not exist.
            static std::vector<karabo::util::Hash> getCachedContent(unsigned int numMessages);

           private:
            Log4CppCacheApp* m_appender;
        };
    } // namespace log
} // namespace karabo

#endif // KARABO_LOG_CACHEAPPENDER_HH
