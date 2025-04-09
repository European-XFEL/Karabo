/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 6, 2011, 2:25 PM
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

#ifndef KARABO_CORE_DEVICE_HH
#define KARABO_CORE_DEVICE_HH

#include <unistd.h>

#include <boost/algorithm/string.hpp>
#include <regex>
#include <string>
#include <tuple>
#include <unordered_set>

#include "DeviceClient.hh"
#include "Lock.hh"
#include "karabo/data/schema/AlarmConditionElement.hh"
#include "karabo/data/schema/OverwriteElement.hh"
#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/data/schema/StateElement.hh"
#include "karabo/data/schema/Validator.hh"
#include "karabo/data/time/Epochstamp.hh"
#include "karabo/data/time/Timestamp.hh"
#include "karabo/data/time/Trainstamp.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/HashFilter.hh"
#include "karabo/data/types/State.hh"
#include "karabo/log/Logger.hh"
#include "karabo/log/utils.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/utils.hh"
#include "karabo/util/MetaTools.hh"
#include "karabo/util/Version.hh"
#include "karabo/xms/InputChannel.hh"
#include "karabo/xms/OutputChannel.hh"
#include "karabo/xms/SlotElement.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    namespace core {

        // Convenient logging
#define KARABO_LOG_DEBUG karabo::log::LoggerStream(this->getInstanceId(), spdlog::level::debug)
#define KARABO_LOG_INFO karabo::log::LoggerStream(this->getInstanceId(), spdlog::level::info)
#define KARABO_LOG_WARN karabo::log::LoggerStream(this->getInstanceId(), spdlog::level::warn)
#define KARABO_LOG_ERROR karabo::log::LoggerStream(this->getInstanceId(), spdlog::level::err)

