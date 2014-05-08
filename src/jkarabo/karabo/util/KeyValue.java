/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class KeyValue<K,V> {
    
    public K key;
    public V value;
    
    public KeyValue(K key, V value) {
        this.key = key;
        this.value = value;
    }
}
