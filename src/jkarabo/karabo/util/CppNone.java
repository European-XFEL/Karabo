/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

/**
 *
 * @author esenov
 */
public class CppNone {
    
    private static CppNone instance = null;
    
    private CppNone() {
    }
    
    public static CppNone getInstance() {
        if (instance == null) {
            instance = new CppNone();
        }
        return instance;
    }

    @Override
    public String toString() {
        return "None";
    }
}
