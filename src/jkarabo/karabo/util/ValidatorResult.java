/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class ValidatorResult {
    
    public boolean success = true;
    public String message = "";

    public ValidatorResult() {
    }

    public ValidatorResult(boolean status, String error) {
        success = status;
        message = error;
    }
}
