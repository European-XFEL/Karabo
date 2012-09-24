/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_CORE_LOWLEVELCONTROLDEVICE_HH
#define	EXFEL_CORE_LOWLEVELCONTROLDEVICE_HH

#include "Device.hh"

/**
 * The main European XFEL namespace
 */
namespace exfel {

  /**
   * Namespace for package core
   */
  namespace core {

    class LowLevelControlDevice : public Device {


    public:

      EXFEL_CLASSINFO("LowLevelControlDevice")

      LowLevelControlDevice() : Device(this){}
      virtual ~LowLevelControlDevice();

      static void expectedParameters(exfel::util::Config& expected);
      void configure(const exfel::util::Config& input);

      void run();

    private:




    };

  }
}

#endif	/* EXFEL_CORE_LOWLEVELCONTROLDEVICE_HH */
