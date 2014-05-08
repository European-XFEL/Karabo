/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import java.util.ArrayList;
import java.util.Arrays;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class ClassInfo extends SchemaFunction {

    private static String classId = "";
    private static String version = "1.0";
    private static String className = "";
    private static String packageName = "";
    private static ClassInfo ci = null;
//    public static int registration = 1;

    public static ClassInfo classInfo(Class<?> cls) {
        className = cls.getSimpleName();
        packageName = cls.getPackage().getName();
        if (cls.isAnnotationPresent(KARABO_CLASSINFO.class)) {
            KARABO_CLASSINFO instance = (KARABO_CLASSINFO) cls.getAnnotation(KARABO_CLASSINFO.class);
            ClassInfo.classId = instance.classId();
            ClassInfo.version = instance.version();
            ci = new ClassInfo();
            return ci;
        }
        return null;
    }

    public ClassInfo getClassInfo() {
        Class<?> cls = this.getClass();
        className = cls.getSimpleName();
        packageName = cls.getPackage().getName();
        if (cls.isAnnotationPresent(KARABO_CLASSINFO.class)) {
            KARABO_CLASSINFO instance = (KARABO_CLASSINFO) cls.getAnnotation(KARABO_CLASSINFO.class);
            ClassInfo.classId = instance.classId();
            ClassInfo.version = instance.version();
            ci = (ClassInfo) this;
            return this;
        }
        return null;
    }

    public String getClassId() {
        if (ci == null) {
            this.getClassInfo();
        }
        return classId;
    }

    public String getVersion() {
        if (ci == null) {
            this.getClassInfo();
        }
        return version;
    }

    public String getClassName() {
        if (ci == null) {
            this.getClassInfo();
        }
        return className;
    }

    public String getNamespace() {
        if (ci == null) {
            this.getClassInfo();
        }
        return packageName;
    }

    public static BoolElement BOOL_ELEMENT(Schema expected) {
        return new BoolElement(expected);
    }

    public static CharElement CHAR_ELEMENT(Schema expected) {
        return new CharElement(expected);
    }

    public static Int8Element INT8_ELEMENT(Schema expected) {
        return new Int8Element(expected);
    }

    public static Int16Element INT16_ELEMENT(Schema expected) {
        return new Int16Element(expected);
    }

    public static Int32Element INT32_ELEMENT(Schema expected) {
        return new Int32Element(expected);
    }

    public static Int64Element INT64_ELEMENT(Schema expected) {
        return new Int64Element(expected);
    }

    public static FloatElement FLOAT_ELEMENT(Schema expected) {
        return new FloatElement(expected);
    }

    public static DoubleElement DOUBLE_ELEMENT(Schema expected) {
        return new DoubleElement(expected);
    }

    public static StringElement STRING_ELEMENT(Schema expected) {
        return new StringElement(expected);
    }

    public static VectorBoolElement VECTOR_BOOL_ELEMENT(Schema expected) {
        return new VectorBoolElement(expected);
    }

    public static VectorCharElement VECTOR_CHAR_ELEMENT(Schema expected) {
        return new VectorCharElement(expected);
    }

    public static VectorInt8Element VECTOR_INT8_ELEMENT(Schema expected) {
        return new VectorInt8Element(expected);
    }

    public static VectorInt16Element VECTOR_INT16_ELEMENT(Schema expected) {
        return new VectorInt16Element(expected);
    }

    public static VectorInt32Element VECTOR_INT32_ELEMENT(Schema expected) {
        return new VectorInt32Element(expected);
    }

    public static VectorInt64Element VECTOR_INT64_ELEMENT(Schema expected) {
        return new VectorInt64Element(expected);
    }

    public static VectorFloatElement VECTOR_FLOAT_ELEMENT(Schema expected) {
        return new VectorFloatElement(expected);
    }

    public static VectorDoubleElement VECTOR_DOUBLE_ELEMENT(Schema expected) {
        return new VectorDoubleElement(expected);
    }

    public static VectorStringElement VECTOR_STRING_ELEMENT(Schema expected) {
        return new VectorStringElement(expected);
    }

    public static NodeElement NODE_ELEMENT(Schema expected) {
        return new NodeElement(expected);
    }

    public static ListElement LIST_ELEMENT(Schema expected) {
        return new ListElement(expected);
    }

    public static ChoiceElement CHOICE_ELEMENT(Schema expected) {
        return new ChoiceElement(expected);
    }

    public static OverwriteElement OVERWRITE_ELEMENT(Schema expected) {
        return new OverwriteElement(expected);
    }

    public static PathElement PATH_ELEMENT(Schema expected) {
        return new PathElement(expected);
    }

    public static SlotElement SLOT_ELEMENT(Schema expected) {
        return new SlotElement(expected);
    }

    public static <T extends ClassInfo> T create(Class<T> cls, Hash configuration, boolean validate) {
        return (T) Configurator.create(cls, configuration, validate);
    }

    public static <T extends ClassInfo> T create(Class<T> cls, Hash configuration) {
        return (T) Configurator.create(cls, configuration);
    }

    public static <T extends ClassInfo> T create(Class<T> cls, String classId, Hash configuration, boolean validate) {
        return (T) Configurator.create(cls, classId, configuration, validate);
    }

    public static <T extends ClassInfo> T create(Class<T> cls, String classId, Hash configuration) {
        return (T) Configurator.create(cls, classId, configuration);
    }

    public static <T extends ClassInfo> T create(Class<T> cls, String classId) {
        return (T) Configurator.create(cls, classId);
    }

    public static <T extends ClassInfo> T createNode(Class<T> cls, String nodeName, String classId, Hash input, boolean validate) {
        return (T) Configurator.createNode(cls, nodeName, classId, input, validate);
    }

    public static <T extends ClassInfo> T createNode(Class<T> cls, String nodeName, String classId, Hash input) {
        return (T) Configurator.createNode(cls, nodeName, classId, input);
    }

    public static <T extends ClassInfo> T createChoice(Class<T> cls, String choiceName, Hash input, boolean validate) {
        return (T) Configurator.createChoice(cls, choiceName, input, validate);
    }

    public static <T extends ClassInfo> T createChoice(Class<T> cls, String choiceName, Hash input) {
        return (T) Configurator.createChoice(cls, choiceName, input);
    }

    public static <T extends ClassInfo> ArrayList<T> createList(Class<T> cls, String listName, Hash input, boolean validate) {
        return (ArrayList<T>) Configurator.createList(cls, listName, input, validate);
    }

    public static <T extends ClassInfo> ArrayList<T> createList(Class<T> cls, String listName, Hash input) {
        return (ArrayList<T>) Configurator.createList(cls, listName, input);
    }

    public static <T extends ClassInfo> Schema getSchema(Class<T> cls, String classId, Schema.AssemblyRules rules) {
        return Configurator.getSchema(cls, classId, rules);
    }

    public static <T extends ClassInfo> Schema getSchema(Class<T> cls, String classId) {
        return Configurator.getSchema(cls, classId);
    }

    public static <Base> void KARABO_REGISTER_FOR_CONFIGURATION(Class<Base> base) {
        if (base.isAnnotationPresent(KARABO_CLASSINFO.class)) {
            KARABO_CLASSINFO instance = (KARABO_CLASSINFO) base.getAnnotation(KARABO_CLASSINFO.class);
            String classid = instance.classId();
            Configurator.registerClass(base, base, classid);
            //System.out.println("*** KARABO_REGISTER_FOR_CONFIGURATION base = " + base.getName() + " : \"" + classid + "\" registered");
        } else {
            throw new RuntimeException("No KARABO_CLASSINFO annotation found for class \"" + base.getSimpleName() + "\"");
        }
    }
    
    public static <Base, Derived extends Base> void KARABO_REGISTER_FOR_CONFIGURATION(Class<Base> base, Class<Derived> derived) {
        ArrayList<Class<?>> chain = inheritanceChain(new ArrayList<Class<?>>(), derived, base);
        if (derived.isAnnotationPresent(KARABO_CLASSINFO.class)) {
            for (Class<?> cls : chain) {
                if (cls.isAnnotationPresent(KARABO_CLASSINFO.class)) {
                    KARABO_CLASSINFO instance = (KARABO_CLASSINFO) cls.getAnnotation(KARABO_CLASSINFO.class);
                    String classid = instance.classId();
                    Configurator.registerClass(base, cls, classid);
                    //System.out.println("*** KARABO_REGISTER_FOR_CONFIGURATION base = " + base.getName() + ", cls = " + cls.getName() + " : \"" + classid + "\" registered");
                }
            }
        } else {
            throw new RuntimeException("No KARABO_CLASSINFO annotation found for class \"" + derived.getSimpleName() + "\"");
        }
    }
    
    protected static ArrayList<Class<?>> inheritanceChain(ArrayList<Class<?>> list, Class<?> top, Class<?> base) {
        if (top == null) {
            return list;
        }
        list.add(0, top);
        if (top.getName().equals(base.getName())) {
            return list;
        }
        return inheritanceChain(list, top.getSuperclass(), base);
    }

}