#define KARABO_NO_SERVER "__none__"

        enum Capabilities {

            PROVIDES_SCENES = (1u << 0),
            PROVIDES_MACROS = (1u << 1),
            PROVIDES_INTERFACES = (1u << 2),
            // add future capabilities as bitmask:
            // SOME_OTHER_FUTURE_CAPABILITY = (1u << 3),
        };

        enum Interfaces {

            Motor = (1u << 0),
            MultiAxisMotor = (1u << 1),
            Trigger = (1u << 2),
            Camera = (1u << 3),
            Processor = (1u << 4),
            DeviceInstantiator = (1u << 5),

            // add future interfaces as bitmask:
            // SOME_OTHER_INTERFACE = (1u << 6),
        };

        /**
         * @class Device
         * @brief all Karabo devices derive from this class
         *
         * The Device class is the base class for all Karabo devices.
         * It provides for a standardized set of basic properties that
         * all devices share and provides interaction with its configuration
         * and properties.
         *
         * Devices are defined by their expected parameters; a list of properties
         * and commands that are known to the distributed system at static time.
         * These parameters describe a devices Schema, which in turn describes
         * the possible configurations of the device.
         */
        class Device : public karabo::xms::SignalSlotable {
            std::vector<std::function<void()>> m_initialFunc;
            /// Validators to validate...
            karabo::data::Validator m_validatorIntern; /// ...internal updates via 'Device::set'
            karabo::data::Validator m_validatorExtern; /// ...external updates via 'Device::slotReconfigure'

            std::shared_ptr<DeviceClient> m_deviceClient;

            std::string m_classId;
            std::string m_serverId;
            std::string m_deviceId;
            // To be injected at initialization time; for internal use only.
            std::string m_timeServerId;

            unsigned long long m_timeId;
            unsigned long long m_timeSec;    // seconds
            unsigned long long m_timeFrac;   // attoseconds
            unsigned long long m_timePeriod; // microseconds
            mutable std::mutex m_timeChangeMutex;

            mutable std::mutex m_objectStateChangeMutex;
            karabo::data::Hash m_parameters;
            karabo::data::Schema m_staticSchema;
            karabo::data::Schema m_injectedSchema;
            karabo::data::Schema m_fullSchema;
            std::map<std::string, karabo::data::Schema> m_stateDependentSchema;

            karabo::data::Epochstamp m_lastBrokerErrorStamp;

           public:
            // Derived classes shall use "<packageName>-<repositoryVersion>" as their version
            KARABO_CLASSINFO(Device, "Device", karabo::util::Version::getVersion())
            KARABO_CONFIGURATION_BASE_CLASS;

            /**
             * The expected parameter section of the Device class, known at
             * static time. The basic parameters described here are available
             * for all devices, many of them being expert or admin visible only.
             *
             * @param expected: a Schema to which these parameters will be
             *                  appended.
             */
            static void expectedParameters(karabo::data::Schema& expected);

            /**
             * Construct a device with a given configuration. The configuration
             * Hash may contain any of the following entries:
             *
             * _serverId_: a string representing the server this device is
             *             running on. If not given the device assumes to run
             *             in stand-alone mode.
             *
             * _deviceId_: a string representing this device's id, part of the
             *             unique identifier in the distributed system. If not
             *             given it defaults to _none_.
             *
             * @param configuration
             */
            Device(const karabo::data::Hash& configuration);

            /**
             * The destructor will reset the DeviceClient attached to this device.
             */
            virtual ~Device();

            /**
             * Register a function to be called after construction
             *
             * Can be called by each class of an inheritance chain.
             * Functions will be called in order of registration.
             */
            void registerInitialFunction(const std::function<void()>& func) {
                m_initialFunc.push_back(func);
            }

#define KARABO_INITIAL_FUNCTION(function) this->registerInitialFunction(std::bind(&Self::function, this));

            /**
             * This method is called to finalize initialization of a device. It is needed to allow user
             * code to hook in after the base device constructor, but before the device is fully initialized.
             *
             * @param connection The broker connection for the device.
             * @param consumeBroadcasts If false, do not listen directly to broadcast messages (addressed to '*').
             *                          Whoever sets this to false has to ensure that broadcast messages reach the
             *                          Device in some other way, otherwise the device will not work correctly.
             * @param timeServerId The id of the time server to be used by the device - usually set by the DeviceServer.
             */
            void finalizeInternalInitialization(const karabo::net::Broker::Pointer& connection, bool consumeBroadcasts,
                                                const std::string& timeServerId);

            /**
             * This function allows to communicate to other (remote) devices.
             * Any device contains also a controller for other devices (DeviceClient)
             * which is returned by this function.
             *
             * @return DeviceClient instance
             */
            DeviceClient& remote();

            /**
             * Updates the state/properties of the device. This function automatically notifies any observers in the
             * distributed system.
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @param value The corresponding value (of corresponding data-type)
             */
            template <class ValueType>
            void set(const std::string& key, const ValueType& value) {
                this->set<ValueType>(key, value, getActualTimestamp());
            }

            void set(const std::string& key, const karabo::data::State& state);

            void set(const std::string& key, const karabo::data::AlarmCondition& condition);

            enum class VectorUpdate {

                removeOne = -2, // only first occurrence
                removeAll = -1, // all occurrences
                add = 1,
                addIfNotIn = 2
            };

            /**
             * Concurrency safe update of vector property
             *
             * Does not work for Hash (i.e. table element) due to Hash equality just checking similarity.
             * Removing might be unreliable for VECTOR_FLOAT or VECTOR_DOUBLE due to floating point equality issues.
             *
             * @param key of the vector property to update
             * @param updates: items to remove from property vector (starting at the front) or to add (at the end)
             * @param updateType: indicates update type - applied individually to all items in 'updates'
             * @param timestamp: timestamp to assign to updated vector property (e.g. getTimestamp())
             */
            template <class ItemType>
            void setVectorUpdate(const std::string& key, const std::vector<ItemType>& updates, VectorUpdate updateType,
                                 const karabo::data::Timestamp& timestamp) {
                // Get current value and update as requested
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                std::vector<ItemType> vec(m_parameters.get<std::vector<ItemType>>(key));
                switch (updateType) {
                    case VectorUpdate::add:
                        vec.insert(vec.end(), updates.begin(), updates.end());
                        break;
                    case VectorUpdate::addIfNotIn:
                        for (const ItemType& update : updates) {
                            auto it = std::find(vec.begin(), vec.end(), update);
                            if (it == vec.end()) vec.push_back(update);
                        }
                        break;
                    case VectorUpdate::removeOne:
                        for (auto itUpdate = updates.begin(); itUpdate != updates.end(); ++itUpdate) {
                            auto itInVec = std::find(vec.begin(), vec.end(), *itUpdate);
                            if (itInVec != vec.end()) {
                                vec.erase(itInVec);
                            }
                        }
                        break;
                    case VectorUpdate::removeAll:
                        auto itNewEnd = std::remove_if(vec.begin(), vec.end(), [&updates](const ItemType& item) {
                            return (std::find(updates.begin(), updates.end(), item) != updates.end());
                        });
                        vec.resize(itNewEnd - vec.begin()); // remove_if does not yet shrink
                        break;
                }

                // Now publish the new value
                karabo::data::Hash h;
                std::vector<ItemType>& vecInHash = h.bindReference<std::vector<ItemType>>(key);
                vecInHash.swap(vec);
                setNoLock(h, timestamp);
            }

            /**
             * Updates the state of the device. This function automatically notifies any observers in the distributed
             * system.
             *
             * Any updates are validated against the device schema and rejected if they are not
             * appropriate for the current device state or are of wrong type. During validation
             * alarm bounds are evaluated and alarms on properties will be raised if alarm
             * conditions are met. Additionally, the distributed system is notified of these
             * alarms.
             *
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @param value The corresponding value (of corresponding data-type)
             * @param timestamp The time of the value change
             */
            template <class ValueType>
            void set(const std::string& key, const ValueType& value, const karabo::data::Timestamp& timestamp) {
                karabo::data::Hash h(key, value);
                this->set(h, timestamp);
            }

            void set(const std::string& key, const karabo::data::State& state,
                     const karabo::data::Timestamp& timestamp);

            void set(const std::string& key, const karabo::data::AlarmCondition& condition,
                     const karabo::data::Timestamp& timestamp);

            /**
             * Writes a hash to the specified channel. The hash internally must
             * follow exactly the data schema as defined in the expected parameters.
             *
             * If 'data' contains an 'NDArray', consider to use the overload of this method that has the 'safeNDArray'
             * flag. If that flag can be set to 'true', data copies can be avoided:
             *
             * writeChannel(channelName, data, getActualTimestamp(), true);
             *
             * @param channelName The output channel name
             * @param data Hash with the data
             *
             * Thread safety:
             * The 'writeChannel(..)' methods and 'signalEndOfStream(..)' must not be called concurrently
             * for the same 'channelName'.
             */
            void writeChannel(const std::string& channelName, const karabo::data::Hash& data);

            /**
             * Writes a hash to the specified channel. The hash internally must
             * follow exactly the data schema as defined in the expected parameters.
             *
             * @param channelName The output channel name
             * @param data Hash with the data
             * @param timestamp A user provided timestamp (if e.g. retrieved from h/w)
             * @param safeNDArray Boolean that should be set to 'true' if 'data' contains any 'NDArray' and their data
             *                    is not changed after this 'writeChannel'. Otherwise, data will be copied if needed,
             *                    i.e. when the output channel has to queue or serves inner-process receivers.
             *
             * Thread safety:
             * The 'writeChannel(..)' methods and 'signalEndOfStream(..)' must not be called concurrently
             * for the same 'channelName'.
             */
            void writeChannel(const std::string& channelName, const karabo::data::Hash& data,
                              const karabo::data::Timestamp& timestamp, bool safeNDArray = false);

            /**
             * Signals an end-of-stream event (EOS) on the output channel identified
             * by channelName
             * @param channelName: the name of the output channel.
             *
             * Thread safety:
             * The 'writeChannel(..)' methods and 'signalEndOfStream(..)' must not be called concurrently
             * for the same 'channelName'.
             */
            void signalEndOfStream(const std::string& channelName) {
                this->getOutputChannel(channelName)->signalEndOfStream();
            }

            /**
             * Updates the state/properties of the device with all key/value pairs given in the hash.
             *
             * Any updates are validated against the device schema and rejected if they are not
             * appropriate for the current device state or are of wrong type. During validation
             * alarm bounds are evaluated and alarms on properties will be raised if alarm
             * conditions are met. Additionally, the distributed system is notified of these
             * alarms.
             * For those paths in 'hash' which do not already have time stamp attributes assigned as tested by
             * Timestamp::hashAttributesContainTimeInformation(hash.getAttributes(<path>))),
             * the actual timestamp is chosen.
             *
             * NOTE: This function will automatically and efficiently (only one message) inform
             * any observers.
             * @param hash Hash of updated internal parameters
             *             (must be in current full schema, e.g. since declared in the expectedParameters function)
             */
            void set(const karabo::data::Hash& hash);

            /**
             * Updates the state of the device with all key/value pairs given in the hash
             *
             * Any updates are validated against the device schema and rejected if they are not
             * appropriate for the current device state or are of wrong type. During validation
             * alarm bounds are evaluated and alarms on properties will be raised if alarm
             * conditions are met. Additionally, the distributed system is notified of these
             * alarms.
             *
             * NOTE: This function will automatically and efficiently (only one message) inform
             * any observers.
             * @param hash Hash of updated internal parameters
             *             (must be in current full schema, e.g. since declared in the expectedParameters function)
             * @param timestamp to indicate when the set occurred - but is ignored for paths in 'hash'
             *                  that already have time stamp attributes as tested by
             *                  Timestamp::hashAttributesContainTimeInformation(hash.getAttributes(<path>)))
             */
            void set(const karabo::data::Hash& hash, const karabo::data::Timestamp& timestamp);

           private:
            /**
             * Internal method for set(Hash, Timestamp), requiring m_objectStateChangeMutex to be locked
             */
            void setNoLock(const karabo::data::Hash& hash, const karabo::data::Timestamp& timestamp);

           public:
            /**
             * Updates the state of the device with all key/value pairs given in the hash.
             * In contrast to the set function, no validation is performed.
             *
             * @param key identifying the property to update
             * @param value: updated value
             */
            template <class ValueType>
            void setNoValidate(const std::string& key, const ValueType& value) {
                this->setNoValidate<ValueType>(key, value, getActualTimestamp());
            }

            /**
             * Updates the state of the device with all key/value pairs given in the hash.
             * In contrast to the set function, no validation is performed.
             *
             * @param key identifying the property to update
             * @param value: updated value
             * @param timestamp optional timestamp to indicate when the set occurred.
             */
            template <class ValueType>
            void setNoValidate(const std::string& key, const ValueType& value,
                               const karabo::data::Timestamp& timestamp) {
                karabo::data::Hash h(key, value);
                this->setNoValidate(h, timestamp);
            }

            /**
             * Updates the state of the device with all key/value pairs given in the hash.
             * In contrast to the set function, no validation is performed.
             *
             * NOTE: This function will automatically and efficiently (only one message) inform
             * any observers.
             * @param config Hash of updated internal parameters (must be declared in the expectedParameters function)
             */
            void setNoValidate(const karabo::data::Hash& hash);

            /**
             * Updates the state of the device with all key/value pairs given in the hash.
             * In contrast to the set function, no validation is performed.
             *
             * NOTE: This function will automatically and efficiently (only one message) inform
             * any observers.
             * @param config Hash of updated internal parameters (must be declared in the expectedParameters function)
             * @param timestamp optional timestamp to indicate when the set occurred.
             */
            void setNoValidate(const karabo::data::Hash& hash, const karabo::data::Timestamp& timestamp);

           private:
            /**
             * Internal version of setNoValidate(hash, timestamp) that requires m_objectStateChangeMutex to be locked
             */
            void setNoValidateNoLock(const karabo::data::Hash& hash, const karabo::data::Timestamp& timestamp);

           public:
            /**
             * Retrieves the current value of any device parameter (that was defined in the expectedParameters function)
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @return value of the requested parameter
             */
            template <class T>
            T get(const std::string& key) const {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);

                try {
                    const karabo::data::Hash::Attributes& attrs =
                          m_fullSchema.getParameterHash().getNode(key).getAttributes();
                    if (attrs.has(KARABO_SCHEMA_LEAF_TYPE)) {
                        const int leafType = attrs.get<int>(KARABO_SCHEMA_LEAF_TYPE);
                        if (leafType == karabo::data::Schema::STATE) {
                            if (typeid(T) == typeid(karabo::data::State)) {
                                return *reinterpret_cast<const T*>(
                                      &karabo::data::State::fromString(m_parameters.get<std::string>(key)));
                            }
                            throw KARABO_PARAMETER_EXCEPTION("State element at " + key +
                                                             " may only return state objects");
                        }
                        if (leafType == karabo::data::Schema::ALARM_CONDITION) {
                            if (typeid(T) == typeid(karabo::data::AlarmCondition)) {
                                return *reinterpret_cast<const T*>(
                                      &karabo::data::AlarmCondition::fromString(m_parameters.get<std::string>(key)));
                            }
                            throw KARABO_PARAMETER_EXCEPTION("Alarm condition element at " + key +
                                                             " may only return alarm condition objects");
                        }
                    }

                    return m_parameters.get<T>(key);
                } catch (const karabo::data::Exception&) {
                    KARABO_RETHROW_AS(
                          KARABO_PARAMETER_EXCEPTION(getInstanceId() + " failed to retrieve parameter '" + key + "'"));
                }
                return *static_cast<T*>(NULL); // never reached. Keep it to make the compiler happy.
            }

            /**
             * Retrieves the current value of any device parameter (that was defined in the expectedParameters function)
             * The value is casted on the fly into the desired type.
             * NOTE: This function is considerably slower than the simple get() functionality
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @return value of the requested parameter
             */
            template <class T>
            T getAs(const std::string& key) const {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                try {
                    return m_parameters.getAs<T>(key);
                } catch (const karabo::data::Exception& e) {
                    KARABO_RETHROW_AS(
                          KARABO_PARAMETER_EXCEPTION("Error whilst retrieving parameter (" + key + ") from device"));
                }
            }

            /**
             * Retrieves all expected parameters of this device
             * @return Schema object containing all expected parameters
             */
            karabo::data::Schema getFullSchema() const;

            /**
             * Append a schema to the existing device schema
             * @param schema to be appended -  may also contain existing elements to overwrite their
             *                attributes like min/max values/sizes, alarm ranges, etc.
             *                If it contains Input-/OutputChannels, they are (re-)created.
             *                If previously an InputChannel existed under the same key, its data/input/endOfStream
             * handlers are kept for the recreated InputChannel.
             * @param unused parameter, kept for backward compatibility.
             */
            void appendSchema(const karabo::data::Schema& schema, const bool /*unused*/ = false);

            /**
             * Replace existing schema descriptions by static (hard coded in expectedParameters) part and
             * add additional (dynamic) descriptions. Previous additions will be removed.
             *
             * @param schema additional, dynamic schema - may also contain existing elements to overwrite their
             *                attributes like min/max values/sizes, alarm ranges, etc.
             *                If it contains Input-/OutputChannels, they are (re-)created (and previously added ones
             * removed). If previously an InputChannel existed under the same key, its data/input/endOfStream handlers
             *                are kept for the recreated InputChannel.
             * @param unused parameter, kept for backward compatibility.
             */
            void updateSchema(const karabo::data::Schema& schema, const bool /*unused*/ = false);

            /**
             * Converts a device parameter key into its aliased key (must be defined in the expectedParameters function)
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @return Aliased representation of the parameter
             */
            template <class AliasType>
            AliasType getAliasFromKey(const std::string& key) const {
                try {
                    std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                    return m_fullSchema.getAliasFromKey<AliasType>(key);
                } catch (const karabo::data::Exception& e) {
                    KARABO_RETHROW_AS(
                          KARABO_PARAMETER_EXCEPTION("Error whilst retrieving alias from parameter (" + key + ")"));
                    return AliasType(); // compiler happiness line - requires that AliasType is default constructable...
                }
            }

            /**
             * Converts a device parameter alias into the original key (must be defined in the expectedParameters
             * function)
             * @param key A valid parameter-alias of the device (must be defined in the expectedParameters function)
             * @return The original name of the parameter
             */
            template <class AliasType>
            std::string getKeyFromAlias(const AliasType& alias) const {
                try {
                    std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                    return m_fullSchema.getKeyFromAlias(alias);
                } catch (const karabo::data::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Error whilst retrieving parameter from alias (" +
                                                                 karabo::data::toString(alias) + ")"));
                    return std::string(); // compiler happiness line...
                }
            }

            /**
             * Checks if the argument is a valid alias of some key, i.e. defined in the expectedParameters function
             * @param alias Arbitrary argument of arbitrary type
             * @return true if it is an alias found in one of three containers of parameters:
             * "reconfigurable", "initial" or "monitored",  otherwise false
             */
            template <class T>
            const bool aliasHasKey(const T& alias) const {
                std::lock_guard<std::mutex> lock(m_objectStateChangeMutex);
                return m_fullSchema.aliasHasKey(alias);
            }

            /**
             * Checks if some alias is defined for the given key
             * @param key in expectedParameters mapping
             * @return true if the alias exists
             */
            bool keyHasAlias(const std::string& key) const;

            /**
             * Checks the type of any device parameter (that was defined in the expectedParameters function)
             * @param key A valid parameter of the device (must be defined in the expectedParameters function)
             * @return The enumerated internal reference type of the value
             */
            karabo::data::Types::ReferenceType getValueType(const std::string& key) const;

            /**
             * Retrieves the current configuration.
             * If no argument is given, all parameters (those described in the expected parameters section) are
             * returned. A subset of parameters can be retrieved by specifying one or more tags.
             * @param tags The tags (separated by comma) the parameter must carry to be retrieved
             * @return A Hash containing the current value of the selected configuration
             */
            karabo::data::Hash getCurrentConfiguration(const std::string& tags = "") const;

            /**
             * Retrieves a slice of the current configuration.
             *
             * @param paths of the configuration which should be returned (as declared in expectedParameters)
             *              (method throws ParameterExcepton if a non-existing path is given)
             * @return Hash with the current values and attributes (e.g. timestamp) of the selected configuration
             *
             */
            karabo::data::Hash getCurrentConfigurationSlice(const std::vector<std::string>& paths) const;

            /**
             * Return a tag filtered version of the input Hash. Tags are as defined
             * in the device schema
             * @param hash to filter
             * @param tags to filter by
             * @return a filtered version of the input Hash.
             */
            karabo::data::Hash filterByTags(const karabo::data::Hash& hash, const std::string& tags) const;

            /**
             * Return the serverId of the server this device is running on
             * @return
             */
            const std::string& getServerId() const {
                return m_serverId;
            }

            /**
             * Return a State object holding the current unified state of
             * the device.
             * @return
             */
            const karabo::data::State getState() {
                return this->get<karabo::data::State>("state");
            }

            /**
             * Update the state of the device, using "actual timestamp".
             *
             * Will also update the instanceInfo describing this device instance (if new or old State are ERROR).
             *
             * @param currentState: the state to update to
             */
            void updateState(const karabo::data::State& currentState) {
                updateState(currentState, karabo::data::Hash(), getActualTimestamp());
            }

            /**
             * Update the state of the device, using "actual timestamp".
             *
             * Will also update the instanceInfo describing this device instance (if new or old State are ERROR).
             *
             * @param currentState: the state to update to
             * @param other: a Hash to set other properties in the same state update message,
             *               time stamp attributes to its paths have precedence over the actual timestamp
             */
            void updateState(const karabo::data::State& currentState, const karabo::data::Hash& other) {
                updateState(currentState, other, getActualTimestamp());
            }

            /**
             * Update the state of the device, using given timestamp.
             *
             * Will also update the instanceInfo describing this device instance (if new or old State are ERROR).
             *
             * @param currentState: the state to update to
             * @param timestamp: time stamp to assign to the state property and the properties in 'other'
             *                   (if the latter do not have specified timestamp attributes)
             *
             */
            void updateState(const karabo::data::State& currentState, const karabo::data::Timestamp& timestamp) {
                updateState(currentState, karabo::data::Hash(), timestamp);
            }

            /**
             * Update the state of the device, using given timestamp.
             *
             * Will also update the instanceInfo describing this device instance (if new or old State are ERROR).
             *
             * @param currentState: the state to update to
             * @param other: a Hash to set other properties in the same state update message,
             *               time stamp attributes to its paths have precedence over the given 'timestamp'
             * @param timestamp: time stamp to assign to the state property and the properties in 'other'
             *                   (if the latter do not have specified timestamp attributes)
             *
             */
            void updateState(const karabo::data::State& currentState, karabo::data::Hash other,
                             const karabo::data::Timestamp& timestamp);


            /**
             * Execute a command on this device
             * @param command
             */
            void execute(const std::string& command) const {
                call("", command);
            }

            /**
             * Execute a command with one argument on this device
             * @param command
             * @param a1
             */
            template <class A1>
            void execute(const std::string& command, const A1& a1) const {
                call("", command, a1);
            }

            /**
             * Execute a command with two arguments on this device
             * @param command
             * @param a1
             * @param a2
             */
            template <class A1, class A2>
            void execute(const std::string& command, const A1& a1, const A2& a2) const {
                call("", command, a1, a2);
            }

            /**
             * Execute a command with three arguments on this device
             * @param command
             * @param a1
             * @param a2
             * @param a3
             */
            template <class A1, class A2, class A3>
            void execute(const std::string& command, const A1& a1, const A2& a2, const A3& a3) const {
                call("", command, a1, a2, a3);
            }

            /**
             * Execute a command with four arguments on this device
             * @param command
             * @param a1
             * @param a2
             * @param a3
             * @param a4
             */
            template <class A1, class A2, class A3, class A4>
            void execute(const std::string& command, const A1& a1, const A2& a2, const A3& a3, const A4& a4) const {
                call("", command, a1, a2, a3, a4);
            }

            /**
             * Get the current alarm condition the device is in
             * @return
             */
            karabo::data::AlarmCondition getAlarmCondition() const {
                return this->get<karabo::data::AlarmCondition>("alarmCondition");
            }

            /**
             * Set the global alarm condition
             * @param condition to set
             * @param needsAcknowledging if this condition will require acknowledgment on the alarm service
             * @param description an optional description of the condition. Consider including remarks on how to resolve
             */
            void setAlarmCondition(const karabo::data::AlarmCondition& condition, bool needsAcknowledging = false,
                                   const std::string& description = std::string());

            /**
             * Get the alarm condition for a specific property
             * @param key of the property to get the condition for
             * @param sep optional separator to use in the key path
             * @return the alarm condition of the property
             */
            const karabo::data::AlarmCondition& getAlarmCondition(const std::string& key,
                                                                  const std::string& sep = ".") const;

            /**
             * A slot called by the device server if the external time ticks update to synchronize
             * this device with the timing system.
             *
             * @param id: current train id
             * @param sec: current system seconds
             * @param frac: current fractional seconds
             * @param period: interval between subsequent ids in microseconds
             */
            void slotTimeTick(unsigned long long id, unsigned long long sec, unsigned long long frac,
                              unsigned long long period);

            // protected since called in Device::slotTimeTick
            /**
             * A hook which is called if the device receives external time-server update, i.e. if slotTimeTick on the
             * device server is called.
             * Can be overwritten by derived classes.
             *
             * @param id: train id
             * @param sec: unix seconds
             * @param frac: fractional seconds (i.e. attoseconds)
             * @param period: interval between ids in microseconds
             */
            virtual void onTimeTick(unsigned long long id, unsigned long long sec, unsigned long long frac,
                                    unsigned long long period) {}

            /**
             * If the device receives time-server updates via slotTimeTick, this hook will be called for every id
             * in sequential order. The time stamps (sec + frac) of subsequent ids might be identical - though they
             * are usually spaced by period.
             * Can be overwritten in derived classes.
             *
             * @param id: train id
             * @param sec: unix seconds
             * @param frac: fractional seconds (i.e. attoseconds)
             * @param period: interval between ids in microseconds
             */
            virtual void onTimeUpdate(unsigned long long id, unsigned long long sec, unsigned long long frac,
                                      unsigned long long period) {}

            /**
             * Append Schema to change/set maximum size information for path - if paths does not exist, throw exception
             *
             * This is similar to the more general appendSchema, but dedicated to a common use case.
             *
             * Caveat: This does not recreate an output channel if its schema is changed
             *
             * @param path  indicates the parameter which should be a Vector- or TableElement
             * @param value is the new maximum size of the parameter
             * @param emitFlag indicates if others should be informed about this Schema update.
             *                 If this method is called for a bunch of paths, it is recommended to
             *                 set this to true only for the last call.
             */
            void appendSchemaMaxSize(const std::string& path, unsigned int value, bool emitFlag = true);

           protected: // Functions and Classes
            virtual void preReconfigure(karabo::data::Hash& incomingReconfiguration) {}

            virtual void postReconfigure() {}

            virtual void preDestruction() {}

            virtual bool allowLock() const {
                return true;
            }

            /**
             * Returns the actual timestamp. The Trainstamp part of Timestamp is extrapolated from the last values
             * received via slotTimeTick (or zero if no time ticks received yet).
             * To receive time ticks, the server of the device has to be connected to a time server.
             *
             * @return the actual timestamp
             */
            inline karabo::data::Timestamp getActualTimestamp() const {
                return getTimestamp(karabo::data::Epochstamp()); // i.e. epochstamp for now
            }

            /**
             * Returns the Timestamp for given Epochstamp. The Trainstamp part of Timestamp is extrapolated forward or
             * backward from the last values received via slotTimeTick
             * (or zero if no time ticks received yet).
             * To receive time ticks, the server of the device has to be connected to a time server.
             * @param epoch for that the time stamp is searched for
             * @return the matching timestamp, consisting of epoch and the corresponding Trainstamp
             */
            karabo::data::Timestamp getTimestamp(const karabo::data::Epochstamp& epoch) const;

           private: // Functions
            void wrapRegisteredInit();

            void initClassId() {
                m_classId = getClassInfo().getClassId();
            }

            void initSchema();

            void initDeviceSlots();

            /**
             *  Called to setup pipeline channels, will
             *  recursively go through the given schema, assuming it to be at least
             *  a part of the schema of the device.
             *  Needs to be called with m_objectStateChangeMutex being locked.
             *  *
             *  * @param schema: the schema to traverse
             *  * @param topLevel: std::string: empty or existing path of full
             *  *                  schema of the device
             *  */
            void initChannels(const karabo::data::Schema& schema, const std::string& topLevel = "");

            /**
             * Create OutputChannel for given path and take care to set handlers needed
             * Needs to be called with m_objectStateChangeMutex being locked.
             * @param path
             */
            void prepareOutputChannel(const std::string& path);

            /**
             * Create InputChannel for given path and take care to set handlers needed
             * Needs to be called with m_objectStateChangeMutex being locked.
             * @param path
             */
            void prepareInputChannel(const std::string& path);

            void trackInputChannelConnections(const std::string& inputChannel, const std::string& outputChannel,
                                              karabo::net::ConnectionStatus status);

            /**
             * This function is called by SignalSlotable to verify if a slot may
             * be called from remote. The function only checks slots that are
             * mentioned in the expectedParameter section ("DeviceSlots")
             * @param slotName: name of the slot
             * @param callee: the calling remote, can be unknown
             *
             * The following checks are performed:
             *
             * 1) Is this device locked by another device? If the lockedBy field
             * is non-empty and does not match the callee's instance id, the call will
             * be rejected
             *
             * 2) Is the slot callable from the current state, i.e. is the
             * current state specified as an allowed state for the slot.
             */
            void slotCallGuard(const std::string& slotName, const std::string& callee);

            void ensureSlotIsValidUnderCurrentLock(const std::string& slotName, const std::string& callee);

            void ensureSlotIsValidUnderCurrentState(const std::string& slotName);

            void slotGetConfiguration();

            void slotGetConfigurationSlice(const karabo::data::Hash& info);

            void slotGetSchema(bool onlyCurrentState);

            void slotReconfigure(const karabo::data::Hash& newConfiguration);

            std::pair<bool, std::string> validate(const karabo::data::Hash& unvalidated, karabo::data::Hash& validated);

            void applyReconfiguration(const karabo::data::Hash& reconfiguration);

            void slotKillDevice();

            karabo::data::Schema getStateDependentSchema(const karabo::data::State& state);

            void updateLatencies(const karabo::data::Hash::Pointer& performanceMeasures);

            void onBrokerError(const std::string& message);

            /**
             * Clear any lock on this device
             */
            void slotClearLock() {
                set("lockedBy", std::string());
            }

            /**
             * Internal method to retrieve time information of this device.
             */
            karabo::data::Hash getTimeInfo();

            /**
             * Returns the actual time information of this device.
             *
             * @param info an unused (at least for now) Hash parameter that
             *        has been added to fit the generic slot call
             *        (Hash in, Hash out) of the GUI server/client protocol.
             *
             * This slot reply is a Hash with the following keys:
             *   - ``time`` and its attributes provide an actual timestamp
             *     with train Id information.
             *   - ``timeServerId`` the id of the time server configured for
             *     the DeviceServer of this device; "None" when there's no time
             *     server configured.
             *   - ``reference`` and its attributes provide the latest
             *     timestamp information received from the timeserver.
             */
            void slotGetTime(const karabo::data::Hash& /* unused */) {
                const karabo::data::Hash result(this->getTimeInfo());
                reply(result);
            }
            /**
             * Returns the actual system information of this device.
             *
             * @param info an unused (at least for now) Hash parameter that
             *        has been added to fit the generic slot call
             *        (Hash in, Hash out) of the GUI server/client protocol.
             *
             * This slot reply is a Hash with the following keys:
             *   - ``user`` the actual user running this device
             *   - ``broker`` the current connected broker node
             *
             * and all keys provided by ``slotGetTime``.
             */
            void slotGetSystemInfo(const karabo::data::Hash& /* unused */);
        };

    } // namespace core
} // namespace karabo

#endif
