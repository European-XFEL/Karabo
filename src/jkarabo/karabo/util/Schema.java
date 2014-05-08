/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import karabo.util.types.FromLiteral;
import karabo.util.vectors.VectorString;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import karabo.util.types.Types.ReferenceType;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class Schema {

    static final String KARABO_SCHEMA_NODE_TYPE = "nodeType";
    static final String KARABO_SCHEMA_LEAF_TYPE = "leafType";
    static final String KARABO_SCHEMA_VALUE_TYPE = "valueType";
    static final String KARABO_SCHEMA_CLASS_ID = "classId";
    static final String KARABO_SCHEMA_DISPLAYED_NAME = "displayedName";
    static final String KARABO_SCHEMA_DESCRIPTION = "description";
    static final String KARABO_SCHEMA_DEFAULT_VALUE = "defaultValue";
    static final String KARABO_SCHEMA_DISPLAY_TYPE = "displayType";
    static final String KARABO_SCHEMA_ACCESS_MODE = "accessMode";
    static final String KARABO_SCHEMA_ALIAS = "alias";
    static final String KARABO_SCHEMA_ALLOWED_STATES = "allowedStates";
    static final String KARABO_SCHEMA_ASSIGNMENT = "assignment";
    static final String KARABO_SCHEMA_TAGS = "tags";
    static final String KARABO_SCHEMA_OPTIONS = "options";
    static final String KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL = "requiredAccessLevel";
    static final String KARABO_SCHEMA_UNIT_ENUM = "unitEnum";
    static final String KARABO_SCHEMA_UNIT_NAME = "unitName";
    static final String KARABO_SCHEMA_UNIT_SYMBOL = "unitSymbol";
    static final String KARABO_SCHEMA_METRIC_PREFIX_ENUM = "metricPrefixEnum";
    static final String KARABO_SCHEMA_METRIC_PREFIX_NAME = "metricPrefixName";
    static final String KARABO_SCHEMA_METRIC_PREFIX_SYMBOL = "metricPrefixSymbol";
    static final String KARABO_SCHEMA_MIN_INC = "minInc";
    static final String KARABO_SCHEMA_MAX_INC = "maxInc";
    static final String KARABO_SCHEMA_MIN_EXC = "minExc";
    static final String KARABO_SCHEMA_MAX_EXC = "maxExc";
    static final String KARABO_SCHEMA_MIN_SIZE = "minSize";
    static final String KARABO_SCHEMA_MAX_SIZE = "maxSize";
    static final String KARABO_SCHEMA_WARN_LOW = "warnLow";
    static final String KARABO_SCHEMA_WARN_HIGH = "warnHigh";
    static final String KARABO_SCHEMA_ALARM_LOW = "alarmLow";
    static final String KARABO_SCHEMA_ALARM_HIGH = "alarmHigh";
    static final String KARABO_SCHEMA_ARCHIVE_POLICY = "archivePolicy";
    static final String KARABO_SCHEMA_MIN = "min";
    static final String KARABO_SCHEMA_MAX = "max";
    static final String KARABO_SCHEMA_OVERWRITE = "overwrite";
    // Container
    private Hash m_hash = new Hash();
    // Filter
    private AccessType m_currentAccessMode;
    private String m_currentState;
    private int m_currentAccessLevel;
    // Root name
    private String m_rootName;
    // Indices
    Map<String, String> m_aliasToKey = new HashMap<>();

    public static class AssemblyRules {

        AccessType m_accessMode = AccessType.INIT;
        String m_state;
        int m_accessLevel;

        public AssemblyRules() {
            m_accessMode = AccessType.INIT_READ_WRITE;
            m_state = "";
            m_accessLevel = -1;
        }

        public AssemblyRules(AccessType atype) {
            m_accessMode = atype;
            m_state = "";
            m_accessLevel = -1;
        }

        public AssemblyRules(AccessType atype, String state) {
            m_accessMode = atype;
            m_state = state;
            m_accessLevel = -1;
        }

        public AssemblyRules(AccessType atype, String state, int accessLevel) {
            m_accessMode = atype;
            m_state = state;
            m_accessLevel = accessLevel;
        }
    }

    public enum NodeType {

        LEAF,
        NODE,
        CHOICE_OF_NODES,
        LIST_OF_NODES;
    }

    public enum LeafType {

        PROPERTY,
        COMMAND;
    }

    public enum AssignmentType {

        OPTIONAL_PARAM,
        MANDATORY_PARAM,
        INTERNAL_PARAM;
    }

    public enum ArchivePolicy {

        EVERY_EVENT,
        EVERY_100MS,
        EVERY_1S,
        EVERY_5S,
        EVERY_10S,
        EVERY_1MIN,
        EVERY_10MIN,
        NO_ARCHIVING;
    }

    public enum AccessLevel {

        OBSERVER,
        USER,
        OPERATOR,
        EXPERT,
        ADMIN;
    }

    public Schema() {
        AssemblyRules rules = new AssemblyRules();
        m_currentAccessMode = rules.m_accessMode;
        m_currentState = rules.m_state;
        m_currentAccessLevel = rules.m_accessLevel;
        m_rootName = "";
    }

    public Schema(String classId) {
        AssemblyRules rules = new AssemblyRules();
        m_currentAccessMode = rules.m_accessMode;
        m_currentState = rules.m_state;
        m_currentAccessLevel = rules.m_accessLevel;
        m_rootName = classId;
    }

    public Schema(String classId, Schema.AssemblyRules rules) {
        m_currentAccessMode = rules.m_accessMode;
        m_currentState = rules.m_state;
        m_currentAccessLevel = rules.m_accessLevel;
        m_rootName = classId;
    }

    public void setAssemblyRules(AssemblyRules rules) {
        m_currentAccessMode = rules.m_accessMode;
        m_currentState = rules.m_state;
        m_currentAccessLevel = rules.m_accessLevel;
    }

    public AssemblyRules getAssemblyRules() {
        AssemblyRules rules = new AssemblyRules();
        rules.m_accessMode = m_currentAccessMode;
        rules.m_accessLevel = m_currentAccessLevel;
        rules.m_state = m_currentState;
        return rules;
    }

    public String getRootName() {
        return m_rootName;
    }

    public Hash getParameterHash() {
        return m_hash;
    }

    public Hash getParameterHash1() {
        return new Hash(m_hash);
    }

    public Set<String> getKeys() {
        return getKeys("");
    }

    public Set<String> getKeys(String path) {
        if (path.isEmpty()) {
            return m_hash.getKeys();
        } else {
            return ((Hash) m_hash.get(path)).getKeys();
        }
    }

    public VectorString getPaths() {
        return m_hash.getPaths();
    }

    //**********************************************
    //          General functions on Schema        *
    //**********************************************
    public boolean has(String path) {
        return m_hash.has(path);
    }

    void addElement(Node node) {

        if (node.hasAttribute(KARABO_SCHEMA_OVERWRITE)) {
            this.overwriteAttributes(node);
            return;
        }

        // Ensure completeness of node parameter description
        ensureParameterDescriptionIsComplete(node); // Will throw in case of error

        // Check whether node is allowed to be added
        boolean accessModeOk = isAllowedInCurrentAccessMode(node);
        boolean accessRoleOk = isAllowedInCurrentAccessLevel(node);
        boolean stateOk = isAllowedInCurrentState(node);

        if (!(accessModeOk && accessRoleOk && stateOk)) {
            return;
        }

        this.getParameterHash().setNode(node);
    }

    public void merge(Schema schema) {
        Hash hash = schema.getParameterHash();
        for (Node node : hash.values()) {
            this.addElement(node);
        }
    }

    public boolean empty() {
        return m_hash.empty();
    }

    //**********************************************
    //              Node property                  *
    //**********************************************
    public boolean isCommand(String path) {
        if (this.isNode(path)) {
            if (this.hasDisplayType(path)) {
                // TODO Bad hack, clean this up later !!!!
                if ("Slot".equals((String) m_hash.getAttribute(path, KARABO_SCHEMA_DISPLAY_TYPE))) {
                    return true;
                }
            }
        }
        return false;
    }

    public boolean isProperty(String path) {
        return this.isLeaf(path) && LeafType.values()[m_hash.getAttribute(path, KARABO_SCHEMA_LEAF_TYPE)] == LeafType.PROPERTY;
    }

    public boolean isLeaf(String path) {
        return NodeType.values()[m_hash.getAttribute(path, KARABO_SCHEMA_NODE_TYPE)] == NodeType.LEAF;
    }

    public boolean isNode(String path) {
        return NodeType.values()[m_hash.getAttribute(path, KARABO_SCHEMA_NODE_TYPE)] == NodeType.NODE;
    }

    public boolean isChoiceOfNodes(String path) {
        return NodeType.values()[m_hash.getAttribute(path, KARABO_SCHEMA_NODE_TYPE)] == NodeType.CHOICE_OF_NODES;
    }

    public boolean isListOfNodes(String path) {
        return NodeType.values()[m_hash.getAttribute(path, KARABO_SCHEMA_NODE_TYPE)] == NodeType.LIST_OF_NODES;
    }

    public NodeType getNodeType(String path) {
        return NodeType.values()[m_hash.getAttribute(path, KARABO_SCHEMA_NODE_TYPE)];
    }

    //**********************************************
    //                Value Type                  *
    //**********************************************
    public ReferenceType getValueType(String path) {
        return FromLiteral.from((String) m_hash.getAttribute(path, KARABO_SCHEMA_VALUE_TYPE));
    }

    //**********************************************
    //                Access Mode                  *
    //**********************************************
    public void setAccessMode(String path, AccessType value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_ACCESS_MODE, value.ordinal());
    }

    public boolean hasAccessMode(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_ACCESS_MODE);
    }

    public boolean isAccessInitOnly(String path) {
        return AccessType.values()[m_hash.getAttribute(path, KARABO_SCHEMA_ACCESS_MODE)] == AccessType.INIT;
    }

    public boolean isAccessReadOnly(String path) {
        return AccessType.values()[m_hash.getAttribute(path, KARABO_SCHEMA_ACCESS_MODE)] == AccessType.READ;
    }

    public boolean isAccessReconfigurable(String path) {
        return AccessType.values()[m_hash.getAttribute(path, KARABO_SCHEMA_ACCESS_MODE)] == AccessType.WRITE;
    }

    public AccessType getAccessMode(String path) {
        return AccessType.values()[m_hash.getAttribute(path, KARABO_SCHEMA_ACCESS_MODE)];
    }

    //**********************************************
    //             DisplayedName                   *
    //**********************************************
    public void setDisplayedName(String path, String value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_DISPLAYED_NAME, value);
    }

    public boolean hasDisplayedName(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_DISPLAYED_NAME);
    }

    public String getDisplayedName(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_DISPLAYED_NAME);
    }

    //**********************************************
    //               Description                   *
    //**********************************************
    public void setDescription(String path, String value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_DESCRIPTION, value);
    }

    public boolean hasDescription(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_DESCRIPTION);
    }

    public String getDescription(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_DESCRIPTION);
    }

    //**********************************************
    //                   Tags                      *
    //**********************************************
    public void setTags(String path, String value, String sep) {
        VectorString vs = new VectorString();
        String[] sa = value.split("[" + sep + "]");
        for (String s : sa) {
            if (!"".equals(s)) {
                vs.add(s);
            }
        }
        m_hash.setAttribute(path, KARABO_SCHEMA_TAGS, vs);
    }

    public void setTags(String path, String value) {
        setTags(path, value, " ,;");
    }

    public boolean hasTags(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_TAGS);
    }

    public VectorString getTags(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_TAGS);
    }

    //**********************************************
    //               DisplayType                   *
    //**********************************************
    public void setDisplayType(String path, String value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_DISPLAY_TYPE, value);
    }

    public boolean hasDisplayType(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_DISPLAY_TYPE);
    }

    public String getDisplayType(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_DISPLAY_TYPE);
    }

    //**********************************************
    //               Assignment                    *
    //**********************************************
    public void setAssignment(String path, AssignmentType value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_ASSIGNMENT, value.ordinal());
    }

    public boolean hasAssignment(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_ASSIGNMENT);
    }

    public boolean isAssignmentMandatory(String path) {
        return AssignmentType.values()[m_hash.getAttribute(path, KARABO_SCHEMA_ASSIGNMENT)] == AssignmentType.MANDATORY_PARAM;
    }

    public boolean isAssignmentOptional(String path) {
        return AssignmentType.values()[m_hash.getAttribute(path, KARABO_SCHEMA_ASSIGNMENT)] == AssignmentType.OPTIONAL_PARAM;
    }

    public boolean isAssignmentInternal(String path) {
        return AssignmentType.values()[m_hash.getAttribute(path, KARABO_SCHEMA_ASSIGNMENT)] == AssignmentType.INTERNAL_PARAM;
    }

    public AssignmentType getAssignment(String path) {
        return AssignmentType.values()[m_hash.getAttribute(path, KARABO_SCHEMA_ASSIGNMENT)];
    }

    //**********************************************
    //                  Options                    *
    //**********************************************
    public void setOptions(String path, String value, String sep) {
        VectorString vs = new VectorString();
        String[] sa = value.split("[" + sep + "]");
        for (String s : sa) {
            if (!"".equals(s)) {
                vs.add(s);
            }
        }
        m_hash.setAttribute(path, KARABO_SCHEMA_OPTIONS, vs);
    }

    public boolean hasOptions(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_OPTIONS);
    }

    public VectorString getOptions(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_OPTIONS);
    }

    //**********************************************
    //                AllowedStates                *
    //**********************************************
    public void setAllowedStates(String path, String value, String sep) {
        VectorString vs = new VectorString();
        String[] sa = value.split("[" + sep + "]");
        for (String s : sa) {
            if (!"".equals(s)) {
                vs.add(s);
            }
        }
        m_hash.setAttribute(path, KARABO_SCHEMA_ALLOWED_STATES, vs);
    }

    public boolean hasAllowedStates(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_ALLOWED_STATES);
    }

    public VectorString getAllowedStates(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_ALLOWED_STATES);
    }

    //**********************************************
    //                  RequiredAccessLevel                *
    //**********************************************
    public void setRequiredAccessLevel(String path, AccessLevel value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, value.ordinal());
    }

    public AccessLevel getRequiredAccessLevel(String path) {
        String[] tokens = path.split("[.]");

        String partialPath = "";
        AccessLevel highestLevel = AccessLevel.OBSERVER;

        for (String token : tokens) {
            if (partialPath.isEmpty()) {
                partialPath = token;
            } else {
                partialPath += "." + token;
            }
            if (m_hash.hasAttribute(partialPath, KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL)) {
                AccessLevel currentLevel = AccessLevel.values()[m_hash.getAttribute(partialPath, KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL)];
                if (currentLevel.ordinal() > highestLevel.ordinal()) {
                    highestLevel = currentLevel;
                }
            }
        }
        return highestLevel;
    }

    //**********************************************
    //                DefaultValue                 *
    //**********************************************
    public <ValueType extends Object> void setDefaultValue(String path, ValueType value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_DEFAULT_VALUE, value);
    }

    public boolean hasDefaultValue(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_DEFAULT_VALUE);
    }

    public <ValueType extends Object> ValueType getDefaultValue(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_DEFAULT_VALUE);
    }

    //**********************************************
    //                  Alias                      *
    //**********************************************
    public boolean keyHasAlias(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_ALIAS);
    }

    public <AliasType extends Object> boolean aliasHasKey(AliasType alias) {
        if (m_aliasToKey.containsKey(alias.toString())) {
            return true;
        }
        return false;
    }

    public <AliasType extends Object> AliasType getAliasFromKey(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_ALIAS);
    }

    public <AliasType extends Object> String getKeyFromAlias(AliasType alias) {
        return m_aliasToKey.get(alias.toString());
    }

    public String getAliasAsString(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_ALIAS).toString();
    }

    public <AliasType extends Object> void setAlias(String path, AliasType value) {
        m_aliasToKey.put(value.toString(), path);
        m_hash.setAttribute(path, KARABO_SCHEMA_ALIAS, value);
    }

    //**********************************************
    //                  Unit                       *
    //**********************************************
    public void setUnit(String path, Units.Unit value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_UNIT_ENUM, value.ordinal());
        Units.Entry<String, String> names = Units.getUnit(value);
        m_hash.setAttribute(path, KARABO_SCHEMA_UNIT_NAME, names.name);
        m_hash.setAttribute(path, KARABO_SCHEMA_UNIT_SYMBOL, names.symbol);
    }

    public boolean hasUnit(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_UNIT_ENUM);
    }

    public Units.Unit getUnit(String path) {
        return Units.Unit.values()[m_hash.getAttribute(path, KARABO_SCHEMA_UNIT_ENUM)];
    }

    public String getUnitName(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_UNIT_NAME);
    }

    public String getUnitSymbol(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_UNIT_SYMBOL);
    }

    //**********************************************
    //                  UnitMetricPrefix           *
    //**********************************************
    public void setMetricPrefix(String path, Units.MetricPrefix value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_METRIC_PREFIX_ENUM, value.ordinal());
        Units.Entry<String, String> names = Units.getMetricPrefix(value);
        m_hash.setAttribute(path, KARABO_SCHEMA_METRIC_PREFIX_NAME, names.name);
        m_hash.setAttribute(path, KARABO_SCHEMA_METRIC_PREFIX_SYMBOL, names.symbol);
    }

    public boolean hasMetricPrefix(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_METRIC_PREFIX_ENUM);
    }

    public Units.MetricPrefix getMetricPrefix(String path) {
        return Units.MetricPrefix.values()[m_hash.getAttribute(path, KARABO_SCHEMA_METRIC_PREFIX_ENUM)];
    }

    public String getMetricPrefixName(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_METRIC_PREFIX_NAME);
    }

    public String getMetricPrefixSymbol(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_METRIC_PREFIX_SYMBOL);
    }

    //******************************************************************
    //    Specific functions for LEAF node (which is not a vector) :   *
    //    Minimum Inclusive value                                      *
    //******************************************************************
    public <ValueType extends Object> void setMinInc(String path, ValueType value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_MIN_INC, value);
    }

    public <ValueType extends Object> ValueType getMinInc(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_MIN_INC);
    }

    public boolean hasMinInc(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_MIN_INC);
    }

    //******************************************************************
    //    Maximum Inclusive value                                      *
    //******************************************************************
    public <ValueType extends Object> void setMaxInc(String path, ValueType value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_MAX_INC, value);
    }

    public <ValueType extends Object> ValueType getMaxInc(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_MAX_INC);
    }

    public boolean hasMaxInc(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_MAX_INC);
    }

    //******************************************************************
    //    Minimum Exclusive value                                      *
    //******************************************************************
    public <ValueType extends Object> void setMinExc(String path, ValueType value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_MIN_EXC, value);
    }

    public <ValueType extends Object> ValueType getMinExc(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_MIN_EXC);
    }

    public boolean hasMinExc(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_MIN_EXC);
    }

    //******************************************************************
    //    Maximum Exclusive value                                      *
    //******************************************************************
    public <ValueType extends Object> void setMaxExc(String path, ValueType value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_MAX_EXC, value);
    }

    public <ValueType extends Object> ValueType getMaxExc(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_MAX_EXC);
    }

    public boolean hasMaxExc(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_MAX_EXC);
    }

    //******************************************************
    //  Specific functions for LEAF node (which is vector):*
    //  Minimum Size of the vector                         *
    //******************************************************
    public void setMinSize(String path, int value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_MIN_SIZE, value);
    }

    public boolean hasMinSize(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_MIN_SIZE);
    }

    public int getMinSize(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_MIN_SIZE);
    }

    //******************************************************
    //  Specific functions for LEAF node (which is vector):*
    //  Maximum Size of the vector                         *  
    //******************************************************
    public void setMaxSize(String path, int value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_MAX_SIZE, value);
    }

    public boolean hasMaxSize(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_MAX_SIZE);
    }

    public int getMaxSize(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_MAX_SIZE);
    }

    //******************************************************
    //                   WarnLow                          *  
    //******************************************************
    public <ValueType extends Object> void setWarnLow(String path, ValueType value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_WARN_LOW, value);
    }

    public <ValueType extends Object> ValueType getWarnLow(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_WARN_LOW);
    }

    public boolean hasWarnLow(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_WARN_LOW);
    }

    //******************************************************
    //                   WarnHigh                         *  
    //******************************************************
    public <ValueType extends Object> void setWarnHigh(String path, ValueType value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_WARN_HIGH, value);
    }

    public <ValueType extends Object> ValueType getWarnHigh(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_WARN_HIGH);
    }

    public boolean hasWarnHigh(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_WARN_HIGH);
    }

    //******************************************************
    //                   AlarmLow                          *  
    //******************************************************
    public <ValueType extends Object> void setAlarmLow(String path, ValueType value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_ALARM_LOW, value);
    }

    public <ValueType extends Object> ValueType getAlarmLow(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_ALARM_LOW);
    }

    public boolean hasAlarmLow(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_ALARM_LOW);
    }

    //******************************************************
    //                   AlarmHigh                          *  
    //******************************************************
    public <ValueType extends Object> void setAlarmHigh(String path, ValueType value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_ALARM_HIGH, value);
    }

    public <ValueType extends Object> ValueType getAlarmHigh(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_ALARM_HIGH);
    }

    public boolean hasAlarmHigh(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_ALARM_HIGH);
    }

    //**********************************************
    //               archivePolicy                 *
    //**********************************************
    public void setArchivePolicy(String path, ArchivePolicy value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_ARCHIVE_POLICY, value);
    }

    public boolean hasArchivePolicy(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_ARCHIVE_POLICY);
    }

    public ArchivePolicy getArchivePolicy(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_ARCHIVE_POLICY);
    }

    //******************************************************
    //      min/max for number of nodes in ListElement     *                       
    //******************************************************
    public void setMin(String path, int value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_MIN, value);
    }

    public boolean hasMin(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_MIN);
    }

    public int getMin(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_MIN);
    }

    public void setMax(String path, int value) {
        m_hash.setAttribute(path, KARABO_SCHEMA_MAX, value);
    }

    public boolean hasMax(String path) {
        return m_hash.hasAttribute(path, KARABO_SCHEMA_MAX);
    }

    public int getMax(String path) {
        return m_hash.getAttribute(path, KARABO_SCHEMA_MAX);
    }

    /**
     * Help function to show all parameters
     */
    //public void help(String classId, std::ostream& os = std::cout);
    void setParameterHash(Hash parameterDescription) {
        m_hash = parameterDescription;
    }

    void setRootName(String rootName) {
        m_rootName = rootName;
    }

    void updateAliasMap() {
        r_updateAliasMap(getKeys(), "");
    }

    private void r_updateAliasMap(Set<String> keys, String oldPath) {
        for (String key : keys) {
            String newPath = key;
            if (!oldPath.isEmpty()) {
                newPath = oldPath + "." + key;
            }
            if (keyHasAlias(newPath)) {
                m_aliasToKey.put(getAliasAsString(newPath), newPath);
            }
            if (getNodeType(newPath) == Schema.NodeType.NODE) {
                r_updateAliasMap(getKeys(newPath), newPath);
            } else if (getNodeType(newPath) == Schema.NodeType.CHOICE_OF_NODES) {
                r_updateAliasMap(getKeys(newPath), newPath);
            } else if (getNodeType(newPath) == Schema.NodeType.LIST_OF_NODES) {
                r_updateAliasMap(getKeys(newPath), newPath);
            }
        }
    }

    private void overwriteAttributes(Node node) {
        if (m_hash.has(node.getKey())) {
            Node thisNodeOpt = m_hash.get(node.getKey());
            Attributes attrs = node.getAttributes();
            if (attrs.size() > 0) {
                for (Map.Entry<String, Object> e : attrs.entrySet()) {
                    String attributeKey = e.getKey();
                    if (thisNodeOpt.hasAttribute(attributeKey)) {
                        thisNodeOpt.setAttribute(attributeKey, e.getValue());
                    }
                }
            }
        }
    }

    private void ensureParameterDescriptionIsComplete(Node node) {
        String error = "";

        if (node.hasAttribute(KARABO_SCHEMA_NODE_TYPE)) {
            NodeType type = NodeType.values()[node.getAttribute(KARABO_SCHEMA_NODE_TYPE)];
            if (type == NodeType.LEAF || type == NodeType.CHOICE_OF_NODES || type == NodeType.LIST_OF_NODES) {
                if (!node.hasAttribute(KARABO_SCHEMA_ASSIGNMENT)) {
                    error = "Missing assignment, i.e. assignmentMandatory() / assignmentOptional(). ";
                }
            }
        } else {
            error = "Missing nodeType attribute. ";
        }
        if (!node.hasAttribute(KARABO_SCHEMA_ACCESS_MODE)) {
            error = "Missing accessMode attribute. ";
        }

        if (!error.isEmpty()) {
            throw new RuntimeException("Bad description for parameter \"" + node.getKey() + "\": " + error);
        }
    }

    private boolean isAllowedInCurrentAccessMode(Node node) {
        return (m_currentAccessMode.get() & (AccessType.values()[node.getAttribute(KARABO_SCHEMA_ACCESS_MODE)]).get()) != 0;
    }

    private boolean isAllowedInCurrentAccessLevel(Node node) {
        if (node.hasAttribute(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL) && (m_currentAccessLevel != -1)) {
            return m_currentAccessLevel >= (int) node.getAttribute(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL);
        } else {
            return true;
        }
    }

    private boolean isAllowedInCurrentState(Node node) {
        if (node.hasAttribute(KARABO_SCHEMA_ALLOWED_STATES) && !m_currentState.isEmpty()) {
            VectorString allowedStates = node.getAttribute(KARABO_SCHEMA_ALLOWED_STATES);
            for (String state : allowedStates) {
                if (state == null ? m_currentState == null : state.equals(m_currentState)) {
                    return true;
                }
            }
            return false;
        } else { // If no states are assigned, access/visibility is always possible
            return true;
        }
    }

    @Override
    public String toString() {
        return "Schema for: " + this.getRootName() + "\n" + m_hash.toString();
    }
}
