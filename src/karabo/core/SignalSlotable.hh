/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 6, 2011, 2:25 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_CORE_SIGNALSLOTABLE_HH
#define	EXFEL_CORE_SIGNALSLOTABLE_HH

#include <log4cpp/Category.hh>

#include <karabo/util/Factory.hh>
#include <karabo/xms/Signal.hh>
#include <karabo/xms/Slot.hh>

/**
 * The main European XFEL namespace
 */
namespace exfel {

  namespace core {
    
    /**
     * The SignalSlotable class.
     * This class implements the so-called "Signal-Slot" design pattern orginally termed
     * by the Qt-Gui framework. However, signals and slots are not restricted to a local application
     * but can be connected and triggered across the network. This allows for programming with network components
     * in the same intuitive (event-driven) way as Qt allows to do with its local components (e.g. widgets).
     *
     * Moreover does this implementation (unlike Qt) not require any propriatary pre-processing.
     * Another additional feature is the ability to setup new signals and/or slots at runtime.
     *
     * For a full documentation of the signal-slot component see the documentation in the software-guide.
     *
     */
    class SignalSlotable {
    public:

      EXFEL_CLASSINFO(SignalSlotable, "SignalSlotable", "1.0")
      EXFEL_FACTORY_BASE_CLASS

      /**
       * Slots may be of three different types:
       * SPECIFIC: The slot is unique in the given network
       * HOST_ID_INVARIANT: Any signal that fits the slots instanceId and function signature will trigger this slot
       * NETWORK_ID_INVARIANT: Any signal that is connected with a compatible function signature will trigger this slot
       *
       */
      enum SlotType {
        SPECIFIC,
        HOST_ID_INVARIANT,
        NETWORK_ID_INVARIANT
      };

      enum ConnectionType {
        NO_TRACK,
        TRACK,
        RECONNECT
      };

#define SIGNAL0(signalName) registerSignal(signalName);
#define SIGNAL1(signalName, a1) registerSignal<a1>(signalName);
#define SIGNAL2(signalName, a1, a2) registerSignal<a1,a2>(signalName);
#define SIGNAL3(signalName, a1, a2, a3) registerSignal<a1,a2,a3>(signalName);
#define SIGNAL4(signalName, a1, a2, a3, a4) registerSignal<a1,a2,a3,a4>(signalName);

#define SLOT0(slotName) registerSlot(boost::bind(&Self::slotName,this),#slotName);
#define SLOT1(slotName, a1) registerSlot<a1>(boost::bind(&Self::slotName,this,_1),#slotName);
#define SLOT2(slotName, a1, a2) registerSlot<a1,a2>(boost::bind(&Self::slotName,this,_1,_2),#slotName);
#define SLOT3(slotName, a1, a2, a3) registerSlot<a1,a2,a3>(boost::bind(&Self::slotName,this,_1,_2,_3),#slotName);
#define SLOT4(slotName, a1, a2, a3, a4) registerSlot<a1,a2,a3,a4>(boost::bind(&Self::slotName,this,_1,_2,_3,_4),#slotName);

#define STATIC_SLOT0(slotName) registerSlot(boost::bind(&Self::slotName,this),#slotName, HOST_ID_INVARIANT);
#define STATIC_SLOT1(slotName, a1) registerSlot<a1>(boost::bind(&Self::slotName,this,_1),#slotName, HOST_ID_INVARIANT);
#define STATIC_SLOT2(slotName, a1, a2) registerSlot<a1,a2>(boost::bind(&Self::slotName,this,_1,_2),#slotName, HOST_ID_INVARIANT);
#define STATIC_SLOT3(slotName, a1, a2, a3) registerSlot<a1,a2,a3>(boost::bind(&Self::slotName,this,_1,_2,_3),#slotName, HOST_ID_INVARIANT);
#define STATIC_SLOT4(slotName, a1, a2, a3, a4) registerSlot<a1,a2,a3,a4>(boost::bind(&Self::slotName,this,_1,_2,_3,_4),#slotName, HOST_ID_INVARIANT);

#define GLOBAL_SLOT0(slotName) registerSlot(boost::bind(&Self::slotName,this),#slotName, NETWORK_ID_INVARIANT);
#define GLOBAL_SLOT1(slotName, a1) registerSlot<a1>(boost::bind(&Self::slotName,this,_1),#slotName, NETWORK_ID_INVARIANT);
#define GLOBAL_SLOT2(slotName, a1, a2) registerSlot<a1,a2>(boost::bind(&Self::slotName,this,_1,_2),#slotName, NETWORK_ID_INVARIANT);
#define GLOBAL_SLOT3(slotName, a1, a2, a3) registerSlot<a1,a2,a3>(boost::bind(&Self::slotName,this,_1,_2,_3),#slotName, NETWORK_ID_INVARIANT);
#define GLOBAL_SLOT4(slotName, a1, a2, a3, a4) registerSlot<a1,a2,a3,a4>(boost::bind(&Self::slotName,this,_1,_2,_3,_4),#slotName, NETWORK_ID_INVARIANT);

