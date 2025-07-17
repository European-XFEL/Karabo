/*
 * $Id$
 *
 * Author: <gero.flucke@xfel.eu>
 *
 * Created on November 6, 2015
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

#include <boost/algorithm/string/split.hpp>
#include <cstdlib>
#include <iomanip>
#include <iosfwd>
#include <iostream>
#include <karabo/core/DeviceClient.hh>
#include <karabo/data/time/Epochstamp.hh>
#include <karabo/data/types/Hash.hh>
#include <karabo/data/types/Schema.hh>
#include <karabo/log/Logger.hh>
#include <karabo/net/AmqpConnection.hh>
#include <karabo/net/AmqpHashClient.hh>
#include <karabo/net/Broker.hh>
#include <karabo/net/EventLoop.hh>
#include <karabo/util/MetaTools.hh>
#include <map>
#include <memory>
#include <unordered_set>

using namespace karabo;
using namespace std::placeholders;

class BrokerStatistics : public std::enable_shared_from_this<BrokerStatistics> {
   public:
    /// Stats as pair of number of calls and accumulated size in bytes.
    typedef std::pair<unsigned int, size_t> Stats;

    /// SenderId as pair of instanceId and a 'target'.
    typedef std::pair<const std::string, const std::string> SenderId;

    typedef std::map<SenderId, Stats> SenderStatsMap;

    /// SlotId is single string: a ':' separates receiverId and slot.
    typedef std::string SlotId;

    typedef std::map<SlotId, Stats> SlotStatsMap;

    BrokerStatistics(std::string domain, data::TimeValue intervalSec, const std::vector<std::string>& senderIds)
        : m_domain(std::move(domain)),
          m_interval(intervalSec, 0ll),
          m_senders(senderIds.begin(), senderIds.end()),
          m_start(0ull, 0ull) {}

    // virtual ~BrokerStatistics() {} // no need for virtual...

    /// Register a message, i.e. increase statistics and possibly print.
    void registerMessage(const std::string& exchange, const std::string& routingKey, const data::Hash::Pointer& header, size_t bodySize);

   private:
    void registerPerSender(const std::string& exchange, const std::string& routingKey, const std::string& senderId, size_t bodySize);

    void registerPerSlotCall(const std::string& exchange, const std::string& routingKey, size_t bodySize);

    void registerLogMessage(size_t bodySize);

    void printStatistics(const data::Epochstamp& timeStamp, float elapsedSeconds) const;

    /// The keys of StatsMap must either support
    /// ostream& operator<<(ostream&, const KeyType&),
    /// preferably with a width of 58 characters or there must be a specialisation
    /// of BrokerStatistics::printId for IdType=KeyType.
    template <typename StatsMap>
    Stats printStatistics(const StatsMap& statsMap, const data::Epochstamp& timeStamp, float elapsedSeconds) const;

    /// Helper of printStatistics.
    template <class IdType>
    void printLine(const IdType& id, const Stats& stats, float elapsedSeconds) const;

    /// Helper of printLine (see also printStatistics).
    template <class IdType>
    void printId(std::ostream& out, const IdType& id) const;

    const std::string m_domain;
    const data::TimeDuration m_interval;
    const std::unordered_set<std::string> m_senders;
    data::Epochstamp m_start;

    /// mapping SenderId to Stats
    SenderStatsMap m_signalStats;

    /// mapping SlotId to Stats
    SlotStatsMap m_slotStats;

    static const std::string m_delimLine;
};

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

static bool debug = false;
const std::string BrokerStatistics::m_delimLine(
      "===============================================================================\n");

////////////////////////////////////////////////////////////////////////////


template <class IdType>
void BrokerStatistics::printId(std::ostream& out, const IdType& id) const {
    out << std::left << std::setw(58) << id;
}


template <>
void BrokerStatistics::printId(std::ostream& out, const SenderId& id) const {
    out << std::left << std::setw(38) << id.first << std::setw(20) << id.second;
}


template <>
void BrokerStatistics::printId(std::ostream& out, const SlotId& id) const {
    // SlotId is a string with a colon
    std::vector<std::string> deviceSlot;
    boost::split(deviceSlot, id, boost::is_any_of(":"));

    const std::string& slotName = (deviceSlot.size() >= 2 ? deviceSlot[1] : "");
    out << std::left << std::setw(38) << deviceSlot[0] << std::setw(20) << slotName;

    // Create a warning if deviceSlot.size() > 2, at least once?
}

////////////////////////////////////////////////////////////////////////////


void BrokerStatistics::registerMessage(const std::string& exchangeFull, const std::string& routingKey, const data::Hash::Pointer& header, size_t bodySize) {
    try {
        // In the very first call we reset the start time.
        // Otherwise (if the constructor initialises m_start with 'now') starting this
        // tool and then starting the first device in the topic leads to wrongly low
        // rates.
        if (!m_start.getSeconds()) m_start.now();

        // Remove redundant domain from exchange
        const bool exchangeOk = exchangeFull.starts_with(m_domain);
        const std::string& exchange = (exchangeOk ? exchangeFull.substr(m_domain.size() + 1) : exchangeFull);
        if (debug && !exchangeOk) {
            std::cerr << "Received unexpected exchange '" << exchangeFull << "'" << std::endl;
        }

        // Get who sent the message:
        const std::string& senderId = header->get<std::string>("signalInstanceId");
        // If special senders requested (i.e. m_senders non-empty), go on only for those
        if (!m_senders.empty() && m_senders.find(senderId) == m_senders.end()) {
           return;
        }

        this->registerPerSender(exchange, routingKey, senderId, bodySize);
        this->registerPerSlotCall(exchange, routingKey, bodySize);

        // Now it might be time to print and reset.
        // Since it is done inside registerMessage, one does not get any printout if
        // the watched topic is silent :-(. But then we do not need monitoring anyway.
        const data::Epochstamp now;
        const data::TimeDuration diff = now.elapsed(m_start);
        if (diff >= m_interval) {
            // Calculating in single float precision should be enough...
            const float elapsedSeconds =
                  static_cast<float>(diff.getTotalSeconds()) + diff.getFractions(data::TIME_UNITS::MICROSEC) / 1.e6f;
            this->printStatistics(now, elapsedSeconds);

            // Reset:
            m_start = now;
            m_signalStats.clear();
            m_slotStats.clear();
        }
    } catch (const std::exception& e) {
        std::cerr << "Problem registering message: " << e.what() << "\nheader:\n" << *header << std::endl;
    }
}

////////////////////////////////////////////////////////////////////////////


void BrokerStatistics::registerPerSender(const std::string& exchange, const std::string& routingKey, const std::string& senderId, size_t bodySize) {

    // Avoid repeating instanceId in routigKey for signals and global slots
    // and shorten known exchanges
    std::string reducedKey, reducedExch;
    if (exchange == "Signals" || exchange == "Global_Slots") {
        reducedExch = exchange.substr(0, 2); // just two characters
        if (routingKey.starts_with(senderId)) {
            reducedKey = routingKey.substr(senderId.size() + 1);
        } else {
            reducedKey = routingKey;
            if (debug) {
                std::cerr << "Unexpected routing key in message from '" << senderId << "': " << routingKey << std::endl;
            }
        }
    } else {
        if (exchange == "Slots") {
            reducedExch = exchange.substr(0, 2); // just two characters
        } else {
            reducedExch = exchange;
            if (debug) {
                std::cerr << "Unexpected exchange in message from '" << senderId << "': " << exchange << std::endl;
            }
        }
        reducedKey = routingKey;
    }

#if __cpp_lib_format
    const std::string target(std::format("{} {}", reducedExch, reducedKey));
#else
    const std::string target((reducedExch + " ") += reducedKey);
#endif

    // Find signal ID in map and increase statistics.
    const SenderId key(senderId, target);
    Stats& stats = m_signalStats[key];
    ++stats.first;
    stats.second += bodySize;
}

////////////////////////////////////////////////////////////////////////////


void BrokerStatistics::registerPerSlotCall(const std::string& exchange, const std::string& routingKey, size_t bodySize) {

    std::string target, slot;
    if (exchange == "Signals") {
        // Treat only direct/broadcast slots here
        return;
    } else if (exchange == "Slots") {
        // Routing is <targetId>.<targetSlot>
        const size_t pos = routingKey.find('.');
        if (pos != std::string::npos) {
            target = routingKey.substr(0, pos - 1);
            slot = routingKey.substr(pos + 1);
        } else {
            target = exchange;
            slot = routingKey;
            if (debug) {
                std::cerr << "Unexpected routing key for 'Slots': " << routingKey << std::endl;
            }
        }
    } else if (exchange == "Global_Slots") {
        target = "[Broadcast]";
        // Routing is <senderId>.<targetSlot>
        const size_t pos = routingKey.find('.');
        if (pos != std::string::npos) {
            slot = routingKey.substr(pos + 1);
        } else {
            if (debug) {
                std::cerr << "Unexpected routing key for 'Global_Slots': " << routingKey << std::endl;
            }
            slot = routingKey;
        }
    } else {
        target = exchange;
        slot = routingKey;
        if (debug) {
            std::cerr << "Unexpected exchange: " << exchange << std::endl;
        }
    }

    Stats& stats = m_slotStats[target + ":" += slot];
    ++stats.first;
    stats.second += bodySize;

    return;
}


////////////////////////////////////////////////////////////////////////////


void BrokerStatistics::printStatistics(const data::Epochstamp& timeStamp, float elapsedSeconds) const {
    std::string when;
    try {
        when = timeStamp.toFormattedString() += " (UTC)";
    } catch (const std::runtime_error& e) {
        // With a mis-configured language system (e.g. 'export LANG=some_garbage'), we get std::runtime_error
        // with message "locale::facet::_S_create_c_locale name not valid".
        when = timeStamp.toIso8601Ext(); // This works - but human readability is reduced...
    }
    // Print kind of header
    std::cout << "\n"
              << m_delimLine << m_delimLine << std::setprecision(2) << std::fixed // 2 digits, keep 0s
              << when << " - average over " << elapsedSeconds << " s:\n";


    std::cout << "Rates by senders and their 'targets' ('signal' or '<targetId>.<slot>'),\n"
              << "prepended by 2 characters of their exchange:\n" << m_delimLine.substr(0, 44) << "\n";
    Stats total = this->printStatistics(m_signalStats, timeStamp, elapsedSeconds);
    this->printLine(SenderId("Total sent", ""), total, elapsedSeconds);
    std::cout << m_delimLine;

    std::cout << "Rates of direct/broadcast slot calls:\n" << m_delimLine.substr(0, 37) << "\n";
    total = this->printStatistics(m_slotStats, timeStamp, elapsedSeconds);
    this->printLine(SlotId("Total slot calls"), total, elapsedSeconds);
    std::cout /*<< m_delimLine*/ << std::flush;
}

