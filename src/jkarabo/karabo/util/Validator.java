/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import karabo.util.time.Timestamp;
import karabo.util.types.FromLiteral;
import karabo.util.types.ToLiteral;
import karabo.util.types.Types;
import karabo.util.vectors.VectorHash;
import karabo.util.vectors.VectorString;
import java.util.LinkedHashSet;
import java.util.Map.Entry;
import java.util.Set;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public final class Validator {
    // Validation flags

    private boolean m_injectDefaults;
    private boolean m_allowUnrootedConfiguration;
    private boolean m_allowAdditionalKeys;
    private boolean m_allowMissingKeys;
    private boolean m_injectTimestamps;
    private Hash m_parametersInWarnOrAlarm = new Hash();
    private Timestamp m_timestamp;

    public enum ProblemType {

        WARN_LOW,
        WARN_HIGH,
        ALARM_LOW,
        ALARM_HIGH;
    }

    public class ValidationRules {

        public boolean injectDefaults;
        public boolean allowUnrootedConfiguration;
        public boolean allowAdditionalKeys;
        public boolean allowMissingKeys;
        public boolean injectTimestamps;
    }

    public Validator() {
        m_injectDefaults = true;
        m_allowUnrootedConfiguration = true;
        m_allowAdditionalKeys = false;
        m_allowMissingKeys = false;
        m_injectTimestamps = false;
    }

    public Validator(ValidationRules rules) {
        this.setValidationRules(rules);
    }

    public void setValidationRules(Validator.ValidationRules rules) {
        m_injectDefaults = rules.injectDefaults;
        m_allowAdditionalKeys = rules.allowAdditionalKeys;
        m_allowMissingKeys = rules.allowMissingKeys;
        m_allowUnrootedConfiguration = rules.allowUnrootedConfiguration;
        m_injectTimestamps = rules.injectTimestamps;
    }

    public Validator.ValidationRules getValidationRules() {
        Validator.ValidationRules rules = new ValidationRules();
        rules.injectDefaults = m_injectDefaults;
        rules.allowAdditionalKeys = m_allowAdditionalKeys;
        rules.allowMissingKeys = m_allowMissingKeys;
        rules.allowUnrootedConfiguration = m_allowUnrootedConfiguration;
        rules.injectTimestamps = m_injectTimestamps;
        return rules;
    }

    public boolean hasParametersInWarnOrAlarm() {
        return !m_parametersInWarnOrAlarm.empty();
    }

    public Hash getParametersInWarnOrAlarm() {
        return m_parametersInWarnOrAlarm;
    }

    public ValidatorResult validate(Schema schema, Hash unvalidatedInput, Hash validatedOutput) {
        return validate(schema, unvalidatedInput, validatedOutput, new Timestamp());
    }

    public ValidatorResult validate(Schema schema, Hash unvalidatedInput, Hash validatedOutput, Timestamp timestamp) {
        // Clear all previous warnings and alarms
        m_parametersInWarnOrAlarm.clear();

        // Prepare timestamp if needed
        if (m_injectTimestamps) {
            m_timestamp = timestamp;
        }

        // In case of failed validation, report why it failed
        String validationFailedReport = "";

        if (!m_allowUnrootedConfiguration) {
            if (unvalidatedInput.size() != 1) {
                return new ValidatorResult(false, "Expecting a rooted input, i.e. a Hash with exactly one key (describing the classId) at the top level");
            } else {
                Node node = unvalidatedInput.iterator().next().getValue();
                String classId = node.getKey();
                if (!schema.getRootName().equals(classId)) {
                    return new ValidatorResult(false, "Wrong schema for given input. Schema describes class \"" + schema.getRootName() + "\", whilst input wants to configure class \"" + classId + "\"");
                }

                if (node.getType() == Types.ReferenceType.HASH) {
                    Node tmp = validatedOutput.set(classId, new Hash());
                    this.r_validate(schema.getParameterHash(), (Hash) node.getValue(), (Hash) tmp.getValue(), validationFailedReport, classId);

                    if (validationFailedReport.isEmpty()) {
                        return new ValidatorResult(true, "");
                    } else {
                        return new ValidatorResult(false, validationFailedReport);
                    }
                } else {
                    return new ValidatorResult(false, "Root-node for given configuration is of wrong type. It must be HASH");
                }
            }
        } else {
            this.r_validate(schema.getParameterHash(), unvalidatedInput, validatedOutput, validationFailedReport, "");
            if (validationFailedReport.isEmpty()) {
                return new ValidatorResult(true, "");
            } else {
                return new ValidatorResult(false, validationFailedReport);
            }
        }
    }

    private void r_validate(Hash master, Hash user, Hash working, String report, String scope) {
        Set<String> keys = new LinkedHashSet<>(user.getKeys());
        
        for (Entry<String, Node> entry : master.entrySet()) {
            String key = entry.getKey();
            String currentScope;
            if (scope.isEmpty()) {
                currentScope = key;
            } else {
                currentScope = scope + "." + key;
            }
            Node node = entry.getValue();
            Schema.NodeType nodeType = Schema.NodeType.values()[node.getAttribute(Schema.KARABO_SCHEMA_NODE_TYPE)];
            boolean userHasNode = user.has(key);
            boolean hasDefault = node.hasAttribute(Schema.KARABO_SCHEMA_DEFAULT_VALUE);

            // Remove current node from all provided
            if (userHasNode) {
                keys.remove(key);
            }
            if (nodeType == Schema.NodeType.LEAF) {

                Schema.AssignmentType assignment = Schema.AssignmentType.values()[node.getAttribute(Schema.KARABO_SCHEMA_ASSIGNMENT)];

                if (!userHasNode) { // Node IS NOT provided
                    if (assignment == Schema.AssignmentType.MANDATORY_PARAM) {
                        if (!m_allowMissingKeys) {
                            report += "Missing mandatory parameter: \"" + currentScope + "\"\n";
                            return;
                        }
                    } else if (assignment == Schema.AssignmentType.OPTIONAL_PARAM && hasDefault && m_injectDefaults) {
                        Node n = working.set(key, node.getAttribute(Schema.KARABO_SCHEMA_DEFAULT_VALUE));
                        this.validateLeaf(node, n, report, currentScope);
                    }
                } else { // Node IS provided
                    Node n = working.setNode(user.getNode(key));
                    this.validateLeaf(node, n, report, currentScope);
                }
            } else if (nodeType == Schema.NodeType.NODE) {
                if (!userHasNode) {
                    if (m_injectDefaults) {
                        Node workNode = working.set(key, new Hash()); // Insert empty node
                        r_validate((Hash) node.getValue(), new Hash(), (Hash) workNode.getValue(), report, currentScope);
                    } else {
                        Hash workFake = new Hash();
                        r_validate((Hash) node.getValue(), new Hash(), workFake, report, currentScope);
                    }
                } else {

                    if (user.getType(key) != Types.ReferenceType.HASH) {
                        report += "Parameter \"" + currentScope + "\" has incorrect node type, expecting HASH not " + ToLiteral.to(user.getType(key)) + "\n";
                        return;
                    } else {
                        Node workNode = working.set(key, new Hash()); // Insert empty node
                        r_validate((Hash) node.getValue(), (Hash) user.get(key), (Hash) workNode.getValue(), report, currentScope);
                    }
                }
            } else if (nodeType == Schema.NodeType.CHOICE_OF_NODES) {

                Schema.AssignmentType assignment = Schema.AssignmentType.values()[node.getAttribute(Schema.KARABO_SCHEMA_ASSIGNMENT)];

                if (!userHasNode) {
                    if (assignment == Schema.AssignmentType.MANDATORY_PARAM) {
                        if (!m_allowMissingKeys) {
                            report += "Missing (choice-)parameter: \"" + currentScope + "\"\n";
                            return;
                        }
                    } else if (assignment == Schema.AssignmentType.OPTIONAL_PARAM && hasDefault && m_injectDefaults) {
                        String optionName = node.getAttribute(Schema.KARABO_SCHEMA_DEFAULT_VALUE);
                        Node workNode = working.set(key, new Hash(optionName, new Hash())); // Inject empty choice
                        r_validate((Hash) ((Hash) node.getValue()).get(optionName), new Hash(), (Hash) ((Hash) workNode.getValue()).get(optionName), report, currentScope + "." + optionName);
                    }
                } else { // User has set a node

                    Set<String> validOptions = ((Hash) master.get(key)).getKeys();

                    // If the option has all-default parameters the user lazily may have set the option as string instead of HASH
                    // We will allow for this and silently inject an empty Hash instead
                    if (user.getType(key) == Types.ReferenceType.STRING) {

                        //System.out.println("Silently converting from STRING");
                        String optionName = user.get(key);
                        if (validOptions.contains(optionName)) { // Is a valid option
                            Node workNode = working.set(key, new Hash(optionName, new Hash())); // Inject empty choice
                            r_validate((Hash) ((Hash) node.getValue()).get(optionName), new Hash(), (Hash) ((Hash) workNode.getValue()).get(optionName), report, currentScope + "." + optionName);
                        } else {
                            report += "Provided parameter: \"" + optionName + "\" is not a valid option for choice: \"" + key + "\". ";
                            report += "Valid options are: " + validOptions + "\n";
                            return;
                        }
                    } else if (user.getType(key) != Types.ReferenceType.HASH) {
                        report += "Parameter \"" + currentScope + "\" has incorrect type, expecting HASH not " + ToLiteral.to(user.getType(key)) + "\n";
                        return;
                    } else {
                        Hash choice = user.get(key);
                        if (choice.size() == 0) {
                            if (assignment == Schema.AssignmentType.MANDATORY_PARAM) {
                                if (!m_allowMissingKeys) {
                                    report += "Missing option for choice: \"" + currentScope + "\". ";
                                    report += "Valid options are: " + validOptions + "\n";
                                    return;
                                }
                            } else if (assignment == Schema.AssignmentType.OPTIONAL_PARAM && hasDefault && m_injectDefaults) {
                                String optionName = node.getAttribute(Schema.KARABO_SCHEMA_DEFAULT_VALUE);
                                Node workNode = working.set(key, new Hash(optionName, new Hash())); // Inject empty choice
                                r_validate((Hash) ((Hash) node.getValue()).get(optionName), new Hash(), (Hash) ((Hash) workNode.getValue()).get(optionName), report, currentScope + "." + optionName);
                            }
                        } else if (choice.size() == 1) { // That is what we expect it should be
                            Node usersOption = choice.iterator().next().getValue();
                            String optionName = usersOption.getKey();
                            if (validOptions.contains(optionName)) { // Is a valid option
                                Node workNode = working.set(key, new Hash(optionName, new Hash())); // Inject empty choice
                                r_validate((Hash) ((Hash) node.getValue()).get(optionName), (Hash) usersOption.getValue(), (Hash) ((Hash) workNode.getValue()).get(optionName), report, currentScope + "." + optionName);
                            } else {
                                report += "Provided parameter: \"" + optionName + "\" is not a valid option for choice: \"" + key + "\". ";
                                report += "Valid options are: " + validOptions + "\n";
                                return;
                            }
                        } else if (choice.size() > 1) {
                            Set<String> usersOptions = choice.getKeys();
                            report += "Choice element \"" + key + "\" expects exactly one option, however multiple options (" + usersOptions + ") were provided. ";
                            report += "Valid options are: " + validOptions + "\n";
                        }
                    }
                }
            } else if (nodeType == Schema.NodeType.LIST_OF_NODES) {

                Schema.AssignmentType assignment = Schema.AssignmentType.values()[node.getAttribute(Schema.KARABO_SCHEMA_ASSIGNMENT)];

                if (!userHasNode) { // Node IS NOT provided
                    if (assignment == Schema.AssignmentType.MANDATORY_PARAM) {
                        if (!m_allowMissingKeys) {
                            report += "Missing (list-)parameter: \"" + currentScope + "\"\n";
                            return;
                        }
                    } else if ((assignment == Schema.AssignmentType.OPTIONAL_PARAM) && hasDefault && m_injectDefaults) {
                        VectorString optionNames = node.getAttribute(Schema.KARABO_SCHEMA_DEFAULT_VALUE);
                        Node workNode = working.set(key, new VectorHash()); // TODO use bindReference here
                        VectorHash workNodes = workNode.getValue();


                        for (String optionName : optionNames) {
                            Hash tmp = new Hash();
                            r_validate((Hash) ((Hash) node.getValue()).get(optionName), new Hash(), tmp, report, currentScope + "." + optionName);
                            workNodes.add(new Hash(optionName, tmp));
                        }
                    }

                } else { // Node IS provided
                    Set<String> validOptions = ((Hash) master.get(key)).getKeys();
                    Node workNode = working.set(key, new VectorHash()); // TODO use bindReference here
                    VectorHash workNodes = workNode.getValue();

                    // If the options have all-default parameters the user lazily may have set the option as string instead of HASH
                    // We will allow for this and silently inject an empty Hashes instead
                    if (user.getType(key) == Types.ReferenceType.VECTOR_STRING) {
                        VectorString optionNames = user.get(key);
                        int optionNamesSize = optionNames.size();
                        if (node.hasAttribute(Schema.KARABO_SCHEMA_MIN) && (optionNamesSize < (int) node.getAttribute(Schema.KARABO_SCHEMA_MIN))) {
                            report += "Too less options given for (list-)parameter: \"" + key + "\". Expecting at least " + node.getAttribute(Schema.KARABO_SCHEMA_MIN) + "\n";
                            return;
                        }
                        if (node.hasAttribute(Schema.KARABO_SCHEMA_MAX) && (optionNamesSize > (int) node.getAttribute(Schema.KARABO_SCHEMA_MAX))) {
                            report += "Too many options given for (list-)parameter: \"" + key + "\". Expecting at most " + node.getAttribute(Schema.KARABO_SCHEMA_MAX);
                            return;
                        }


                        for (String optionName : optionNames) {
                            //System.out.println("Silently converting from STRING");
                            if (validOptions.contains(optionName)) { // Is a valid option
                                Hash tmp = new Hash();
                                r_validate((Hash) ((Hash) node.getValue()).get(optionName), new Hash(), tmp, report, currentScope + "." + optionName);
                                workNodes.add(new Hash(optionName, tmp));
                            } else {
                                report += "Provided parameter: \"" + optionName + "\" is not a valid option for list: \"" + key + "\". ";
                                report += "Valid options are: " + validOptions + "\n";
                                return;
                            }
                        }
                    } else if (user.getType(key) != Types.ReferenceType.VECTOR_HASH) {
                        report += "Parameter \"" + currentScope + "\" has incorrect type, expecting VECTOR_HASH not " + ToLiteral.to(user.getType(key)) + "\n";
                        return;
                    } else {

                        VectorHash userOptions = user.get(key);
                        int optionNamesSize = userOptions.size();
                        if (node.hasAttribute(Schema.KARABO_SCHEMA_MIN) && (optionNamesSize < (int) node.getAttribute(Schema.KARABO_SCHEMA_MIN))) {
                            report += "Too less options given for (list-)parameter: \"" + key + "\". Expecting at least " + node.getAttribute(Schema.KARABO_SCHEMA_MIN);
                            report += "Valid options are: " + validOptions + "\n";
                            return;
                        }
                        if (node.hasAttribute(Schema.KARABO_SCHEMA_MAX) && (optionNamesSize > (int) node.getAttribute(Schema.KARABO_SCHEMA_MAX))) {
                            report += "Too many options given for (list-)parameter: \"" + key + "\". Expecting at most " + node.getAttribute(Schema.KARABO_SCHEMA_MAX);
                            report += "Valid options are: " + validOptions + "\n";
                            return;
                        }

                        // That is what we expect it should be
                        for (int i = 0; i < userOptions.size(); ++i) {
                            Hash option = userOptions.get(i);
                            Node rootNode = option.iterator().next().getValue();
                            String optionName = rootNode.getKey();
                            if (validOptions.contains(optionName)) { // Is a valid option
                                Hash tmp = new Hash();
                                r_validate((Hash) ((Hash) node.getValue()).get(optionName), (Hash) rootNode.getValue(), tmp, report, currentScope + "." + optionName);
                                workNodes.add(new Hash(optionName, tmp));
                            } else {
                                report += "Provided parameter: \"" + optionName + "\" is not a valid option for list: \"" + key + "\". ";
                                report += "Valid options are: " + validOptions + "\n";
                                return;
                            }
                        }
                    }
                }
            }
        }

        if (!m_allowAdditionalKeys && !keys.isEmpty()) {


            for (String key : keys) {
                String currentScope;
                if (scope.isEmpty()) {
                    currentScope = key;
                } else {
                    currentScope = scope + "." + key;
                }
                report += "Encountered unexpected configuration parameter: \"" + currentScope + "\"\n";
            }
        }
    }

    public void validateLeaf(Node masterNode, Node workNode, String report, String scope) {

        if (m_injectTimestamps) {
            attachTimestampIfNotAlreadyThere(workNode);
        }

        Types.ReferenceType referenceType = FromLiteral.from((String) masterNode.getAttribute(Schema.KARABO_SCHEMA_VALUE_TYPE));
        Types.ReferenceType referenceCategory = Types.category(referenceType);
        Types.ReferenceType givenType = workNode.getType();

        // Check data types
        if (givenType != referenceType) {
            // Try casting this guy
            try {
                workNode.setType(referenceType);
            } catch (Exception e) {
                report += "Failed to cast the value of parameter \"" + scope + "\" from " + ToLiteral.to(givenType);
                report += " to " + ToLiteral.to(referenceType) + "\n";
                return;
            }
        }

        // Check ranges
        if (referenceCategory == Types.ReferenceType.SIMPLE) {
            if (masterNode.hasAttribute(Schema.KARABO_SCHEMA_OPTIONS)) {
                VectorString options = masterNode.getAttribute(Schema.KARABO_SCHEMA_OPTIONS);
                boolean found = false;
                for (String option : options) {
                    if (option == null ? workNode.getValueAsAny() == null : option.equals(workNode.getValueAsString())) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    report += "Value " + workNode.getValueAsString() + " for parameter \"" + scope + "\" is not one of the valid options: " + options + "\n";
                }
            }

            if (masterNode.hasAttribute(Schema.KARABO_SCHEMA_MIN_EXC)) {
                double minExc = masterNode.getAttribute(Schema.KARABO_SCHEMA_MIN_EXC);
                double value = workNode.getValue();
                if (value <= minExc) {
                    report += "Value " + value + " for parameter \"" + scope + "\" is out of lower bound " + minExc + "\n";
                }
            }

            if (masterNode.hasAttribute(Schema.KARABO_SCHEMA_MIN_INC)) {
                double minInc = (double) masterNode.getAttribute(Schema.KARABO_SCHEMA_MIN_INC);
                double value = workNode.getValue();
                if (value < minInc) {
                    report += "Value " + value + " for parameter \"" + scope + "\" is out of lower bound " + minInc + "\n";
                }
            }

            if (masterNode.hasAttribute(Schema.KARABO_SCHEMA_MAX_EXC)) {
                double maxExc = (double) masterNode.getAttribute(Schema.KARABO_SCHEMA_MAX_EXC);
                double value = workNode.getValue();
                if (value >= maxExc) {
                    report += "Value " + value + " for parameter \"" + scope + "\" is out of upper bound " + maxExc + "\n";
                }
            }

            if (masterNode.hasAttribute(Schema.KARABO_SCHEMA_MAX_INC)) {
                double maxInc = masterNode.getAttribute(Schema.KARABO_SCHEMA_MAX_INC);
                double value = workNode.getValue();
                if (value > maxInc) {
                    report += "Value " + value + " for parameter \"" + scope + "\" is out of upper bound " + maxInc + "\n";
                }
            }

            if (masterNode.hasAttribute(Schema.KARABO_SCHEMA_WARN_LOW)) {
                double threshold = masterNode.getAttribute(Schema.KARABO_SCHEMA_WARN_LOW);
                double value = workNode.getValue();
                if (value < threshold) {
                    String msg = "Value " + workNode.getValue() + " of parameter \"" + scope + "\" went below warn level of " + threshold;
                    m_parametersInWarnOrAlarm.set(scope, new Hash("type", "WARN_LOW", "message", msg), "\0");
                    attachTimestampIfNotAlreadyThere(workNode);
                }
            }

            if (masterNode.hasAttribute(Schema.KARABO_SCHEMA_WARN_HIGH)) {
                double threshold = masterNode.getAttribute(Schema.KARABO_SCHEMA_WARN_HIGH);
                double value = workNode.getValue();
                if (value > threshold) {
                    String msg = "Value " + workNode.getValue() + " of parameter \"" + scope + "\" went above warn level of " + threshold;
                    m_parametersInWarnOrAlarm.set(scope, new Hash("type", "WARN_HIGH", "message", msg), "\0");
                    attachTimestampIfNotAlreadyThere(workNode);
                }
            }

            if (masterNode.hasAttribute(Schema.KARABO_SCHEMA_ALARM_LOW)) {
                double threshold = masterNode.getAttribute(Schema.KARABO_SCHEMA_ALARM_LOW);
                double value = workNode.getValue();
                if (value < threshold) {
                    String msg = "Value " + workNode.getValue() + " of parameter \"" + scope + "\" went below alarm level of " + threshold;
                    m_parametersInWarnOrAlarm.set(scope, new Hash("type", "ALARM_LOW", "message", msg), "\0");
                    attachTimestampIfNotAlreadyThere(workNode);
                }
            }

            if (masterNode.hasAttribute(Schema.KARABO_SCHEMA_ALARM_HIGH)) {
                double threshold = masterNode.getAttribute(Schema.KARABO_SCHEMA_ALARM_HIGH);
                double value = workNode.getValue();
                if (value > threshold) {
                    String msg = "Value " + workNode.getValue() + " of parameter \"" + scope + "\" went above alarm level of " + threshold;
                    m_parametersInWarnOrAlarm.set(scope, new Hash("type", "ALARM_HIGH", "message", msg), "\0");
                    attachTimestampIfNotAlreadyThere(workNode);
                }
            }

            //if (masterNode.hasAttribute(""))
        } else if (referenceCategory == Types.ReferenceType.SEQUENCE) {
            int currentSize = ((VectorString) workNode.getValue()).size();

            // TODO Check whether we are really going to validate inner elements of a vector for max/min..., maybe not.

            if (masterNode.hasAttribute(Schema.KARABO_SCHEMA_MIN_SIZE)) {
                int minSize = masterNode.getAttribute(Schema.KARABO_SCHEMA_MIN_SIZE);
                if (currentSize < minSize) {
                    report += "Number of elements (" + currentSize + " for (vector-)parameter \"" + scope + "\" is smaller than lower bound (" + minSize + ")\n";
                }
            }

            if (masterNode.hasAttribute(Schema.KARABO_SCHEMA_MAX_SIZE)) {
                int maxSize = masterNode.getAttribute(Schema.KARABO_SCHEMA_MAX_SIZE);
                if (currentSize > maxSize) {
                    report += "Number of elements (" + currentSize + " for (vector-)parameter \"" + scope + "\" is greater than upper bound (" + maxSize + ")\n";
                }
            }
        }
    }

    public void attachTimestampIfNotAlreadyThere(Node node) {
        if (m_injectTimestamps) {
            Attributes attributes = node.getAttributes();
            if (!Timestamp.hashAttributesContainTimeInformation(attributes)) {
                new Timestamp().toHashAttributes(attributes);
            }
        }
    }
}
