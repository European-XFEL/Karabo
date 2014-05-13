/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

package karabo.util;

import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 * @author Sergey Esenov serguei.essenov@xfel.eu
 */
public class Registrator {
    
    private Registrator() {}   // private contructor means: 'Registrator' class is static only
    
    public final static void registerAll() {}  // call this function to result in execution of static block
    
    static {
        try {
            //System.out.println("static initializer called.");
            Class.forName("karabo.util.BinarySerializer");
            Class.forName("karabo.util.BinarySerializerHash");
            Class.forName("karabo.util.BinarySerializerSchema");
            Class.forName("karabo.util.HashBinarySerializer");
            Class.forName("karabo.util.SchemaBinarySerializer");
            
            Class.forName("karabo.io.AbstractInput");
            Class.forName("karabo.io.Input");
            Class.forName("karabo.io.InputHash");
            Class.forName("karabo.io.InputSchema");
            Class.forName("karabo.io.BinaryFileInputHash");
            Class.forName("karabo.io.BinaryFileInputSchema");
            
            Class.forName("karabo.io.AbstractOutput");
            Class.forName("karabo.io.Output");
            Class.forName("karabo.io.OutputHash");
            Class.forName("karabo.io.OutputSchema");
            Class.forName("karabo.io.BinaryFileOutputHash");
            Class.forName("karabo.io.BinaryFileOutputSchema");
            
            
            Class.forName("karabo.util.TextSerializer");
            Class.forName("karabo.util.TextSerializerHash");
            Class.forName("karabo.util.HashXmlSerializer");
            Class.forName("karabo.util.SchemaXmlSerializer");
            
            Class.forName("karabo.io.TextFileInput");
            Class.forName("karabo.io.TextFileOutput");
        } catch (ClassNotFoundException ex) {
            Logger.getLogger(Registrator.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
}
