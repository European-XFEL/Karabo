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

#include <boost/type_traits.hpp>

#include <karabo/util/Factory.hh>
#include <karabo/net/BrokerChannel.hh>
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

            SignalSlotable* m_signalSlotable;
            std::string m_slotFunction;

            Slot(SignalSlotable* signalSlotable, const std::string& slotFunction)
            : m_signalSlotable(signalSlotable), m_slotFunction(slotFunction) {
            }

            //void handlePossibleReply(const karabo::util::Hash& header);

            void extractSenderInformation(const karabo::util::Hash& header);

            void invalidateSenderInformation();

            virtual void callRegisteredSlotFunctions(const karabo::util::Hash& header, const karabo::util::Hash& body) = 0;
        };

        class Slot0 : public Slot {
            typedef boost::function<void () > SlotHandler;

        public:

            KARABO_CLASSINFO(Slot0, "Slot0", "1.0")

            Slot0(SignalSlotable* signalSlotable, const std::string& slotFunction)
            : Slot(signalSlotable, slotFunction) {
            }

            virtual ~Slot0() {
            }

            void registerSlotFunction(const SlotHandler& slotHandler) {
                m_slotHandlers.push_back(slotHandler);
            }

        private:

            void callRegisteredSlotFunctions(const karabo::util::Hash& header, const karabo::util::Hash& body) {
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

            Slot1(SignalSlotable* signalSlotable, const std::string& slotFunction) : Slot(signalSlotable, slotFunction) {
            }

            virtual ~Slot1() {
            }

            void registerSlotFunction(const SlotHandler& slotHandler) {
                m_slotHandlers.push_back(slotHandler);
            }

        private:

            void callRegisteredSlotFunctions(const karabo::util::Hash& header, const karabo::util::Hash& body) {
                try {
                    extractSenderInformation(header);
                    const A1& a1 = body.get<A1>("a1");
                    for (size_t i = 0; i < m_slotHandlers.size(); ++i) {
                        m_slotHandlers[i](a1);
                    }
                    invalidateSenderInformation();
                } catch (const karabo::util::CastException& e) {
                    karabo::util::Exception::addToTrace(e);
                    KARABO_LOG_FRAMEWORK_ERROR << KARABO_SIGNALSLOT_EXCEPTION("Received incompatible argument (see above) for slot \"" + m_slotFunction + "\". Check your connection!");
                    invalidateSenderInformation();
                } catch (const karabo::util::Exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "An exception was thrown in slot \"" << m_slotFunction << "\": " << e;
                } catch (...) {
                    KARABO_LOG_FRAMEWORK_ERROR << "An unknown exception was thrown in slot \"" << m_slotFunction << "\"";
                }
            }

            std::vector<SlotHandler> m_slotHandlers;
        };

        template <class A1, class A2>
        class Slot2 : public Slot {
            typedef boost::function<void (const A1&, const A2&) > SlotHandler;

        public:

            KARABO_CLASSINFO(Slot2, "Slot2", "1.0")

            Slot2(SignalSlotable* signalSlotable, const std::string& slotFunction)
            : Slot(signalSlotable, slotFunction) {               
            }

            virtual ~Slot2() {
            }

            void registerSlotFunction(const SlotHandler& slotHandler) {
                m_slotHandlers.push_back(slotHandler);
            }

        private:

            void callRegisteredSlotFunctions(const karabo::util::Hash& header, const karabo::util::Hash& body) {
                try {
                    extractSenderInformation(header);
                    const A1& a1 = body.get<A1>("a1");
                    const A2& a2 = body.get<A2>("a2");
                    for (size_t i = 0; i < m_slotHandlers.size(); ++i) {
                        m_slotHandlers[i](a1, a2);
                    }
                    invalidateSenderInformation();
                } catch (const karabo::util::CastException& e) {
                    karabo::util::Exception::addToTrace(e);
                    std::cout << KARABO_SIGNALSLOT_EXCEPTION("Received incompatible arguments (see above) for slot \"" + m_slotFunction + "\". Check your connection!") << std::endl;
                    invalidateSenderInformation();
                } catch (const karabo::util::Exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "An exception was thrown in slot \"" << m_slotFunction << "\": " << e;
                } catch (...) {
                    KARABO_LOG_FRAMEWORK_ERROR << "An unknown exception was thrown in slot \"" << m_slotFunction << "\"";
                }
            }

            std::vector<SlotHandler> m_slotHandlers;
        };

        template <class A1, class A2, class A3>
        class Slot3 : public Slot {
            typedef boost::function<void (const A1&, const A2&, const A3&) > SlotHandler;

        public:

            KARABO_CLASSINFO(Slot3, "Slot3", "1.0")

            Slot3(SignalSlotable* signalSlotable, const std::string& slotFunction)
            : Slot(signalSlotable, slotFunction) {               
            }

            virtual ~Slot3() {
            }

            void registerSlotFunction(const SlotHandler& slotHandler) {
                m_slotHandlers.push_back(slotHandler);
            }

        private:

            void callRegisteredSlotFunctions(const karabo::util::Hash& header, const karabo::util::Hash& body) {
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
                    karabo::util::Exception::addToTrace(e);
                    std::cout << KARABO_SIGNALSLOT_EXCEPTION("Received incompatible arguments (see above) for slot \"" + m_slotFunction + "\". Check your connection!") << std::endl;
                    invalidateSenderInformation();
                } catch (const karabo::util::Exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "An exception was thrown in slot \"" << m_slotFunction << "\": " << e;
                } catch (...) {
                    KARABO_LOG_FRAMEWORK_ERROR << "An unknown exception was thrown in slot \"" << m_slotFunction << "\"";
                }
            }

            std::vector<SlotHandler> m_slotHandlers;
        };

        template <class A1, class A2, class A3, class A4>
        class Slot4 : public Slot {
            typedef boost::function<void (const A1&, const A2&, const A3&, const A4&) > SlotHandler;

        public:

            KARABO_CLASSINFO(Slot4, "Slot4", "1.0")

            Slot4(SignalSlotable* signalSlotable, const std::string& slotFunction)
            : Slot(signalSlotable, slotFunction) {                
            }

            virtual ~Slot4() {
            }

            void registerSlotFunction(const SlotHandler& slotHandler) {
                m_slotHandlers.push_back(slotHandler);
            }

        private:

            void callRegisteredSlotFunctions(const karabo::util::Hash& header, const karabo::util::Hash& body) {
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
                    karabo::util::Exception::addToTrace(e);
                    std::cout << KARABO_SIGNALSLOT_EXCEPTION("Received incompatible arguments (see above) for slot \"" + m_slotFunction + "\". Check your connection!") << std::endl;
                    invalidateSenderInformation();
                } catch (const karabo::util::Exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "An exception was thrown in slot \"" << m_slotFunction << "\": " << e;
                } catch (...) {
                    KARABO_LOG_FRAMEWORK_ERROR << "An unknown exception was thrown in slot \"" << m_slotFunction << "\"";
                }
            }
            std::vector<SlotHandler> m_slotHandlers;
        };

    } // namespace xms
} // namespace karabo

#endif
