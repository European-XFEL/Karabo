/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "Motor.hh"
#include <string>
#include <iostream>


using namespace std;

namespace exfel {
    namespace util {

        EXFEL_REGISTER_FACTORY_CC(Vehicle, Motor)

        void Motor::expectedParameters(Schema& expected) {

            try {

                FLOAT_ELEMENT(expected)
                        .key("position")
                        .displayedName("Position")
                        .description("Absolute position")
                        .assignmentOptional().defaultValue(0.0)
                        .reconfigurable()
                        .commit();

                FLOAT_ELEMENT(expected)
                        .key("offset")
                        .displayedName("Offset")
                        .description("Reference point for DialPosition")
                        .assignmentOptional().defaultValue(0.0)
                        .reconfigurable()
                        .commit()
                        //              .monitorable()
                        ;

                FLOAT_ELEMENT(expected)
                        .key("velocity")
                        .displayedName("Velocity")
                        .description("Velocity of the motor")
                        .minInc(0.0).maxInc(20.0)
                        .assignmentOptional().defaultValue(1.0)
                        .reconfigurable()
                        .commit();

                VectorElement<unsigned short>(expected)
                        .key("steps")
                        .displayedName("Steps")
                        .description("blabla")
                        .minSize(5).maxSize(5)
                        .assignmentOptional().defaultValue(vector<unsigned short>(5, 10))
                        .init()
                        .commit();

            } catch (...) {
                RETHROW;
            }

        }

        void Motor::configure(const Hash& conf) {
            try {
                m_initialParams = conf;

                //
                m_reconfigurationMaster = Vehicle::reconfigurableParameters("Motor");
                Hash empty("Motor", Hash());

                m_reconfigurationParams = m_reconfigurationMaster.validate(empty);

                m_monitorMaster = Vehicle::monitorableParameters("Motor");

                //m_position = conf.get<float>("position");
                m_velocity = conf.get<float>("velocity");
                m_offset = conf.get<float>("offset");

            } catch (...) {
                RETHROW_AS(PARAMETER_EXCEPTION("MOTOR"))
            }


        }

        void Motor::reconfigure(const Hash& conf) {
            try {
                //      cout << "Reconfiguration original: " << m_params << endl;
                //      cout << "Reconfiguration config: " << conf << endl;
                m_reconfigurationParams.update(conf);
                m_reconfigurationMaster.validate(m_reconfigurationParams);
                //      cout << "Reconfiguration updated: " << m_params << endl;
            } catch (...) {
                RETHROW_AS(PARAMETER_EXCEPTION("MOTOR"))
            }
        }

        void Motor::start() {
            try {
                cout << "velocity: " << m_reconfigurationParams.getFromPath<float>("Motor.velocity") << endl;
                cout << "monitor\n" << m_monitorMaster << endl;
            } catch (...) {
                RETHROW_AS(PARAMETER_EXCEPTION("MOTOR"))
            }
        }
    }
}
