/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import java.util.Arrays;
import java.util.Collection;
import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Objects;
import java.util.Set;
import java.util.StringTokenizer;
import karabo.util.types.ComplexDouble;
import karabo.util.types.ToLiteral;
import karabo.util.types.Types;
import karabo.util.vectors.VectorBoolean;
import karabo.util.vectors.VectorByte;
import karabo.util.vectors.VectorCharacter;
import karabo.util.vectors.VectorDouble;
import karabo.util.vectors.VectorFloat;
import karabo.util.vectors.VectorHash;
import karabo.util.vectors.VectorInteger;
import karabo.util.vectors.VectorLong;
import karabo.util.vectors.VectorShort;
import karabo.util.vectors.VectorString;

/**
 * Hash is the basic container widely used in Karabo Framework.<br>
 * <br>
 * Hash container:<br>
 * <ul>
 * <li>The Hash is a heterogeneous generic key/value container that associates a string key to a value of any type.</li>
 * <li>The Hash is a core data structure in Karabo software framework, and is widly used in the karabo system.</li>
 * <li>For instance, exchanging data and configurations between two or more entities (devices, GUI), database interface
 * (store and retrieval ), meta-data handling, etc.</li>
 * <li>The Hash class is much like a XML-DOM container with the difference of allowing only unique keys on a given
 * tree-level.</li>
 * <li>Like and XML DOM object, the Hash provides a multi-level (recursive) key-value associative container, where keys
 * are strings and values can be of any Java type.</li>
 * </ul>
 * <br>
 * Concept:<br>
 * <ul>
 * <li>Provide recursive key-value associative container (keys are strings and unique, values can be of any type)</li>
 * <li>Preserve insertion order, while optimized for random key-based lookup. Different iterators are available for each
 * use case.</li>
 * <li>Like in XML, each hash key can have a list of (key-value) attributes (attribute keys are strings and unique,
 * attribute values can be of any type).</li>
 * <li>Seamless serialization to/from Binary -- currently implemented!</li>
 * <li>Usage: configuration, device-state cache, database interface (result-set), message protocol, meta-data handling,
 * etc.</li>
 * <li>Generic set, get for retrieving values from keys. Assumes recursion on "." characters in key by default.
 * Separator can be specified per function call.</li>
 * <li>Exposed iterators will a sequential iterator (insertion order) and a alpha-numeric order iterator.</li>
 * <li>Each iterator provides access to its key, value, and attributes in form of a Hash::Node and can thus be used for
 * recursive traversal.</li>
 * <li>Insertion of a non-existing key leads to new entry whilst insertion of an existing key will only update (merge)
 * the corresponding value/attributes.</li>
 * <li>Additional functionality include: list of paths, clear/erase, find, merge, comparison, etc.</li>
 * </ul>
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class Hash extends Object implements Iterable {

    private OrderedMap m_container = new OrderedMap();

    /**
     * Create empty Hash object.
     */
    public Hash() {
    }

    /**
     * Create Hash with one node.
     *
     * @param key1 path for node1.
     * @param val1 value for node1.
     */
    public Hash(String key1, Object val1) {
        this.set(key1, val1);
    }

    /**
     * Create Hash with two nodes.
     *
     * @param key1 path for node1.
     * @param val1 value for node1.
     * @param key2 path for node2.
     * @param val2 value for node2.
     */
    public Hash(String key1, Object val1, String key2, Object val2) {
        this.set(key1, val1);
        this.set(key2, val2);
    }

    /**
     * Create Hash with three nodes.
     *
     * @param key1 path for node1.
     * @param val1 value for node1.
     * @param key2 path for node2.
     * @param val2 value for node2.
     * @param key3 path for node3.
     * @param val3 value for node3.
     */
    public Hash(String key1, Object val1, String key2, Object val2,
            String key3, Object val3) {
        this.set(key1, val1);
        this.set(key2, val2);
        this.set(key3, val3);
    }

    /**
     * Create Hash with four nodes.
     *
     * @param key1 path for node1.
     * @param val1 value for node1.
     * @param key2 path for node2.
     * @param val2 value for node2.
     * @param key3 path for node3.
     * @param val3 value for node3.
     * @param key4 path for node4.
     * @param val4 value for node4.
     */
    public Hash(String key1, Object val1, String key2, Object val2,
            String key3, Object val3, String key4, Object val4) {
        this.set(key1, val1);
        this.set(key2, val2);
        this.set(key3, val3);
        this.set(key4, val4);
    }

    /**
     * Create Hash with five nodes.
     *
     * @param key1 path for node1.
     * @param val1 value for node1.
     * @param key2 path for node2.
     * @param val2 value for node2.
     * @param key3 path for node3.
     * @param val3 value for node3.
     * @param key4 path for node4.
     * @param val4 value for node4.
     * @param key5 path for node5.
     * @param val5 value for node5.
     */
    public Hash(String key1, Object val1, String key2, Object val2,
            String key3, Object val3, String key4, Object val4,
            String key5, Object val5) {
        this.set(key1, val1);
        this.set(key2, val2);
        this.set(key3, val3);
        this.set(key4, val4);
        this.set(key5, val5);
    }

    /**
     * Create Hash with six nodes.
     *
     * @param key1 path for node1.
     * @param val1 value for node1.
     * @param key2 path for node2.
     * @param val2 value for node2.
     * @param key3 path for node3.
     * @param val3 value for node3.
     * @param key4 path for node4.
     * @param val4 value for node4.
     * @param key5 path for node5.
     * @param val5 value for node5.
     * @param key6 path for node6.
     * @param val6 value for node6.
     */
    public Hash(String key1, Object val1, String key2, Object val2,
            String key3, Object val3, String key4, Object val4,
            String key5, Object val5, String key6, Object val6) {
        this.set(key1, val1);
        this.set(key2, val2);
        this.set(key3, val3);
        this.set(key4, val4);
        this.set(key5, val5);
        this.set(key6, val6);
    }

    /**
     * Copy constructor.
     *
     * @param other Hash object used for initialization of Hash being constructed
     */
    public Hash(Hash other) {
        m_container = new OrderedMap(other.getContainer());
    }

    /**
     * Enumeration used for defining different merging strategies.
     */
    public enum MergePolicy {

        /**
         * Attributes from source Hash object are merging with attributes of target Hash object.
         */
        MERGE_ATTRIBUTES,
        /**
         * Attributes of source Hash object replace attributes of target Hash object.
         */
        REPLACE_ATTRIBUTES;
    }

    private OrderedMap getContainer() {
        return m_container;
    }

    /**
     * Check Hash container for emptiness.
     *
     * @return true if container is empty.
     */
    public boolean empty() {
        return m_container.empty();
    }

    /**
     * Get value of type T by providing a <b>path</b> and a <b>separator</b>.
     *
     * @param <T> anything that inherits from Object
     * @param path is a string key optionally representing a hierarchy.
     * @param separator is a string denoting a character used for building a hierarchy
     * @return The result should be cast to type T
     */
    public <T extends Object> T get(String path, String separator) {
        try {
            Hash hash = getLastHash(path, separator);
            String key = getLastKey(path, separator);
            int index = tryGetIndex(key);
            if (index == -1) {
                return hash.getContainer().getNode(key).getValue();
            }
            key = key.substring(0, key.indexOf("["));
            VectorHash vh = (VectorHash) hash.getContainer().get(key);
            Hash result = (Hash) vh.get(index);
            return (T) result;
        } catch (Exception e) {
            throw new RuntimeException("Array syntax on a leaf is not possible: " + e);
        }
    }

    /**
     * Get value of type T by providing a <b>path</b> with default separator ".".
     *
     * @param <T> anything that inherits from Object
     * @param path is a string key optionally representing a hierarchy.
     * @return returned value should be casted to type T
     */
    public <T extends Object> T get(String path) {
        return (T) this.<T>get(path, ".");
    }

    /**
     * Get value of type T by providing a <b>path</b> with default separator ".". This function used in Python to enable
     * index form of extracting value from Hash.
     *
     * @param <T> anything that inherits from Object
     * @param path is a string key optionally representing a hierarchy by means of ".".
     * @return returned value should be casted to type T
     */
    public <T extends Object> T __getitem__(String path) {
        return (T) this.<T>get(path);
    }

    /**
     * Check if path is part of given Hash object
     *
     * @param path is a string key
     * @param separator character used as separator
     * @return true if path belong to Hash
     */
    public boolean has(String path, String separator) {
        try {
            Hash hash = getLastHash(path, separator);
            String key = getLastKey(path, separator);
            int index = tryGetIndex(key);
            if (index == -1) {
                return hash.getContainer().has(key);
            } else {
                key = key.substring(0, key.indexOf("["));
                return ((VectorHash) hash.getContainer().get(key)).size() > index;
            }
        } catch (Exception e) {
        }
        return false;
    }

    /**
     * Check if path is part of Hash.
     *
     * @param path is a string key with "." as a separator.
     * @return true if part belongs to Hash.
     */
    public boolean has(String path) {
        return has(path, ".");
    }

    /**
     * Check if path is part of Hash. This form is used in Python.
     *
     * @param path a string key
     * @return true if the path belongs to Hash
     */
    public boolean __contain__(String path) {
        return has(path);
    }

    /**
     * Make Hash container empty.
     */
    public void clear() {
        m_container.clear();
    }

    /**
     * Get number of elements on the top level of hierarchy
     *
     * @return number of elements
     */
    public int size() {
        return m_container.size();
    }

    /**
     * Get number of elements on the top level of hierarchy. This form used in Python.
     *
     * @return number of elements
     */
    public int __len__() {
        return size();
    }

    /**
     * Get Hash node by path and separator.
     *
     * @param path <i>path</i> is a string key
     * @param separator <i>separator</i> is a character used for discovering a hierarchy in <b>path</b>.
     * @return requested node
     */
    public Node getNode(String path, String separator) {
        Hash hash = getLastHash(path, separator);
        String key = getLastKey(path, separator);
        int index = tryGetIndex(key);
        if (index == -1) {
            return hash.getContainer().getNode(key);
        }
        throw new RuntimeException("Array syntax on a leaf is not possible (should be a Hash and not a Node)");
    }

    /**
     * Get Hash node by path considering "." character as separator.
     *
     * @param key <i>key</i> is a string, optionally containing a hierarchy.
     * @return requested node.
     */
    public Node getNode(String key) {
        return this.getNode(key, ".");
    }

    private void setNode(String key, Node node) {
        m_container.set(key, node);
    }

    /**
     * Find Hash node by path and separator.
     *
     * @param path <i>path</i> is requested path to node
     * @param separator <i>separator</i> is a separator character used for discovering a hierarchy.
     * @return found node or none
     */
    public Node find(String path, String separator) {
        try {
            return this.getNode(path, separator);
        } catch (Exception e) {
        }
        return new Node();
    }

    /**
     * Find Hash node by path and default separator ".".
     *
     * @param path <i>path</i> is requested path to node
     * @return found node or none
     */
    public Node find(String path) {
        return find(path, ".");
    }

    /**
     * Get value type using path and separator
     *
     * @param path <i>path</i> is requested path to value type
     * @param separator <i>separator</i> is a separator character used for discovering a hierarchy.
     * @return type as a <i>ReferenceType</i> enumeration value
     */
    public Types.ReferenceType getType(String path, String separator) {
        Hash hash = getLastHash(path, separator);
        String key = getLastKey(path, separator);
        int index = tryGetIndex(key);
        if (index == -1) {
            return hash.getContainer().getNode(key).getType();
        } else {
            return Types.ReferenceType.HASH;
        }
    }

    /**
     * Get value type using path and default separator ".".
     *
     * @param path <i>path</i> is requested path to value type
     * @return type as a <i>ReferenceType</i> enumeration value.
     */
    public Types.ReferenceType getType(String path) {
        return getType(path, ".");
    }

    private String getLastKey(String path, String separator) {
        StringTokenizer st = new StringTokenizer(path, separator);
        String token = st.nextToken();
        while (st.hasMoreTokens()) {
            token = st.nextToken();
        }
        return token;
    }

    private Hash getLastHash(String path, String separator) {
        StringTokenizer st = new StringTokenizer(path, separator);
        String token = st.nextToken();
        Hash hash = this;
        while (st.hasMoreTokens()) {
            OrderedMap container = hash.getContainer();
            int index = tryGetIndex(token);
            if (index == -1) {
                if (!container.has(token)) {
                    throw new RuntimeException("No token '" + token + "' found in container.");
                }
                hash = container.getNode(token).getValue();
            } else {
                token = token.substring(0, token.indexOf("["));
                if (!container.has(token)) {
                    throw new RuntimeException("No token '" + token + "' found in container.");
                }
                VectorHash hashes = container.getNode(token).getValue();
                if (hashes.size() <= index) {
                    throw new RuntimeException("Index '" + index + "' out of range(0," + hashes.size() + ").");
                }
                hash = hashes.get(index);
            }
            token = st.nextToken();
        }
        return hash;
    }

    private int tryGetIndex(String str) {
        if (str.isEmpty()) {
            return -1;
        }
        int ei = str.length() - 1;
        if (str.charAt(ei) == ']') {
            int si = str.indexOf('[');
            if (si == -1 || si > ei) {
                return -1;
            }
            return Integer.parseInt(str.substring(si + 1, ei));
        }
        return -1;
    }

    private Hash setNodesAsNeeded(String path, String separator) {
        StringTokenizer st = new StringTokenizer(path, separator);
        String token = st.nextToken();
        Hash hash = this;
        while (st.hasMoreTokens()) {
            OrderedMap container = hash.getContainer();
            int index = tryGetIndex(token);
            if (index == -1) {
                if (container.has(token)) {
                    Node node = container.getNode(token);
                    if (!node.is(Hash.class)) {
                        node.setValue(new Hash());   // force node to be Hash
                    }
                    hash = node.getValue();
                } else {
                    hash = new Hash();
                    container.set(token, hash);
                }
            } else {
                token = token.substring(0, token.indexOf("["));
                if (container.has(token)) {
                    Node theNode = container.getNode(token);
                    if (!(theNode.getValue() instanceof VectorHash)) {
                        theNode.setValue(new VectorHash(index + 1));
                    }
                    VectorHash hashes = theNode.getValue();
                    if (hashes.size() <= index) {
                        hashes.ensureCapacity(index + 1);
                    }
                    hash = hashes.get(index);
                } else {
                    VectorHash hashes = container.set(token, new VectorHash(index + 1)).getValue();
                    hash = hashes.get(index);
                }
            }
            token = st.nextToken();
        }
        return hash;
    }

    /**
     * Insert key/value pair into current container
     *
     * @param path <i>path</i> is a path to associated value
     * @param value <i>value</i> set under specified above <i>path</i>.
     * @param separator <i>separator</i> used for setting up a hierarchy in <i>path</i>.
     * @return just created new node.
     */
    public Node set(String path, Object value, String separator) {
        try {
            Hash leaf = setNodesAsNeeded(path, separator);
            String key = getLastKey(path, separator);
            int index = tryGetIndex(key);
            if (index == -1) {
                return leaf.getContainer().set(key, value);
            } else {
                key = key.substring(0, key.indexOf("["));
                if (leaf.getContainer().has(key)) {
                    Node node = leaf.getNode(key);
                    if (!node.is(VectorHash.class)) {
                        VectorHash hashes = new VectorHash(index + 1);
                        hashes.set(index, (Hash) value);
                        node.setValue((VectorHash) hashes.clone());
                    } else {
                        VectorHash hashes = (VectorHash) node.getValue();
                        if (index >= hashes.size()) {
                            hashes.ensureCapacity(index + 1);
                        }
                        hashes.set(index, (Hash) value);
                    }
                    return node;
                } else {
                    VectorHash hashes = new VectorHash(index + 1);
                    hashes.set(index, (Hash) value);
                    return leaf.getContainer().set(key, hashes);
                }
            }
        } catch (Exception e) {
            throw new RuntimeException("Only Hash objects may be assigned to a leaf node of array type: " + e);
        }
    }

    /**
     * Insert key/value pair into current container
     *
     * @param key <i>key</i> is a path to associated value. Default separator "." is used.
     * @param value <i>value</i> set under specified above <i>path</i>.
     * @return just created new node is returned.
     */
    public Node set(String key, Object value) {
        return this.set(key, value, ".");
    }

    public Node set(String key, Types.ReferenceType type, Object value) {
        switch (type) {
            case BOOL:
                return this.set(key, Boolean.parseBoolean((String)value));
            case VECTOR_BOOL:
                return this.set(key, new VectorBoolean((String)value));
            case CHAR:
                return this.set(key, ((String)value).charAt(0));
            case VECTOR_CHAR:
                return this.set(key, new VectorCharacter((String)value));
            case INT8:
            case UINT8:
                return this.set(key, (byte)Integer.parseInt((String)value));
            case VECTOR_INT8:
            case VECTOR_UINT8:
                return this.set(key, new VectorByte((String)value));
            case INT16:
            case UINT16:
                return this.set(key, Short.parseShort((String)value));
            case VECTOR_INT16:
            case VECTOR_UINT16:
                return this.set(key, new VectorShort((String)value));
            case INT32:
            case UINT32:
                return this.set(key, Integer.parseInt((String)value));
            case VECTOR_INT32:
            case VECTOR_UINT32:
                return this.set(key, new VectorInteger((String)value));
            case INT64:
            case UINT64:
                return this.set(key, Long.parseLong((String)value));
            case VECTOR_INT64:
            case VECTOR_UINT64:
                return this.set(key, new VectorLong((String)value));
            case FLOAT:
                return this.set(key, Float.parseFloat((String)value));
            case VECTOR_FLOAT:
                return this.set(key, new VectorFloat((String)value));
            case DOUBLE:
                return this.set(key, Double.parseDouble((String)value));
            case VECTOR_DOUBLE:
                return this.set(key, new VectorDouble((String)value));
            case STRING:
                return this.set(key, value);
            case VECTOR_STRING:
                return this.set(key, new VectorString((String)value));
            case HASH:
                return this.set(key, (Hash)value);
            case VECTOR_HASH:
                return this.set(key, (VectorHash)value);
            case SCHEMA:
                return this.set(key, (Schema)value);
            //case COMPLEX_FLOAT:
            //case VECTOR_COMPLEX_FLOAT:
            case COMPLEX_DOUBLE:
                return this.set(key, new ComplexDouble((String)value));
            case VECTOR_COMPLEX_DOUBLE:
            case NONE:
            case VECTOR_NONE:
            default:
                break;
        }
        throw new RuntimeException("Unsupported type " + type);
    }

    /**
     * Insert key/value pair into current container. This form is used in Python.
     *
     * @param key <i>key</i> is a path to associated value. Default separator "." is used.
     * @param value <i>value</i> set under specified above <i>path</i>.
     * @return just created new node is returned.
     */
    public Node __setitem__(String key, Object value) {
        return this.set(key, value);
    }

    /**
     * Copy source node to current node.
     *
     * @param srcElement source node
     * @return current (destination) node.
     */
    public Node setNode(Node srcElement) {
        Node destElement = this.set(srcElement.getKey(), srcElement.getValue());
        destElement.setAttributes(srcElement.getAttributes());
        return destElement;
    }

    /**
     * Erase element with key "path"
     *
     * @param path string key representing hierarchy using separator
     * @param separator character representing separator
     */
    public void erase(String path, String separator) {
        Hash hash = getLastHash(path, separator);
        String key = getLastKey(path, separator);
        int index = tryGetIndex(key);
        if (index == -1) {
            hash.getContainer().erase(key);
        } else {
            VectorHash vectorHash = (VectorHash) hash.getContainer().get(key);
            vectorHash.remove(index);
        }
    }

    /**
     * Erase element with key "path"
     *
     * @param key string key representing hierarchy using default (".") separator.
     */
    public void erase(String key) {
        this.erase(key, ".");
    }

    /**
     * Erase element with key "path". This form is used in Python.
     *
     * @param key string key representing hierarchy using default (".") separator.
     */
    public void __delitem__(String key) {
        this.erase(key);
    }

    /**
     * Get all keys in Hash container
     *
     * @return set of keys.
     */
    public Set<String> getKeys() {
        return m_container.getKeys();
    }

    /**
     * Get all nodes in current Hash container.
     *
     * @return collection of nodes.
     */
    public Collection<Node> values() {
        return m_container.values();
    }

    /**
     * Get all paths (full path names) in current Hash container.
     *
     * @param separator character used as a separator
     * @return collection of paths as a VectorString object.
     */
    public VectorString getPaths(String separator) {
        VectorString result = new VectorString();
        if (this.empty()) {
            return result;
        }
        getPaths(this, result, "", separator);
        return result;
    }

    /**
     * Get all paths (full path names) in hash container.
     *
     * @param hash Hash container.
     * @param paths collection of paths.
     * @param prefix prefix string
     * @param separator character used for discovering a hierarchy in path names.
     */
    public static void getPaths(Hash hash, Collection<String> paths, String prefix, String separator) {
        if (hash.empty()) {
            paths.add(prefix);
            return;
        }
        for (Node node : hash.values()) {
            String currentKey = node.getKey();

            if (!prefix.isEmpty()) {
                currentKey = prefix + separator + currentKey;
            }
            if (node.is(Hash.class)) {
                getPaths((Hash) node.getValue(), paths, currentKey, separator);
            } else {
                if (node.is(VectorHash.class)) {
                    for (int i = 0; i < ((VectorHash) node.getValue()).size(); i++) {
                        String newKey = currentKey + "[" + Integer.toString(i) + "]";
                        getPaths(((VectorHash) node.getValue()).get(i), paths, newKey, separator);
                    }
                } else {
                    paths.add(currentKey);
                }
            }
        }
    }

    /**
     * Get all paths (full path names) in current Hash container.
     *
     * @return collection of paths as a VectorString object.
     */
    public VectorString getPaths() {
        return getPaths(".");
    }

    /**
     * Get all pairs: string-node
     *
     * @return set of string-node pairs
     */
    public Set<Entry<String, Node>> entrySet() {
        return m_container.entrySet();
    }

    @Override
    public Iterator<Entry<String, Node>> iterator() {
        return m_container.entrySet().iterator();
    }

    @Override
    public int hashCode() {
        return m_container.hashCode();
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        final Hash other = (Hash) obj;
        if (!Objects.equals(this.m_container, other.m_container)) {
            return false;
        }
        return true;
    }

    /**
     * Check if <i>path</i> is of class <i>type</i>
     *
     * @param path path to check
     * @param type expected type of value
     * @param separator character used for discovering a hierarchy in <i>path</i>.
     * @return true if value type is of class <i>type</i>.
     */
    public boolean is(String path, Class<? extends Object> type, String separator) {
        Hash hash = getLastHash(path, separator);
        String key = getLastKey(path, separator);
        int index = tryGetIndex(key);
        if (index == -1) {
            return hash.getContainer().getNode(key).is(type);
        } else {
            key = key.substring(0, key.indexOf("["));
            VectorHash vh = (VectorHash) hash.getContainer().get(key);
            if (vh.size() <= index) {
                throw new RuntimeException("VectorHash index " + Integer.toString(index)
                        + " is out of range [0," + Integer.toString(vh.size()) + "]");
            }
            return vh.get(index) instanceof Hash;
        }
    }

    /**
     * Check if <i>path</i> is of ReferenceType <i>type</i>.
     *
     * @param path path to check
     * @param type expected value type as an item from ReferenceType enumeration.
     * @param separator character used for discovering a hierarchy in <i>path</i>.
     * @return true if value type is of <i>type</i>.
     */
    public boolean is(String path, Types.ReferenceType type, String separator) {
        return getType(path, separator) == type;
    }

    /**
     * Check if <i>path</i> is of class <i>type</i>.
     *
     * @param path path to check. Separator is ".".
     * @param type expected type of value
     * @return true if value type is of <i>type</i>.
     */
    public boolean is(String path, Class<? extends Object> type) {
        return is(path, type, ".");
    }

    /**
     * Check if <i>path</i> is of ReferenceType <i>type</i>.
     *
     * @param path path to check. Separator is ".".
     * @param type expected value type as an item from ReferenceType enumeration.
     * @return true if value type is of <i>type</i>.
     */
    public boolean is(String path, Types.ReferenceType type) {
        return is(path, type, ".");
    }

    /**
     * Build <i>flat</i> Hash object where all keys on the same level: no hierarchy.
     *
     * @param hash input Hash object.
     * @param flat output flat Hash object.
     * @param prefix string prefix.
     * @param separator character used for getting hierarchy.
     */
    public static void flatten(Hash hash, Hash flat, String prefix, String separator) {
        if (!hash.empty()) {
            for (Node node : hash.values()) {
                String currentKey = node.getKey();
                if (!prefix.isEmpty()) {
                    currentKey = prefix + separator + currentKey;
                }
                if (node.is(Hash.class)) {
                    flatten((Hash) node.getValue(), flat, currentKey, separator);
                } else {
                    if (node.is(VectorHash.class)) {
                        VectorHash vh = (VectorHash) node.getValue();
                        for (int i = 0; i < vh.size(); ++i) {
                            String currentKeyWithIndex = currentKey + "[" + Integer.toString(i) + "]";
                            flatten(vh.get(i), flat, currentKeyWithIndex, separator);
                        }
                    } else {
                        flat.set(currentKey, node.getValue(), "");
                        flat.setAttributes(currentKey, node.getAttributes(), "");
                    }
                }
            }
        }
    }

    /**
     * Build <i>flat</i> Hash object where all keys on the same level: no hierarchy.
     *
     * @param separator character used for getting hierarchy.
     * @return flattened Hash object.
     */
    public Hash flatten(String separator) {
        Hash flat = new Hash();
        flatten(this, flat, "", separator);
        return flat;
    }

    /**
     * Build <i>flat</i> Hash object where all keys on the same level: no hierarchy. Character "." is used as a
     * separator.
     *
     * @return flattened Hash object.
     */
    public Hash flatten() {
        return flatten(".");
    }

    /**
     * Build <i>hierarchical</i> Hash object from flattened Hash.
     *
     * @param separator character used for getting hierarchy.
     * @return hierarchical Hash object.
     */
    public Hash unflatten(String separator) {
        Hash tree = new Hash();
        for (Node node : this.values()) {
            tree.set(node.getKey(), node.getValue(), separator);
            tree.setAttributes(node.getKey(), node.getAttributes(), separator);
        }
        return tree;
    }

    /**
     * Build <i>hierarchical</i> Hash object from flattened Hash. Character "." is used as a separator.
     *
     * @return hierarchical Hash object.
     */
    public Hash unflatten() {
        return unflatten(".");
    }

    /**
     * Merge <i>other</i> Hash object into current Hash.
     *
     * @param other external Hash Object.
     * @param policy merging policy used.
     * @return merged Hash object.
     */
    public Hash merge(Hash other, MergePolicy policy) {
        if (policy == MergePolicy.MERGE_ATTRIBUTES) {
            mergeAndMergeAttributes(other);
        } else if (policy == MergePolicy.REPLACE_ATTRIBUTES) {
            mergeAndReplaceAttributes(other);
        }
        return this;
    }

    private void mergeAndMergeAttributes(Hash other) {
        if (this.empty() && other.empty()) {
            m_container = new OrderedMap(other.getContainer());
            return;
        }
        for (Node otherNode : other.values()) {
            String key = otherNode.getKey();
            if (this.has(key)) {
                Node thisNode = this.getNode(key);
                Attributes attrs = otherNode.getAttributes();
                for (Map.Entry<String, Object> entry : attrs.entrySet()) {
                    thisNode.setAttribute(entry.getKey(), entry.getValue());
                }

                // Both nodes are Hash
                if (thisNode.is(Hash.class) && otherNode.is(Hash.class)) {
                    ((Hash) thisNode.getValue()).mergeAndMergeAttributes((Hash) otherNode.getValue());
                    continue;
                }

                // Both nodes are vector<Hash>
                if (thisNode.is(VectorHash.class) && otherNode.is(VectorHash.class)) {
                    VectorHash this_vec = (VectorHash) thisNode.getValue();
                    VectorHash other_vec = (VectorHash) otherNode.getValue();
                    for (int i = 0; i < other_vec.size(); i++) {
                        this_vec.add(other_vec.get(i));
                    }
                    continue;
                }
                thisNode.setValue(otherNode.getValue());
            } else {
                this.setNode(key, otherNode);
            }
        }
    }

    private void mergeAndReplaceAttributes(Hash other) {
        if (this.empty() && !other.empty()) {
            m_container = new OrderedMap(other.getContainer());
            return;
        }
        for (Node otherNode : other.values()) {
            String key = otherNode.getKey();
            if (this.has(key)) {
                Node thisNode = this.getNode(key);
                Attributes attrs = otherNode.getAttributes();
                thisNode.setAttributes(attrs);

                // Both nodes are Hash
                if (thisNode.is(Hash.class) && otherNode.is(Hash.class)) {
                    ((Hash) thisNode.getValue()).mergeAndReplaceAttributes((Hash) otherNode.getValue());
                    continue;
                }

                // Both nodes are vector<Hash>
                if (thisNode.is(VectorHash.class) && otherNode.is(VectorHash.class)) {
                    VectorHash this_vec = (VectorHash) thisNode.getValue();
                    VectorHash other_vec = (VectorHash) otherNode.getValue();
                    for (int i = 0; i < other_vec.size(); i++) {
                        this_vec.add(other_vec.get(i));
                    }
                    continue;
                }
                thisNode.setValue(otherNode.getValue());
            } else {
                this.setNode(key, otherNode);
            }
        }
    }

    /**
     * Merge <i>other</i> Hash object into current Hash using default merging policy: MergePolicy.REPLACE_ATTRIBUTES.
     *
     * @param other external Hash Object.
     * @return merged Hash object.
     */
    public Hash merge(Hash other) {
        return merge(other, MergePolicy.REPLACE_ATTRIBUTES);
    }

    /**
     * Merge <i>other</i> Hash object into current Hash using default merging policy: MergePolicy.REPLACE_ATTRIBUTES.
     * This form is used in Python in "+=" operator.
     *
     * @param other external Hash Object.
     * @return merged Hash object.
     */
    public Hash __iadd__(Hash other) {
        return merge(other);
    }

    /**
     * Get all attributes belonging to <i>path</i> using <i>separator</i>.
     *
     * @param path <i>path</i> in current Hash object.
     * @param separator character used as a separator in <i>path</i>.
     * @return all attributes as Attributes object.
     */
    public Attributes getAttributes(String path, String separator) {
        return this.getNode(path, separator).getAttributes();
    }

    /**
     * Get all attributes belonging to <i>path</i> using "." as separator.
     *
     * @param path <i>path</i> in current Hash object.
     * @return Attributes object.
     */
    public Attributes getAttributes(String path) {
        return this.getAttributes(path, ".");
    }

    /**
     * Set attributes for given <i>path</i> by external <i>attributes</i> object
     *
     * @param path <i>path</i> in current Hash object.
     * @param attributes external Attributes object.
     * @param separator separator character.
     */
    public void setAttributes(String path, Attributes attributes, String separator) {
        getNode(path, separator).setAttributes(attributes);
    }

    /**
     * Set attributes for given <i>path</i> by external <i>attributes</i> object.
     *
     * @param path <i>path</i> in current Hash object. Character "." used as separator.
     * @param attributes external Attributes object.
     */
    public void setAttributes(String path, Attributes attributes) {
        this.setAttributes(path, attributes, ".");
    }

    /**
     * Check if <i>attribute</i> belongs to node with key <i>path</i>.
     *
     * @param path requested key.
     * @param attribute tested attribute.
     * @param separator character used as a separator.
     * @return true if <i>attribute</i> belongs to node with key <i>path</i>.
     */
    public boolean hasAttribute(String path, String attribute, String separator) {
        return getNode(path, separator).hasAttribute(attribute);
    }

    /**
     * Check if <i>attribute</i> belongs to node with key <i>path</i>.
     *
     * @param path requested key. Default separator is ".".
     * @param attribute tested attribute.
     * @return true if <i>attribute</i> belongs to node with key <i>path</i>.
     */
    public boolean hasAttribute(String path, String attribute) {
        return hasAttribute(path, attribute, ".");
    }

    /**
     * Get attribute value by <i>path</i> and <i>attribute</i>
     *
     * @param <AttributeType> attribute value type as Generic parameter.
     * @param path requested path.
     * @param attribute requested attribute.
     * @param separator requested separator.
     * @return attribute value of <i>AttributeType</i> type.
     */
    public <AttributeType extends Object> AttributeType getAttribute(String path, String attribute, String separator) {
        return (AttributeType) getNode(path, separator).<AttributeType>getAttribute(attribute);
    }

    /**
     * Get attribute value by <i>path</i> and <i>attribute</i>
     *
     * @param <AttributeType> attribute value type as Generic parameter.
     * @param path requested path. Default separator is ".".
     * @param attribute requested attribute.
     * @return attribute value of <i>AttributeType</i> type.
     */
    public <AttributeType extends Object> AttributeType getAttribute(String path, String attribute) {
        return (AttributeType) this.<AttributeType>getAttribute(path, attribute, ".");
    }

    /**
     * Set new attribute value for <i>path</i> and <i>attribute</i>.
     *
     * @param path used path.
     * @param attribute used attribute.
     * @param value attribute value to set.
     * @param separator used separator character.
     */
    public void setAttribute(String path, String attribute, Object value, String separator) {
        getNode(path, separator).setAttribute(attribute, value);
    }

    /**
     * Set new attribute value for <i>path</i> and <i>attribute</i>.
     *
     * @param path used path. Default separator is ".".
     * @param attribute used attribute.
     * @param value attribute value to set.
     */
    public void setAttribute(String path, String attribute, Object value) {
        setAttribute(path, attribute, value, ".");
    }

    /**
     * Check if both Hash objects have the same hierarchical structure: same keys, same value types, but values may
     * differ.
     *
     * @param left left Hash object.
     * @param right right Hash object.
     * @return true if "<i>similar</i>".
     */
    public static boolean similar(Hash left, Hash right) {
        if (left.size() != right.size()) {
            return false;
        }

        for (String path : left.getPaths()) {
            if (!similar(left.getNode(path), right.getNode(path))) {
                return false;
            }
        }
        return true;
    }

    /**
     * Check if both VectorHash objects have the same hierarchical structure: same keys, same value types, but values
     * may differ.
     *
     * @param left left VectorHash object.
     * @param right right VectorHash object.
     * @return true if "<i>similar</i>".
     */
    public static boolean similar(VectorHash left, VectorHash right) {
        if (left.size() != right.size()) {
            return false;
        }

        for (int i = 0; i < left.size(); i++) {
            if (!similar(left.get(i), right.get(i))) {
                return false;
            }
        }
        return true;
    }

    /**
     * Check if both Node objects have the same hierarchical structure: same keys, same value types, but values may
     * differ.
     *
     * @param left left Node object.
     * @param right right Node object.
     * @return true if "<i>similar</i>".
     */
    public static boolean similar(Node left, Node right) {
        if (!left.getKey().equals(right.getKey())) {
            return false;
        }
        if (left.getValue().getClass() != right.getValue().getClass()) {
            return false;
        }
        if (left.getValue() != right.getValue()) {
            return false;
        }
        return true;
    }

    /**
     * String representation of Hash object. Used when trying to print using <i>System.out.println</i>, for example.
     *
     * @return string view of Hash.
     */
    @Override
    public String toString() {
        try {
            return toStream(this, 0);
        } catch (Exception e) {
            throw new RuntimeException("Exception in Hash.toStream: " + e);
        }
        //return m_container.toString();
    }

    /**
     * String representation of Hash object.
     *
     * @param hash Hash object to convert to String
     * @param depth current depth used to calculate proper indentation.
     * @return resulting String object.
     */
    protected String toStream(Hash hash, int depth) {
        StringBuilder os = new StringBuilder();
        String newLine = System.getProperty("line.separator");
        char[] fill = new char[2 * depth];
        Arrays.fill(fill, ' ');

        for (Node node : hash.values()) {
            os.append(fill);
            os.append(node.getKey());
            Attributes attrs = node.getAttributes();
            if (attrs.size() > 0) {
                for (Entry<String, Object> e : attrs.entrySet()) {
                    os.append(" ").append(e.getKey()).append("=\"").append(e.getValue().toString()).append("\"");
                }
            }
            Types.ReferenceType type = node.getType();
            if (type == Types.ReferenceType.HASH) {
                os.append(" +").append(newLine).append(toStream((Hash) node.getValue(), depth + 1));
            } else if (type == Types.ReferenceType.VECTOR_HASH) {
                VectorHash hashes = node.getValue();
                os.append(" @").append(newLine);
                for (int i = 0; i < hashes.size(); ++i) {
                    os.append(fill).append("[").append(i).append("]").append(newLine).append(toStream(hashes.get(i), depth + 1));
                }
            } else if (type == Types.ReferenceType.SCHEMA) {
                os.append(" => ").append((Schema) node.getValue()).append(newLine);
//            } else if (Types.isPointer(type)) {// TODO Add pointer types
//                os.append(" => xxx " + ToLiteral.to(type) + newLine);
            } else {
                os.append(" => ").append(node.getValue().toString()).append(" ").append(ToLiteral.to(type)).append(newLine);
            }
        }
        return os.toString();
    }
}
