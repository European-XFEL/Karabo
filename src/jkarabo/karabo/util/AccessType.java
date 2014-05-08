/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public enum AccessType {

    INIT(1),
    READ(1 << 1),
    WRITE(1 << 2),
    INIT_READ((1) | (1 << 1)),
    INIT_WRITE((1) | (1 << 2)),
    INIT_READ_WRITE((1) | (1 << 1) | (1 << 2));
    private final int code;

    private AccessType(int c) {
        code = c;
    }

    public int get() {
        return code;
    }
}
