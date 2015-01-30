/*
 * $Id: Slot.hh 2810 2011-01-07 10:36:58Z wrona $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on August 19, 2010, 4:49 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XMS_SLOT_HH
#define	KARABO_XMS_SLOT_HH

#include <karabo/util.hpp>
#include <karabo/log.hpp>

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package io
     */
    namespace xms {

        // Forward SignalSlotable
        class SignalSlotable;

        class Slot {
            friend class SignalSlotable;

            std::string m_instanceIdOfSender;
            std::string m_userIdOfSender;
            std::string m_accessLevelOfSender;
            std::string m_sessionTokenOfSender;

        public:

            const std::string& getInstanceIdOfSender() const;

            const std::string& getUserIdOfSender() const;

            const std::string& getAccessLevelOfSender() const;

            const std::string& getSessionTokenOfSender() const;

        protected:

            std::string m_slotFunction;

            mutable boost::mutex m_callRegisteredSlotFunctionsMutex;

            Slot(const std::string& slotFunction) : m_slotFunction(slotFunction) {
            }

            void extractSenderInformation(const karabo::util::Hash& header);

            void invalidateSenderInformation();

            virtual void callRegisteredSlotFunctions(const karabo::util::Hash& header, const karabo::util::Hash& body) = 0;
        };

        class Slot0 : public Slot {
            typedef boost::function<void () > SlotHandler;

        public:

            KARABO_CLASSINFO(Slot0, "Slot0", "1.0")

            Slot0(const std::string& slotFunction) : Slot(slotFunction) {
            }

            virtual ~Slot0() {
            }

            void registerSlotFunction(const SlotHandler& slotHandler) {
                m_slotHandlers.push_back(slotHandler);
            }

        private:

            void callRegisteredSlotFunctions(const karabo::util::Hash& header, const karabo::util::Hash& body) {
                // This mutex protects against concurrent runs of the same slot function
                boost::mutex::scoped_lock lock(m_callRegisteredSlotFunctionsMutex);
                extractSenderInformation(header);
                for (size_t i = 0; i < m_slotHandlers.size(); ++i) {
                    m_slotHandlers[i]();
                }
                invalidateSenderInformation();
            }

            std::vector<SlotHandler> m_slotHandlers;
        };

        template <class A1>
        class Slot1 : public Slot {
            typedef boost::function<void (const A1&) > SlotHandler;

        public:

            KARABO_CLASSINFO(Slot1, "Slot1", "1.0")

            Slot1(const std::string& slotFunction) : Slot(slotFunction) {
            }

            virtual ~Slot1() {
            }

            void registerSlotFunction(const SlotHandler& slotHandler) {
                m_slotHandlers.push_back(slotHandler);
            }

        private:

            void callRegisteredSlotFunctions(const karabo::util::Hash& header, const karabo::util::Hash& body) {

                boost::mutex::scoped_lock lock(m_callRegisteredSlotFunctionsMutex);

                try {
                    extractSenderInformation(header);
                    const A1& a1 = body.get<A1>("a1");
                    for (size_t i = 0; i < m_slotHandlers.size(); ++i) {
                        m_slotHandlers[i](a1);
                    }
                    invalidateSenderInformation();
                } catch (const karabo::util::CastException& e) {
                    invalidateSenderInformation();
                    KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("Received incompatible argument (see above) for slot \"" + m_slotFunction + "\". Check your connection!"));
                } catch (const karabo::util::Exception& e) {
                    invalidateSenderInformation();
                    KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("An exception was thrown in slot \"" + m_slotFunction + "\""));
                } catch (...) {
                    invalidateSenderInformation();
                    KARABO_RETHROW;
                }
            }

            std::vector<SlotHandler> m_slotHandlers;
        };

        template <class A1, class A2>
        class Slot2 : public Slot {
            typedef boost::function<void (const A1&, const A2&) > SlotHandler;

        public:

            KARABO_CLASSINFO(Slot2, "Slot2", "1.0")

            Slot2(const std::string& slotFunction) : Slot(slotFunction) {
            }

            virtual ~Slot2() {
            }

            void registerSlotFunction(const SlotHandler& slotHandler) {
                m_slotHandlers.push_back(slotHandler);
            }

        private:

            void callRegisteredSlotFunctions(const karabo::util::Hash& header, const karabo::util::Hash& body) {

                boost::mutex::scoped_lock lock(m_callRegisteredSlotFunctionsMutex);

                try {
                    extractSenderInformation(header);
                        const A1& a1 = body.get<A1>("a1");
                        const A2& a2 = body.get<A2>("a2");
                    for (size_t i = 0; i < m_slotHandlers.size(); ++i) {
                        m_slotHandlers[i](a1, a2);
                    }                    
                    invalidateSenderInformation();
                } catch (const karabo::util::CastException& e) {
                    invalidateSenderInformation();
                    KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("Received incompatible argument (see above) for slot \"" + m_slotFunction + "\". Check your connection!"));
                } catch (const karabo::util::Exception& e) {
                    invalidateSenderInformation();
                    KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("An exception was thrown in slot \"" + m_slotFunction + "\""));
                } catch (...) {
                    invalidateSenderInformation();
                    KARABO_RETHROW;
                }

            }

            std::vector<SlotHandler> m_slotHandlers;
        };

        template <class A1, class A2, class A3>
        class Slot3 : public Slot {
            typedef boost::function<void (const A1&, const A2&, const A3&) > SlotHandler;

        public:

            KARABO_CLASSINFO(Slot3, "Slot3", "1.0")

            Slot3(const std::string& slotFunction) : Slot(slotFunction) {
            }

            virtual ~Slot3() {
            }

            void registerSlotFunction(const SlotHandler& slotHandler) {
                m_slotHandlers.push_back(slotHandler);
            }

        private:

            void callRegisteredSlotFunctions(const karabo::util::Hash& header, const karabo::util::Hash& body) {

                boost::mutex::scoped_lock lock(m_callRegisteredSlotFunctionsMutex);

                try {
                    extractSenderInformation(header);
                    const A1& a1 = body.get<A1 > ("a1");
                    const A2& a2 = body.get<A2 > ("a2");
                    const A3& a3 = body.get<A3 > ("a3");
                    for (size_t i = 0; i < m_slotHandlers.size(); ++i) {
                        m_slotHandlers[i](a1, a2, a3);
                    }
                    invalidateSenderInformation();
                } catch (const karabo::util::CastException& e) {
                    invalidateSenderInformation();
                    KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("Received incompatible arguments (see above) for slot \"" + m_slotFunction + "\". Check your connection!"));
                } catch (const karabo::util::Exception& e) {
                    invalidateSenderInformation();
                    KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("An exception was thrown in slot \"" + m_slotFunction + "\""));
                } catch (...) {
                    invalidateSenderInformation();
                    KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("An unknown exception was thrown in slot \"" + m_slotFunction + "\""));
                }
            }

            std::vector<SlotHandler> m_slotHandlers;
        };

        template <class A1, class A2, class A3, class A4>
        class Slot4 : public Slot {
            typedef boost::function<void (const A1&, const A2&, const A3&, const A4&) > SlotHandler;

        public:

            KARABO_CLASSINFO(Slot4, "Slot4", "1.0")

            Slot4(const std::string& slotFunction) : Slot(slotFunction) {
            }

            virtual ~Slot4() {
            }

            void registerSlotFunction(const SlotHandler& slotHandler) {
                m_slotHandlers.push_back(slotHandler);
            }

        private:

            void callRegisteredSlotFunctions(const karabo::util::Hash& header, const karabo::util::Hash& body) {

                boost::mutex::scoped_lock lock(m_callRegisteredSlotFunctionsMutex);

                try {
                    extractSenderInformation(header);
                    const A1& a1 = body.get<A1 > ("a1");
                    const A2& a2 = body.get<A2 > ("a2");
                    const A3& a3 = body.get<A3 > ("a3");
                    const A4& a4 = body.get<A4 > ("a4");
                    for (size_t i = 0; i < m_slotHandlers.size(); ++i) {
                        m_slotHandlers[i](a1, a2, a3, a4);
                    }
                    invalidateSenderInformation();
                } catch (const karabo::util::CastException& e) {
                    invalidateSenderInformation();
                    KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("Received incompatible arguments (see above) for slot \"" + m_slotFunction + "\". Check your connection!"));
                } catch (const karabo::util::Exception& e) {
                    invalidateSenderInformation();
                    KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("An exception was thrown in slot \"" + m_slotFunction + "\""));
                } catch (...) {
                    invalidateSenderInformation();
                    KARABO_RETHROW_AS(KARABO_SIGNALSLOT_EXCEPTION("An unknown exception was thrown in slot \"" + m_slotFunction + "\""));
                }
            }
            std::vector<SlotHandler> m_slotHandlers;
        };

    }
}

#endif