      SignalSlotable();

      SignalSlotable(const exfel::net::BrokerConnection::Pointer& connection, const std::string& instanceId, int heartbeatRate = 5);

      virtual ~SignalSlotable() {
      }

      /**
       * Necessary method as part of the factory/configuration system
       * @param expected [out] Description of expected parameters for this object (Master)
       */
      static void expectedParameters(exfel::util::Config& expected);

      /**
       * If this object is constructed using the factory/configuration system this method is called
       * @param input Validated (@see expectedParameters) and default-filled configuration
       */
      void configure(const exfel::util::Config& input);

      /**
       * Access to the identification of the current host
       * @return hostId
       *
       * TODO Should be static ??
       * 
       */
      const std::string& getHostId() const;

      /**
       * Sets the default hostId that is used on start-up
       * CAVEAT: Will only work PRIOR to the instantiation of the object
       * @param defaultHostId
       */
      static void setDefaultHostId(const std::string& defaultHostId);
      
      /**
       * Sets the id for identifying the current host within the network
       * @param hostId
       */
      void setHostId(const std::string& hostId);
      
      /**
       * Access to the identification of the current instance using signals and slots
       * @return instanceId
       */
      virtual const std::string& getInstanceId() const;

      /**
       * Sets the id that identifies a specific instance making use of signals and slots
       * @param instanceId
       */
      virtual void setInstanceId(const std::string& instanceId);

      /**
       * Access to the networkId. The networkId is composed of the hostId and the instanceId.
       * HostId and instanceId are concatenated with and additional "/" as a separator.
       * The networkId uniquily identifies a slot or a signal in the whole network.
       * @return networkId
       */
      std::string getNetworkId() const;

      void trackExistenceOfComponent(const std::string& networkId);

      /**
       * This function will block the main-thread.
       */
      void runEventLoop(bool emitHeartbeat = true);

      virtual void componentNotAvailable(const std::string& networkId) {
        std::cout << "Component is not available: " << networkId << std::endl;
      }

      virtual void componentAvailableAgain(const std::string& networkId) {
        std::cout << "Component is back: " << networkId << std::endl;
      }

      virtual void connectionNotAvailable(const std::string& slotNetworkId, const exfel::util::Hash& affectedSignals, const exfel::util::Hash& affectedSlots);

      virtual void connectionAvailableAgain(const std::string& slotNetworkId, const exfel::util::Hash& affectedSignals, const exfel::util::Hash& affectedSlots);

      void ping() const;

      /**
       * The slotPing is a default global slot which emits the signalGotPinged signal
       */
      void slotPing();

      void slotShowSignalsAndSlots();

      void slotReceiveSignalsAndSlots(const std::vector<std::string>& signals, const std::vector<std::string>& slots);

      void showSlots(const std::string& networkId = "");

      void showSignalsAndSlots(const std::string& networkId = "");


      /**
       * This function establishes a connection between a signal and a slot.
       * The real functionality of the connection is NEITHER checked initially, NOR tracked during its lifetime.
       * Thus, an emitted signal may never reach the connected slot, still not triggering any error.
       *
       * @param signalNetworkId
       * @param signalSignature
       * @param slotNetworkId
       * @param slotSignature
       */
      void connectN(const std::string& signalNetworkId, const std::string& signalSignature, const std::string& slotNetworkId, const std::string& slotSignature);

