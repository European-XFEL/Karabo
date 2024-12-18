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
/*
 * File:   CppInputHandler.hh
 * Author: esenov
 *
 * Created on September 17, 2013, 5:23 PM
 */

#ifndef KARABO_IO_CPPINPUTHANDLER_HH
#define KARABO_IO_CPPINPUTHANDLER_HH

#include <karabo/io/AbstractInput.hh>
#include <karabo/io/InputHandler.hh>

namespace karabo {
    namespace io {

        /**
         * @class CppInputHandler
         * @brief The CppInputHandler specializes the InputHandler class for Karabo's C++ interface.
         *        The handlers used here are std::function objects
         */
        template <class InputType>
        class CppInputHandler : public InputHandler {
           public:
            KARABO_CLASSINFO(CppInputHandler, "CppInputHandler" + std::string(typeid(InputType).name()), "1.0")

            CppInputHandler() {}

            CppInputHandler(const AbstractInput::Pointer& input)
                : m_input(std::static_pointer_cast<InputType>(input)) {}

            virtual ~CppInputHandler() {}

            void registerIOEventHandler(const std::any& ioEventHandler) {
                m_ioEventHandler =
                      std::any_cast<std::function<void(const typename InputType::Pointer&)> >(ioEventHandler);
            }

            void registerEndOfStreamEventHandler(const std::any& endOfStreamEventHandler) {
                m_endOfStreamEventHandler = std::any_cast<std::function<void()> >(endOfStreamEventHandler);
            }

            void triggerIOEvent() {
                if (!m_ioEventHandler) {
                    if (typename InputType::Pointer in = m_input.lock()) m_ioEventHandler(in);
                }
            }

            void triggerEndOfStreamEvent() {
                if (!m_endOfStreamEventHandler) m_endOfStreamEventHandler();
            }

           private:
            std::weak_ptr<InputType> m_input;
            std::function<void(const typename InputType::Pointer&)> m_ioEventHandler;
            std::function<void()> m_endOfStreamEventHandler;
        };

    } // namespace io
} // namespace karabo

#endif /* KARABO_IO_CPPINPUTHANDLER_HH */
