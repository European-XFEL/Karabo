/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;
import static karabo.util.ClassInfo.inheritanceChain;
import karabo.util.vectors.VectorHash;
import karabo.util.vectors.VectorString;

/**
 * 
 * @author Sergey Esenov serguei.essenov at xfel.eu
 * @param <Base> anything that inherits from Object
 */
@KARABO_CLASSINFO(classId = "Configurator", version = "1.0")
public class Configurator<Base> {

    private final Map<String, Map<String, Class<?>>> m_registry = new LinkedHashMap<>();
    private static String m_default;
    private Class<Base> base;
    private static Map<Class, Configurator> configurators = new LinkedHashMap<>();

    private static <BaseClass> Configurator init(Class<BaseClass> base) {
        if (configurators.get(base) == null) {
            configurators.put(base, new Configurator<>(base));
        }
        return configurators.get(base);
    }

    private Configurator(Class<Base> base) {
        this.base = base;
    }

    private static <B> String ctorKey(Class<B> base) {
        return base.getName() + "_" + Hash.class.getName();
    }

    private <B, A1> String ctorKey(Class<B> base, Class<A1> c1) {
        String h = base.getName() + "_" + Hash.class.getName();
        String a1 = c1.getName();
        return h + "," + a1;
    }

    private <B, A1, A2> String ctorKey(Class<B> base, Class<A1> c1, Class<A2> c2) {
        String h = base.getName() + "_" + Hash.class.getName();
        String a1 = c1.getName();
        String a2 = c2.getName();
        return h + "," + a1 + "," + a2;
    }

    private <B, A1, A2, A3> String ctorKey(Class<B> base, Class<A1> c1, Class<A2> c2, Class<A3> c3) {
        String h = base.getName() + "_" + Hash.class.getName();
        String a1 = c1.getName();
        String a2 = c2.getName();
        String a3 = c3.getName();
        return h + "," + a1 + "," + a2 + "," + a3;
    }

    public static <BaseClass, DerivedClass> void registerClass(Class<BaseClass> base, Class<DerivedClass> derived, String derivedClassId) {
        Configurator configurator = Configurator.init(base);
        if (!configurator.m_registry.containsKey(derivedClassId)) {
            configurator.m_registry.put(derivedClassId, new LinkedHashMap());
        }
        Map<String, Class<?>> r = (Map) configurator.m_registry.get(derivedClassId);
        String ctorStr = ctorKey(base);
        if (!r.containsKey(ctorStr)) {
            r.put(ctorStr, derived);
            //System.out.println("*** registerClass: classId is \"" + derivedClassId + "\" and ctor string is \"" + ctorStr + "\" -> class " + derived.getName());
        }
    }

    public void setDefault(String classId) {
        Configurator.m_default = classId;
    }

    public static Schema getSchema(Class base, String classId, Schema.AssemblyRules rules) {
        Configurator configurator = Configurator.init(base);
        Schema schema = new Schema(classId, rules);
        //System.out.println("Schema getSchema(Class base, String classId = " + classId + ", Schema.AssemblyRules rules)");
        if (configurator.m_registry.containsKey(classId)) {
            Map<String, Class<?>> r = (Map<String, Class<?>>) configurator.m_registry.get(classId);
            String ctorStr = (String)ctorKey(base);
            if (r.containsKey(ctorStr)) {
                Class<?> classObj = r.get(ctorStr);
                ArrayList<Class<?>> chain = inheritanceChain(new ArrayList<Class<?>>(), classObj, base);
                //System.out.println("getSchema inheritance chain : " + chain);
                for (Class<?> cls : chain) {
                    try {
                        Method m = cls.getDeclaredMethod("expectedParameters", new Class[]{Schema.class});
                        m.invoke(null, new Object[]{schema});
                    } catch (NoSuchMethodException ex) {
                    } catch (SecurityException | IllegalAccessException | IllegalArgumentException | InvocationTargetException ex) {
                        Logger.getLogger(Configurator.class.getName()).log(Level.SEVERE, null, ex);
                    }
                }
            } else {
                Logger.getLogger(Configurator.class.getName()).log(Level.WARNING,
                        "Subregistry for \"{0}\" does not contain ctor string \"{1}\".  Use KARABO_REGISTER_FOR_CONFIGURATION first.",
                        new Object[]{classId, ctorStr});
                return null;
            }
        } else {
            Logger.getLogger(Configurator.class.getName()).log(Level.WARNING,
                    "Registry does not contain \"{0}\".  Use KARABO_REGISTER_FOR_CONFIGURATION first.", classId);
            return null;
        }
        return schema;
    }