      /**
       * This function establishes a connection between a signal and a slot.
       * The real functionality of the connection will be checked initially and be tracked throughout its lifetime.
       * The virtual function connectionNotAvailable(const string& networkId, const Hash& affectedSignals, const Hash& affectedSlots) will be called
       * in case the connection is/gets unavaible. NOTE: This notification is delayed by at least the heartbeat-rate of the connected component.
       *
       * @param signalNetworkId
       * @param signalSignature
       * @param slotNetworkId
       * @param slotSignature
       */
      void connectT(const std::string& signalNetworkId, const std::string& signalSignature, const std::string& slotNetworkId, const std::string& slotSignature);

      /**
       * This function establishes a connection between a signal and a slot.
       * The real functionality of the connection will be checked initially and be tracked throughout its lifetime.
       * The virtual function connectionNotAvailable(const string& networkId, const Hash& affectedSignals, const Hash& affectedSlots) will be called
       * in case the connection is/gets unavaible. NOTE: This notification is delayed by at least the heartbeat-rate of the connected component.
       * Furthermore will the lost connection automatically be tried to be re-established once the lost component(s) get(s) available again.
       * The re-connection requests will fade out with the power of 4 and last maximum 17 hours. After that and at any other time all dangling connections 
       * can actively tried to be re-connected by calling the function "slotTryReconnectNow()".
       *
       * @param signalNetworkId
       * @param signalSignature
       * @param slotNetworkId
       * @param slotSignature
       */
      void connectR(const std::string& signalNetworkId, const std::string& signalSignature, const std::string& slotNetworkId, const std::string& slotSignature);

      /**
       * This function establishes a connection between a signal and a slot.
       * The real functionality of the connection is NEITHER checked initially, NOR tracked during its lifetime.
       * Thus, an emitted signal may never reach the connected slot, still not triggering any error.
       *
       * @param signalId
       * @param slotId
       */
      void connectN(const std::string& signalId, const std::string& slotId);

      /**
       * This function establishes a connection between a signal and a slot.
       * The real functionality of the connection will be checked initially and be tracked throughout its lifetime.
       * The virtual function "connectionNotAvailable(const string& networkId, const Hash& affectedSignals, const Hash& affectedSlots)" will be called
       * in case the connection is/gets unavaible. NOTE: This notification is delayed by at least the heartbeat-rate of the connected component.
       *
       * @param signalId
       * @param slotId
       */
      void connectT(const std::string& signalId, const std::string& slotId);

      /**
       * This function establishes a connection between a signal and a slot.
       * The real functionality of the connection will be checked initially and be tracked throughout its lifetime.
       * The virtual function connectionNotAvailable(const string& networkId, const Hash& affectedSignals, const Hash& affectedSlots) will be called
       * in case the connection is/gets unavaible. NOTE: This notification is delayed by at least the heartbeat-rate of the connected component.
       * Furthermore will the lost connection automatically be tried to be re-established once the lost component(s) get(s) available again.
       * The re-connection requests will fade out with the power of 4 and last maximum 17 hours. After that and at any other time all dangling connections
       * can actively tried to be re-connected by calling the function "slotTryReconnectNow()".
       *
       * @param signalId
       * @param slotId
       */
      void connectR(const std::string& signalId, const std::string& slotId);

      /**
       * This function establishes a connection between a signal and a slot.
       * If the networkId is not given, the signal/slot is interpreted as local and automatically
       * assigned a "self" networkId
       */
      void connect(const std::string& signal, const std::string& slot, ConnectionType connectionType = TRACK);


      /**
       * Connects a signal and slot by explicitely seperating networkId from the slotId/signalId.
       * @param signalNetworkId
       * @param signalSignature
       * @param slotNetworkId
       * @param slotSignature
       */
      void connect(const std::string& signalNetworkId, const std::string& signalSignature,
              const std::string& slotNetworkId, const std::string& slotSignature, ConnectionType connectionType = TRACK);


      void disconnect(const std::string& signal, const std::string& slot);

      /**
       * This function finds all signal keys within a Config object (non-recursive) by a regular expression.
       * It will then automatically establish connections to the corresponding slot values.
       *
       * @param config Config object as obtained by the configure method
       * @param signalRegularExpression A perl-regular expression for the signal key
       */
      void autoConnectAllSignals(const exfel::util::Config& config, const std::string signalRegularExpression = "^signal.*");