////////////////////////////////////////////////////////////////////////////


template <typename StatsMap>
BrokerStatistics::Stats BrokerStatistics::printStatistics(const StatsMap& statsMap, const data::Epochstamp& timeStamp,
                                                          float elapsedSeconds) const {
    // Iterators for looping through map of stats:
    typename StatsMap::const_iterator iter = statsMap.begin();
    const typename StatsMap::const_iterator end = statsMap.end();

    // Keep track of the one with highest rate:
    typename StatsMap::const_iterator iterQuickest = iter;

    // Sum messages and kBytes:
    unsigned int numTotal = 0;
    size_t kBytesTotal = 0;

    // Now loop and print for each signal:
    for (; iter != end; ++iter) {
        const Stats& stats = iter->second; // i.e. count & sum_of_bytes
        if (stats.first) {                 // i.e. if some counts
            this->printLine(iter->first, stats, elapsedSeconds);
        }
        numTotal += stats.first;
        kBytesTotal += stats.second;
        // Check whether iter is the 'Plappermaul' so far:
        if (stats.first > iterQuickest->second.first) {
            iterQuickest = iter;
        }
    }

    // Finally print the one with the highest rate again...
    if (iterQuickest != end) { // even if 0 Hz... (else '&& maxCount > 0')
        std::cout << "\nHighest rate was:\n";
        this->printLine(iterQuickest->first, iterQuickest->second, elapsedSeconds);
    }

    return Stats(numTotal, kBytesTotal);
}