    public static Schema getSchema(Class base, String classId) {
        Configurator configurator = Configurator.init(base);
        Schema schema = new Schema(classId);
        //System.out.println("Schema getSchema(Class base, String classId = " + classId + ")");
        if (configurator.m_registry.containsKey(classId)) {
            Map<String, Class<?>> r = (Map<String, Class<?>>) configurator.m_registry.get(classId);
            String ctorStr = ctorKey(base);
            if (r.containsKey(ctorStr)) {
                Class<?> classObj = r.get(ctorStr);
                ArrayList<Class<?>> chain = inheritanceChain(new ArrayList<Class<?>>(), classObj, base);
                //System.out.println("getSchema inheritance chain : " + chain);
                for (Class<?> cls : chain) {
                    try {
                        Method m = cls.getDeclaredMethod("expectedParameters", new Class[]{Schema.class});
                        m.invoke(null, new Object[]{schema});
                    } catch (NoSuchMethodException ex) {
                    } catch (SecurityException | IllegalAccessException | IllegalArgumentException | InvocationTargetException ex) {
                        Logger.getLogger(Configurator.class.getName()).log(Level.SEVERE, null, ex);
                    }
                }
            } else {
                Logger.getLogger(Configurator.class.getName()).log(Level.WARNING,
                        "Subregistry for \"{0}\" does not contain ctor string \"{1}\".  Use KARABO_REGISTER_FOR_CONFIGURATION first.",
                        new Object[]{classId, ctorStr});
                return null;
            }
        } else {
            Logger.getLogger(Configurator.class.getName()).log(Level.WARNING,
                    "Registry does not contain \"{0}\".  Use KARABO_REGISTER_FOR_CONFIGURATION first.", classId);
            return null;
        }
        return schema;
    }

    public static void validateConfiguration(Class base, String classId, Hash configuration, Hash validated) {
        Schema schema = getSchema(base, classId);
        Validator validator = new Validator(); // Default validation
        ValidatorResult ret = validator.validate(schema, configuration, validated);
        if (!ret.success) {
            throw new RuntimeException("Validation failed. \n" + ret.message);
        }
    }

    public static <Base> Base createDefault(Class<Base> base, boolean validate) {
        Configurator configurator = Configurator.init(base);
        String defaultClassId = Configurator.m_default;
        if (defaultClassId.isEmpty()) {
            throw new RuntimeException("No default was defined");
        }
        return create(base, defaultClassId, new Hash(), validate);
    }

    public static <Base> Base createDefault(Class<Base> base) {
        return createDefault(base, true);
    }

    public static <Base, Derived extends Base> Base create(Class<Base> base, String classId, Hash configuration, boolean validate) {
        Configurator configurator = Configurator.init(base);
        if (!configurator.m_registry.containsKey(classId)) {
            throw new RuntimeException("ClassId: '" + classId + "' is not registered.");
        }
        Map<String, Class<?>> r = (Map<String, Class<?>>) configurator.m_registry.get(classId);
        String ctorStr = ctorKey(base);
        if (!r.containsKey(ctorStr)) {
            throw new RuntimeException("Ctor string: '" + ctorStr + "' is not registered in base registry '" + classId + "'.");
        }

        Class<?> derived = r.get(ctorStr);

//        System.out.println("*** Configurator: classId is \"" + classId + "\", ctor string is \""
//                + ctorStr + "\", Base class is \"" + base.getName() + "\", Derived class is \"" + derived.getName() + "\"");
        Constructor ctor;
        try {
            ctor = derived.getDeclaredConstructor(Hash.class);
        } catch (NoSuchMethodException | SecurityException ex) {
            Logger.getLogger(Configurator.class.getName()).log(Level.SEVERE, null, ex);
            return null;
        }

        try {
            if (validate) {
                Hash validated = new Hash();
                validateConfiguration(base, classId, configuration, validated);
                return (Base) ctor.newInstance(validated);
            } else {
                return (Base) ctor.newInstance(configuration);
            }
        } catch (InstantiationException | IllegalAccessException | IllegalArgumentException | InvocationTargetException ex) {
            Logger.getLogger(Configurator.class.getName()).log(Level.SEVERE, null, ex);
            return null;
        }
    }