      /**
       * This function finds all slot keys within a Config object (non-recursive) by a regular expression.
       * It will then automatically establish connections to the corresponding signal values.
       *
       * @param config Config object as obtained by the configure method
       * @param slotRegularExpression A perl-regular expression for the signal key
       */
      void autoConnectAllSlots(const exfel::util::Config& config, const std::string slotRegularExpression = "^slot.*");

      /**
       * Emits a void signal.
       * @param signalFunction
       */
      void emit(const std::string& signalFunction) const {
        std::string signature(composeSignature(signalFunction));
        boost::shared_ptr < boost::signals2::signal<void ()> > signalPointer;
        m_signalFunctions.get(signature, signalPointer);
        (*signalPointer)();
      }

      /**
       * Emits a signal with one argument.
       * @param signalFunction
       * @param a1
       */
      template <class A1>
      void emit(const std::string& signalFunction, const A1& a1) const {
        try {
          std::string signature(composeSignature<A1 > (signalFunction));
          boost::shared_ptr < boost::signals2::signal<void (const A1&) > > signalPointer;
          m_signalFunctions.get(signature, signalPointer);
          (*signalPointer)(a1);
        } catch (...) {
          RETHROW;
        }
      }

      /**
       * Emits a signal with two arguments.
       * @param signalFunction
       * @param a1
       * @param a2
       */
      template <class A1, class A2>
      void emit(const std::string& signalFunction, const A1& a1, const A2& a2) const {
        try {
          std::string signature(composeSignature<A1, A2 > (signalFunction));
          boost::shared_ptr < boost::signals2::signal<void (const A1&, const A2&) > > signalPointer;
          if (m_signalFunctions.has(signature)) {
            m_signalFunctions.get(signature, signalPointer);
            (*signalPointer)(a1, a2);
          } else {
            std::cout << "WARNING: Signal " << signature << " not registered" << std::endl;
          }
          
        } catch (...) {
          RETHROW;
        }
      }

     /**
      * Emits a signal with three arguments.
      * @param signalFunction
      * @param a1
      * @param a2
      * @param a3
      */
      template <class A1, class A2, class A3>
      void emit(const std::string& signalFunction, const A1& a1, const A2& a2, const A3& a3) const {
        try {
          std::string signature(composeSignature<A1, A2, A3 > (signalFunction));
          boost::shared_ptr < boost::signals2::signal<void (const A1&, const A2&, const A3&) > > signalPointer;
          m_signalFunctions.get(signature, signalPointer);
          (*signalPointer)(a1, a2, a3);
        } catch (...) {
          RETHROW;
        }
      }

      /**
       * Emits a signal with four arguments.
       * @param signalFunction
       * @param a1
       * @param a2
       * @param a3
       * @param a4
       */
      template <class A1, class A2, class A3, class A4>
      void emit(const std::string& signalFunction, const A1& a1, const A2& a2, const A3& a3, const A4& a4) const {
        try {
          std::string signature(composeSignature<A1, A2, A3, A4 > (signalFunction));
          boost::shared_ptr < boost::signals2::signal<void (const A1&, const A2&, const A3&, const A4&) > > signalPointer;
          m_signalFunctions.get(signalFunction, signalPointer);
          (*signalPointer)(a1, a2, a3, a4);
        } catch (...) {
          RETHROW;
        }
      }

    protected: // Functions

      void registerSignal(const std::string& funcName) {
        std::string signature(composeSignature(funcName));
        // Have to use a shared pointer here because a boost::signal2 is non-copyable
        boost::shared_ptr < boost::signals2::signal<void ()> > p(new boost::signals2::signal<void ()>);
        boost::shared_ptr<exfel::xms::Signal> s(new exfel::xms::Signal(signature, m_signalService));
        // Boost manual: By default, bind makes a copy of the provided function object (i.e. here a copy of a pointer)
        p->connect(boost::bind(&exfel::xms::Signal::emit0, s));
        m_signalFunctions.set(signature, p);
        m_signalInstances[signature] = s;
      }

      template <class A1>
      void registerSignal(const std::string& funcName) {

        std::string signature(composeSignature<A1 > (funcName));
        boost::shared_ptr < boost::signals2::signal<void (const A1&) > > p(new boost::signals2::signal<void (const A1&) >);
        boost::shared_ptr<exfel::xms::Signal> s(new exfel::xms::Signal(signature, m_signalService));
        p->connect(boost::bind(&exfel::xms::Signal::emit1<A1>, s, _1));
        m_signalFunctions.set(signature, p);
        m_signalInstances[signature] = s;
      }