////////////////////////////////////////////////////////////////////////////


template <class IdType>
void BrokerStatistics::printLine(const IdType& id, const Stats& stats, float elapsedSeconds) const {
    const float kBytes = stats.first ? stats.second / (1.e3f * stats.first) : 0.f;

    this->printId(std::cout, id);
    std::cout << ":" << std::right << std::setw(7) << stats.first / elapsedSeconds << " Hz," << std::right
              << std::setw(6) << kBytes << " kB\n";
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


std::vector<std::string> instancesOfServers(const std::string& serverOut, unsigned int sleepSeconds, bool debug) {
    std::vector<std::string> senders;

    if (!serverOut.empty()) {
        if (sleepSeconds > 10) { // If waiting is long, give a hint when it started
            std::cout << "\n" << karabo::data::Timestamp().toFormattedString() << " (UTC):";
        }
        std::cout << "\nGathering topology to identify devices of servers. . " << std::flush;
        // Instead of the gymnastics below, we could add a slot to the server to query it for all their devices...

        // Need an event loop for
        std::jthread thread(std::bind(&net::EventLoop::work));
        std::cout << ". " << std::flush;

        auto client =
              std::make_shared<core::DeviceClient>("", false); // default unique id, explicitly call initialize()
        client->initialize();
        std::cout << ". " << std::flush;  // output some progress markers...
        client->enableInstanceTracking(); // blocking a while to gather topology
        std::cout << ". " << std::flush;

        // If servers busy, discovery might take longer than the above blocking
        while (sleepSeconds-- > 0u) { // postfix decrement!
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << ". " << std::flush;
        }
        std::cout << "\n";
        if (!serverOut.empty()) {
            senders.push_back(serverOut);
            const std::vector<std::string> devices(client->getDevices(serverOut));
            std::cout << "\nFound " << devices.size() << " devices of server " << serverOut;
            if (debug) {
                std::cout << ": " << data::toString(devices) << std::endl;
            } else {
                std::cout << ".";
            }
            senders.insert(senders.end(), devices.begin(), devices.end());
        }
        net::EventLoop::stop();
        thread.join();
    }
    return senders;
}

void printHelp(const char* name) {
    // Get name without leading directories
    std::string nameStr(name ? name : "'command'");
    const std::string::size_type lastSlashPos = nameStr.find_last_of('/');
    if (lastSlashPos != std::string::npos) {
        nameStr.replace(0, lastSlashPos + 1, "");
    }
    std::cout << "\n  " << nameStr << " [-h|--help] [other options with values] [interval]\n\n"
              << "Prints the rate and average size of all messages sent to the "
              << "broker and of\n"
              << "all intended direct/broadcast slot calls.\n"
              << "Broker host and topic are read from the usual environment "
              << "variables\nKARABO_BROKER and KARABO_BROKER_TOPIC or, if "
              << "these are not defined, use the\n"
              << "usual defaults. Optional 'interval' argument specifies the time in seconds\n"
              << "for averaging (default: 5).\n"
              << "Available options:\n"
              << "   --senders a[,b[,c[,...]]]    Consider only messages FROM given ids\n"
              << "   --sendersServer serverId     Consider only messages FROM given serverId,\n"
              << "                                   including its devices\n"
              << "   --discoveryWait seconds      Extra seconds for topology discovery\n"
              << "   --debug y|n                  If yes, adds some debug output\n\n"
              << "The option '--sendersServer' requires to discover the\n"
              << "topology of the Karabo installation. If a server of interest is slowly\n"
              << "responding, the normal discovery time might be too short to identify all its\n"
              << "devices and some extra delay should be added using '--discoveryWait'.\n"
              << std::endl;
}


void startAmqpMonitor(const std::vector<std::string>& brokers, const std::string& domain,
                      const std::vector<std::string>& senders,
                      const data::TimeValue& interval) {
    net::AmqpConnection::Pointer connection(std::make_shared<net::AmqpConnection>(brokers));

    // std::shared_ptr<BrokerStatistics> stats(std::make_shared<BrokerStatistics>(interval, receivers, senders));
    // auto binSerializer = data::BinarySerializer<data::Hash>::create("Bin");

    auto readHandler = [stats{std::make_shared<BrokerStatistics>(domain, interval, senders)},
                        binSerializer{data::BinarySerializer<data::Hash>::create("Bin")}](
                             const data::Hash::Pointer& header, const data::Hash::Pointer& body,
                             const std::string& exchange, const std::string& routingKey) {
        // `BrokerStatistics' expects the message formatted as a Hash: with 'header' as Hash and 'body' as Hash with the
        // single key 'raw' as vector<char> which is the serialized 'body' value.
        // If the incoming 'body' does not contain a proper 'raw' key, serialize the body part.
        size_t bodySize = 0;
        if (body->has("raw") && body->is<std::vector<char>>("raw")) {
            bodySize = body->get<std::vector<char>>("raw").size();
        } else {
            std::vector<char> raw;
            raw.reserve(1000);
            binSerializer->save(body, raw); // body -> raw
            bodySize = raw.size();
        }
        stats->registerMessage(exchange, routingKey, header, bodySize);
    };

    // FIXME: Add a 'skipFlag' to client to skip deserialisation of message body.
    //        If done, can get rid of the serializer above.
    AMQP::Table queueArgs;
    queueArgs
          .set("x-max-length", 10'000)    // Queue limit
          .set("x-overflow", "drop-head") // drop oldest if limit reached
          .set("x-message-ttl", 30'000);  // message time-to-live in ms
    std::ostringstream idStr;
    idStr << domain << ".messageLogger/" << karabo::net::bareHostName() << "/" << getpid();
    net::AmqpHashClient::Pointer client =
          net::AmqpHashClient::create(connection, idStr.str(), queueArgs, readHandler, [](const std::string& msg) {
              std::cout << "Error reading message: " << msg
                        << "\n-----------------------------------------------------------------------\n"
                        << std::endl;
          });
    // Wait until connection established and thus connection->getCurrentUrl() shows proper url
    std::promise<boost::system::error_code> isConnected;
    auto futConnected = isConnected.get_future();
    connection->asyncConnect([&isConnected](const boost::system::error_code& ec) { isConnected.set_value(ec); });
    const boost::system::error_code ec = futConnected.get();
    if (ec) {
        throw KARABO_NETWORK_EXCEPTION("Broker connection failed: " + ec.message());
    }

    std::cout << "\nStart monitoring message rates of \n   domain        '" << domain << "'\n   on broker     '"
              << connection->getCurrentUrl() << "',\n   ";
    if (!senders.empty()) {
        std::cout << "messages from '" << data::toString(senders) << "',\n   ";
    }
    std::cout << "interval is    " << interval << " s." << std::endl;

    // Lambda to initiate subscription, returns future to wait for:
    auto subscribe = [](net::AmqpHashClient::Pointer& client, const std::string& exchange,
                        const std::string& bindingKey) {
        if (debug) {
            std::cout << "Subscribing to exchange: '" << exchange << "' and binding key: '" << bindingKey << "'"
                      << std::endl;
        }

        auto done = std::make_shared<std::promise<boost::system::error_code>>();
        std::future<boost::system::error_code> fut = done->get_future();
        client->asyncSubscribe(exchange, bindingKey,
                               [done{std::move(done)}](const boost::system::error_code& ec) { done->set_value(ec); });
        return fut;
    };

    std::vector<std::future<boost::system::error_code>> futures;
    if (senders.empty()) {
        // Bind to all possible messages ...
        const std::vector<std::array<std::string, 2>> defaultTable = {
              {domain + ".Signals", "#"},     // any INSTANCE, any SIGNAL
              {domain + ".Slots", "#"},       // any INSTANCE, any direct slot call
              {domain + ".Global_Slots", "#"} // any INSTANCE, any broadcast slot
        };
        for (const auto& a : defaultTable) {
            futures.push_back(subscribe(client, a[0], a[1]));
        }
    } else {
        for (const auto& sendId : senders) {
            // FIXME: We miss any direct slot calls/replies calls originating from sendId
            futures.push_back(subscribe(client, domain + ".Signals", sendId + ".#"));
            futures.push_back(subscribe(client, domain + ".Global_Slots", sendId + ".#")); // broadcast slot
        }
    }
    for (auto& fut : futures) {
        const boost::system::error_code ec = fut.get();
        if (ec) {
            throw KARABO_NETWORK_EXCEPTION(std::string("Failed to subscribe to AMQP broker: ") += ec.message());
        }
    }

    // Block forever
    net::EventLoop::work();
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


int main(int argc, const char** argv) {
    net::EventLoop::addThread(2);

    // Setup option defaults
    karabo::data::Hash options("period", static_cast<data::TimeValue>(5ull), "--receivers", "", "--senders", "",
                               "--sendersServer", "", "--discoveryWait", "0");
    for (int i = 1; i < argc; i += 2) {
        const std::string argv_i(argv[i]);
        if (argv_i == "-h" || argv_i == "--help") { // both for backward compatibility
            printHelp(argv[0]);
            return EXIT_SUCCESS;
        } else if (argc == i + 1) {
            // The last of an odd number of arguments maybe the averaging period
            const data::TimeValue p = strtoull(argv[i], 0, 0);
            if (p > 0ull) {
                options.set("period", p);
            } else {
                std::cerr << "Interval must be longer than 1 s, but is deduced from '" << argv_i << "'" << std::endl;
            }
        } else if (argv_i != "--senders" && argv_i != "--sendersServer" && argv_i != "--discoveryWait"
                   && argv_i != "--debug") {
            printHelp(argv[0]);
            return EXIT_FAILURE;
        } else {
            options.set(argv[i], argv[i + 1]);
        }
    }
    // fromString<bool>(..) understands y, yes, Yes, true, True, 1, n, no, No, false, False, 0 and maybe more...
    debug = (options.has("--debug") ? data::fromString<bool>(options.get<std::string>("--debug")) : false);

    const std::string topic(karabo::net::Broker::brokerDomainFromEnv());
    const data::TimeValue interval = options.get<data::TimeValue>("period");

    // Unpack configured senders and receivers.
    std::vector<std::string> senders(
          karabo::data::fromString<std::string, std::vector>(options.get<std::string>("--senders")));
    // If full server is requested, unpack and insert to senders as well.
    const auto sendFromServers =
          instancesOfServers(options.get<std::string>("--sendersServer"),
                             data::fromString<unsigned int>(options.get<std::string>("--discoveryWait")), debug);
    senders.insert(senders.end(), sendFromServers.begin(), sendFromServers.end());

    // Start Logger, but suppress INFO and DEBUG
    log::Logger::configure(data::Hash("level", "WARN"));
    log::Logger::useConsole();

    const std::vector<std::string> brokers(net::Broker::brokersFromEnv());
    const std::string brkType = net::Broker::brokerTypeFrom(brokers);

    try {
        if (brkType == "amqp") {
            startAmqpMonitor(brokers, topic, senders, interval);
        } else {
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION(brkType + " not supported!");
        }

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