    public static <Base, Derived extends Base> Base create(Class<Base> base, String classId, Hash configuration) {
        return create(base, classId, configuration, true);
    }

    public static <Base, Derived extends Base> Base create(Class<Base> base, String classId) {
        return create(base, classId, new Hash());
    }

    public static KeyValue<String, Hash> splitIntoClassIdAndConfiguration(Hash rootedConfiguration) {
        if (rootedConfiguration.size() != 1) {
            throw new RuntimeException("Expecting exactly one (root-)node identifying the classId in configuration");
        }
        Node node = rootedConfiguration.iterator().next().getValue();
        String classid = node.getKey();
        Hash config = (Hash) node.getValue();
        return new KeyValue<>(classid, config);
    }

    public static <Base> Base create(Class<Base> base, Hash configuration, boolean validate) {
        try {
            KeyValue<String, Hash> p = splitIntoClassIdAndConfiguration(configuration);
            return create(base, p.key, p.value, validate);
        } catch (Exception e) {
            throw new RuntimeException("This create method expects a rooted Hash with the root node name specifying the classId");
        }
    }

    public static <Base> Base create(Class<Base> base, Hash configuration) {
        return create(base, configuration, true);
    }

    public static <Base> Base createNode(Class<Base> base, String nodeName, String classId, Hash input, boolean validate) {
        if (input.has(nodeName)) {
            return create(base, classId, (Hash) input.get(nodeName), validate);
        } else {
            throw new RuntimeException("Given nodeName \"" + nodeName + "\" is not part of input configuration");
        }
    }

    public static <Base> Base createNode(Class<Base> base, String nodeName, String classId, Hash input) {
        return createNode(base, nodeName, classId, input, true);
    }

    public static <Base> Base createChoice(Class<Base> base, String choiceName, Hash input, boolean validate) {
        if (input.has(choiceName)) {
            return create(base, (Hash) input.get(choiceName), validate);
        } else {
            throw new RuntimeException("Given choiceName \"" + choiceName + "\" is not part of input configuration");
        }
    }

    public static <Base> Base createChoice(Class<Base> base, String choiceName, Hash input) {
        return createChoice(base, choiceName, input, true);
    }

    public static <Base> ArrayList<Base> createList(Class<Base> base, String listName, Hash input, boolean validate) {
        if (input.has(listName)) {
            VectorHash tmp = input.get(listName);
            ArrayList<Base> instances = new ArrayList<>();
            for (int i = 0; i < tmp.size(); ++i) {
                instances.add(create(base, tmp.get(i), validate));
            }
            return instances;
        } else {
            throw new RuntimeException("Given listName \"" + listName + "\" is not part of input configuration");
        }
    }

    public static <Base> ArrayList<Base> createList(Class<Base> base, String listName, Hash input) {
        return createList(base, listName, input, true);
    }

    public static <B> VectorString getRegisteredClasses(Class<B> base) {
        Configurator configurator = Configurator.init(base);
        VectorString registeredClasses = new VectorString();
        for (Map.Entry<String, Map<String, Class<?>>> entry : (Set<Entry<String, Map<String, Class<?>>>>) configurator.m_registry.entrySet()) {
            registeredClasses.add(entry.getKey());
        }
        return registeredClasses;
    }
}