      template <class A1, class A2>
      void registerSignal(const std::string& funcName) {

        std::string signature(composeSignature<A1, A2 > (funcName));
        boost::shared_ptr < boost::signals2::signal<void (const A1&, const A2&) > > p(new boost::signals2::signal<void (const A1&, const A2&) >);
        boost::shared_ptr<exfel::xms::Signal> s(new exfel::xms::Signal(signature, m_signalService));
        p->connect(boost::bind(&exfel::xms::Signal::emit2<A1, A2>, s, _1, _2));
        m_signalFunctions.set(signature, p);
        m_signalInstances[signature] = s;
      }

      template <class A1, class A2, class A3>
      void registerSignal(const std::string& funcName) {

        std::string signature(composeSignature<A1, A2, A3 > (funcName));
        boost::shared_ptr < boost::signals2::signal<void (const A1&, const A2&, const A3&) > > p(new boost::signals2::signal<void (const A1&, const A2&, const A3&) >);
        boost::shared_ptr<exfel::xms::Signal> s(new exfel::xms::Signal(signature, m_signalService));
        p->connect(boost::bind(&exfel::xms::Signal::emit3<A1, A2, A3>, s, _1, _2, _3));
        m_signalFunctions.set(signature, p);
        m_signalInstances[signature] = s;
      }

      template <class A1, class A2, class A3, class A4>
      void registerSignal(const std::string& funcName) {

        std::string signature(composeSignature<A1, A2, A3, A4 > (funcName));
        boost::shared_ptr < boost::signals2::signal<void (const A1&, const A2&, const A3&, const A4&) > > p(new boost::signals2::signal<void (const A1&, const A2&, const A3&, const A4&) >);
        boost::shared_ptr<exfel::xms::Signal> s(new exfel::xms::Signal(signature, m_signalService));
        p->connect(boost::bind(&exfel::xms::Signal::emit4<A1, A2, A3, A4>, s, _1, _2, _3, _4));
        m_signalFunctions.set(signature, p);
        m_signalInstances[signature] = s;
      }

      void registerSlot(const boost::function<void () >& slot, const std::string& funcName, const SlotType& slotType = SPECIFIC) {
        std::string slotId = composeSignature(funcName, slotType);
        if (m_slotServices.find(slotId) != m_slotServices.end()) return; // Already registered
        exfel::net::Service::Pointer service = m_connection->createService(); // New Service
        boost::shared_ptr<exfel::xms::Slot0> s(new exfel::xms::Slot0(service, slotId)); // New Slot
        s->processAsync(slot); // Bind user's slot-function to Slot
        m_slots.push_back(s); // Keep Slot alive
        m_threadGroup.create_thread(boost::bind(&exfel::net::Service::run, service)); // Start thread for Service's run
        m_slotServices[slotId] = service; // Store Service using slotId as key
      }

      template <class A1>
      void registerSlot(const boost::function<void (const A1&) >& slot, const std::string& funcName, const SlotType& slotType = SPECIFIC) {
        std::string slotId = composeSignature<A1 > (funcName, slotType);
        //std::cout << slotId << std::endl;
        if (m_slotServices.find(slotId) != m_slotServices.end()) return;
        exfel::net::Service::Pointer service = m_connection->createService();
        boost::shared_ptr<exfel::xms::Slot1<A1> > s(new exfel::xms::Slot1<A1 > (service, slotId));
        s->processAsync(slot);
        m_slots.push_back(s);
        m_threadGroup.create_thread(boost::bind(&exfel::net::Service::run, service));
        m_slotServices[slotId] = service;
      }

      template <class A1, class A2>
      void registerSlot(const boost::function<void (const A1&, const A2&) >& slot, const std::string& funcName, const SlotType& slotType = SPECIFIC) {
        std::string slotId = composeSignature<A1, A2 > (funcName, slotType);
        if (m_slotServices.find(slotId) != m_slotServices.end()) return;
        exfel::net::Service::Pointer service = m_connection->createService();
        boost::shared_ptr<exfel::xms::Slot2<A1, A2> > s(new exfel::xms::Slot2<A1, A2 > (service, slotId));
        s->processAsync(slot);
        m_slots.push_back(s);
        m_threadGroup.create_thread(boost::bind(&exfel::net::Service::run, service));
        m_slotServices[slotId] = service;
      }

