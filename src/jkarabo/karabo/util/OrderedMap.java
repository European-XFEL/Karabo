/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Set;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class OrderedMap {

    private LinkedHashMap<String, Node> m_mapNodes = new LinkedHashMap<>();

    public OrderedMap() {
    }

    public <ValueType extends Object> OrderedMap(String key1, ValueType val1) {
        this.set(key1, val1);
    }

    public <ValueType> OrderedMap(String key1, ValueType val1, String key2, ValueType val2) {
        this.set(key1, val1);
        this.set(key2, val2);
    }

    public <ValueType> OrderedMap(String key1, ValueType val1, String key2, ValueType val2,
            String key3, ValueType val3) {
        this.set(key1, val1);
        this.set(key2, val2);
        this.set(key3, val3);
    }

    public <ValueType> OrderedMap(String key1, ValueType val1, String key2, ValueType val2,
            String key3, ValueType val3, String key4, ValueType val4) {
        this.set(key1, val1);
        this.set(key2, val2);
        this.set(key3, val3);
        this.set(key4, val4);
    }

    public <ValueType> OrderedMap(String key1, ValueType val1, String key2, ValueType val2,
            String key3, ValueType val3, String key4, ValueType val4,
            String key5, ValueType val5) {
        this.set(key1, val1);
        this.set(key2, val2);
        this.set(key3, val3);
        this.set(key4, val4);
        this.set(key5, val5);
    }

    public <ValueType> OrderedMap(String key1, ValueType val1, String key2, ValueType val2,
            String key3, ValueType val3, String key4, ValueType val4,
            String key5, ValueType val5, String key6, ValueType val6) {
        this.set(key1, val1);
        this.set(key2, val2);
        this.set(key3, val3);
        this.set(key4, val4);
        this.set(key5, val5);
        this.set(key6, val6);
    }

    @SuppressWarnings("unchecked")
    public OrderedMap(OrderedMap other) {
        for (Node node : other.values()) {
            m_mapNodes.put(node.getKey(), new Node(node));
        }
    }

    private LinkedHashMap<String, Node> getContainer() {
        return m_mapNodes;
    }

    public boolean empty() {
        return m_mapNodes.isEmpty();
    }

    public void clear() {
        m_mapNodes.clear();
    }

    public int size() {
        return m_mapNodes.size();
    }

    /**
     * Check if key is part of current OrderedMap
     * @param key name
     * @return true if key exists
     */
    public boolean has(String key) {
        return m_mapNodes.containsKey(key);
    }

    /**
     * Erase element with key from OrderedMap
     * @param key name
     */
    public void erase(String key) {
        if (m_mapNodes.containsKey(key)) {
            m_mapNodes.remove(key);
        }
    }

    public Set<String> getKeys() {
        return m_mapNodes.keySet();
    }

    public Collection<Node> values() {
        return m_mapNodes.values();
    }

    public Set<Map.Entry<String, Node>> entrySet() {
        return m_mapNodes.entrySet();
    }

    public final <ValueType extends Object> Node set(String key, ValueType val) {
        if (m_mapNodes.containsKey(key)) {
            Node node = m_mapNodes.get(key);
            node.setValue(val);
            return node;
        } else {
            Node local = new Node(key, val);
            m_mapNodes.put(key, local);
            return m_mapNodes.get(key);
        }
    }

    public Node getNode(String key) {
        if (!m_mapNodes.containsKey(key)) {
            throw new RuntimeException("Key \"" + key + "\" does not exist");
        }
        return m_mapNodes.get(key);
    }

    public <ValueType extends Object> ValueType get(String key) {
        return getNode(key).getValue();
    }

    public <ValueType extends Object> void get(String key, ValueType val) {
        val = getNode(key).getValue();
    }

    public Object getAny(String key) {
        return (Object) getNode(key).getValue();
    }

    @Override
    public String toString() {
        return m_mapNodes.toString();
    }
}
