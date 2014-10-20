/* 
 * Author: <burkhard.heisen>
 *
 * Created on February 5, 2013, 11:06 AM
 */

#include "ConfigurationTestClasses.hh"

namespace configurationTest {

    KARABO_REGISTER_FOR_CONFIGURATION(Shape, Circle);
    KARABO_REGISTER_FOR_CONFIGURATION(Shape, Circle, EditableCircle);
    KARABO_REGISTER_FOR_CONFIGURATION(Shape, Rectangle);
    KARABO_REGISTER_FOR_CONFIGURATION(GraphicsRenderer);
    KARABO_REGISTER_FOR_CONFIGURATION(TestStruct1, TestStruct2);
    KARABO_REGISTER_FOR_CONFIGURATION(SchemaNodeElements);

}