      template <class A1, class A2, class A3>
      void registerSlot(const boost::function<void (const A1&, const A2&, const A3&) >& slot, const std::string& funcName, const SlotType& slotType = SPECIFIC) {
        std::string slotId = composeSignature<A1, A2, A3 > (funcName, slotType);
        if (m_slotServices.find(slotId) != m_slotServices.end()) return;
        exfel::net::Service::Pointer service = m_connection->createService();
        boost::shared_ptr<exfel::xms::Slot3<A1, A2, A3> > s(new exfel::xms::Slot3<A1, A2, A3 > (service, slotId));
        s->processAsync(slot);
        m_slots.push_back(s);
        m_threadGroup.create_thread(boost::bind(&exfel::net::Service::run, service));
        m_slotServices[slotId] = service;
      }

      template <class A1, class A2, class A3, class A4>
      void registerSlot(const boost::function<void (const A1&, const A2&, const A3&, const A4&) >& slot, const std::string& funcName, const SlotType& slotType = SPECIFIC) {
        std::string slotId = composeSignature<A1, A2, A3, A4 > (funcName, slotType);
        if (m_slotServices.find(slotId) != m_slotServices.end()) return;
        exfel::net::Service::Pointer service = m_connection->createService();
        boost::shared_ptr<exfel::xms::Slot4<A1, A2, A3, A4> > s(new exfel::xms::Slot4<A1, A2, A3, A4 > (service, slotId));
        s->processAsync(slot);
        m_slots.push_back(s);
        m_threadGroup.create_thread(boost::bind(&exfel::net::Service::run, service));
        m_slotServices[slotId] = service;
      }
      
      std::string fetchNetworkId(const std::string& signalOrSlotId) const;
      
      std::string fetchHostId(const std::string& anyId) const;

    protected: // Member variables

      static std::string m_hostId;
      std::string m_instanceId;

    private: // Functions

      void registerDefaultSignalsAndSlots();

      void initReconnectIntervals();

      void startTrackingSystem();

      std::string composeSignature(const std::string& funcName, const SlotType& slotType = SPECIFIC) const {
        std::string functionSignature = prepareFunctionSignature(funcName);
        std::string networkId = prepareNetworkId(slotType);
        return networkId + functionSignature;
      }

      template <class A1>
      std::string composeSignature(const std::string& funcName, const SlotType& slotType = SPECIFIC) const {
        std::string functionSignature = prepareFunctionSignature<A1 > (funcName);
        std::string networkId = prepareNetworkId(slotType);
        return networkId + functionSignature;
      }

      template <class A1, class A2>
      std::string composeSignature(const std::string& funcName, const SlotType& slotType = SPECIFIC) const {
        std::string functionSignature = prepareFunctionSignature<A1, A2 > (funcName);
        std::string networkId = prepareNetworkId(slotType);
        return networkId + functionSignature;
      }

      template <class A1, class A2, class A3>
      std::string composeSignature(const std::string& funcName, const SlotType& slotType = SPECIFIC) const {
        std::string functionSignature = prepareFunctionSignature<A1, A2, A3 > (funcName);
        std::string networkId = prepareNetworkId(slotType);
        return networkId + functionSignature;
      }

      template <class A1, class A2, class A3, class A4>
      std::string composeSignature(const std::string& funcName, const SlotType& slotType = SPECIFIC) const {
        std::string functionSignature = prepareFunctionSignature<A1, A2, A3, A4 > (funcName);
        std::string networkId = prepareNetworkId(slotType);
        return networkId + functionSignature;
      }

      std::string prepareFunctionSignature(const std::string& funcName) const {
        std::string f(boost::trim_copy(funcName));
        return f;
      }

      template <class A1>
      std::string prepareFunctionSignature(const std::string& funcName) const {
        std::string f(boost::trim_copy(funcName));
        const int typeFormat = exfel::util::Types::FORMAT_INTERN;
        exfel::util::Types& t = exfel::util::Types::getInstance();
        std::string a1 = t.getTypeAsString<A1, typeFormat > ();
        return f + "-" + a1;
      }

