/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_LOWLEVELCONTROLDEVICE_HH
#define	KARABO_CORE_LOWLEVELCONTROLDEVICE_HH

#include "Device.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

  /**
   * Namespace for package core
   */
  namespace core {

    class LowLevelControlDevice : public Device {


    public:

      KARABO_CLASSINFO("LowLevelControlDevice")

      LowLevelControlDevice() : Device(this){}
      virtual ~LowLevelControlDevice();

      static void expectedParameters(karabo::util::Config& expected);
      void configure(const karabo::util::Config& input);

      void run();

    private:




    };

  }
}

#endif	/* KARABO_CORE_LOWLEVELCONTROLDEVICE_HH */