      template <class A1, class A2>
      std::string prepareFunctionSignature(const std::string& funcName) const {
        std::string f(boost::trim_copy(funcName));
        const int typeFormat = exfel::util::Types::FORMAT_INTERN;
        exfel::util::Types& t = exfel::util::Types::getInstance();
        std::string a1 = t.getTypeAsString<A1, typeFormat > ();
        std::string a2 = t.getTypeAsString<A2, typeFormat > ();
        return f + "-" + a1 + "-" + a2;
      }

      template <class A1, class A2, class A3>
      std::string prepareFunctionSignature(const std::string& funcName) const {
        std::string f(boost::trim_copy(funcName));
        exfel::util::Types& t = exfel::util::Types::getInstance();
        const int typeFormat = exfel::util::Types::FORMAT_INTERN;
        std::string a1 = t.getTypeAsString<A1, typeFormat> ();
        std::string a2 = t.getTypeAsString<A2, typeFormat> ();
        std::string a3 = t.getTypeAsString<A3, typeFormat> ();
        return f + "-" + a1 + "-" + a2 + "-" + a3;
      }

      template <class A1, class A2, class A3, class A4>
      std::string prepareFunctionSignature(const std::string& funcName) const {
        std::string f(boost::trim_copy(funcName));
        exfel::util::Types& t = exfel::util::Types::getInstance();
        const int typeFormat = exfel::util::Types::FORMAT_INTERN;
        std::string a1 = t.getTypeAsString<A1, typeFormat> ();
        std::string a2 = t.getTypeAsString<A2, typeFormat> ();
        std::string a3 = t.getTypeAsString<A3, typeFormat> ();
        std::string a4 = t.getTypeAsString<A4, typeFormat> ();
        return f + "-" + a1 + "-" + a2 + "-" + a3 + "-" + a4;
      }

      std::string prepareNetworkId(const SlotType& slotType) const {
        std::string networkId;
        if (slotType == SPECIFIC)
          networkId = getHostId() + "/" + getInstanceId() + "/";
        else if (slotType == HOST_ID_INVARIANT)
          networkId = getInstanceId() + "/";
        else if (slotType == NETWORK_ID_INVARIANT)
          networkId = "";
        return networkId;
      }

      std::string specifySignature(const std::string& signature);
      
      void slotConnect(const std::string& signal, const std::string& slot);

      void slotDisconnect(const std::string& signal, const std::string& slot);

      void slotHeartbeat(const std::string& networkId, const int& timeToLive);

      void refreshTimeToLiveForConnectedSlot(const std::string& networkId, int timeToLive);

      void letConnectionSlowlyDieWithoutHeartbeat();

      void trackExistenceOfConnection(const std::string& signalId, const std::string& slotId, ConnectionType connectionType);

      void stopTrackingExistenceOfConnection(const std::string& signalId, const std::string& slotId);

      void addTrackedComponent(const std::string& networkId);

      exfel::util::Hash prepareConnectionNotAvailableInformation(const exfel::util::Hash& signals) const;

      void slotTryReconnectNow();

    private: // Member variables

      typedef boost::shared_ptr<exfel::xms::Signal> SignalInstancePointer;
      typedef std::map<std::string, SignalInstancePointer> SignalInstances;
      typedef SignalInstances::const_iterator SignalInstancesConstIt;
      SignalInstances m_signalInstances;

      boost::thread_group m_threadGroup;
      exfel::net::BrokerConnection::Pointer m_connection;
      exfel::net::Service::Pointer m_signalService;
      exfel::util::Hash m_signalFunctions;
      std::vector<boost::any> m_slots;

      typedef std::map<std::string, exfel::net::Service::Pointer> SlotServices;
      typedef SlotServices::const_iterator SlotServicesConstIt;
      SlotServices m_slotServices;

      typedef std::pair<std::string,int> AssocEntry;
      typedef std::set<AssocEntry> AssocType;
      typedef AssocType::const_iterator AssocTypeConstIterator;

      exfel::util::Hash m_trackedComponents;
      int m_timeToLive;
      static std::set<int> m_reconnectIntervals;
      
      boost::mutex m_connectMutex;
      boost::mutex m_heartbeatMutex;


    };

  } // namespace core
} // namespace exfel

#endif
