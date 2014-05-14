/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

package karabo.util.types;

/**
 *
 * @author Sergey Esenov serguei.essenov@xfel.eu
 */
public class ComplexFloat {

    private final float re;   // the real part
    private final float im;   // the imaginary part

    // create a new object with the given real and imaginary parts
    public ComplexFloat(float real, float imag) {
        re = real;
        im = imag;
    }

    public ComplexFloat(String c) {
        String s = c.substring(1, c.length() - 1); // remove parentheses (round brackets)
        String[] sa = s.split("[,]");
        if (sa.length == 2) {
            re = Float.parseFloat(sa[0]);
            im = Float.parseFloat(sa[1]);
        } else {
            re = Float.parseFloat(sa[0]);
            im = 0.0f;
        }
    }

    // return a string representation of the invoking ComplexFloat object

    @Override
    public String toString() {
        return "(" + re + "," + im + ")";
    }

    // return abs/modulus/magnitude and angle/phase/argument
    public float abs() {
        return (float)Math.hypot(re, im);
    }  // Math.sqrt(re*re + im*im)

    public float phase() {
        return (float)Math.atan2(im, re);
    }  // between -pi and pi

    // return a new ComplexFloat object whose value is (this + b)
    public ComplexFloat plus(ComplexFloat b) {
        ComplexFloat a = this;             // invoking object
        float real = a.re + b.re;
        float imag = a.im + b.im;
        return new ComplexFloat(real, imag);
    }

    // return a new ComplexFloat object whose value is (this - b)
    public ComplexFloat minus(ComplexFloat b) {
        ComplexFloat a = this;
        float real = a.re - b.re;
        float imag = a.im - b.im;
        return new ComplexFloat(real, imag);
    }

    // return a new ComplexFloat object whose value is (this * b)
    public ComplexFloat times(ComplexFloat b) {
        ComplexFloat a = this;
        float real = a.re * b.re - a.im * b.im;
        float imag = a.re * b.im + a.im * b.re;
        return new ComplexFloat(real, imag);
    }

    // scalar multiplication
    // return a new object whose value is (this * alpha)
    public ComplexFloat times(float alpha) {
        return new ComplexFloat(alpha * re, alpha * im);
    }

    // return a new ComplexFloat object whose value is the conjugate of this
    public ComplexFloat conjugate() {
        return new ComplexFloat(re, -im);
    }

    // return a new ComplexFloat object whose value is the reciprocal of this
    public ComplexFloat reciprocal() {
        float scale = re * re + im * im;
        return new ComplexFloat(re / scale, -im / scale);
    }

    // return the real or imaginary part
    public float re() {
        return re;
    }

    public float im() {
        return im;
    }

    // return a / b
    public ComplexFloat divides(ComplexFloat b) {
        ComplexFloat a = this;
        return a.times(b.reciprocal());
    }

    // return a new ComplexFloat object whose value is the complex exponential of this
    public ComplexFloat exp() {
        return new ComplexFloat((float)(Math.exp(re) * Math.cos(im)), (float)(Math.exp(re) * Math.sin(im)));
    }

    // return a new ComplexFloat object whose value is the complex sine of this
    public ComplexFloat sin() {
        return new ComplexFloat((float)(Math.sin(re) * Math.cosh(im)), (float)(Math.cos(re) * Math.sinh(im)));
    }

    // return a new ComplexFloat object whose value is the complex cosine of this
    public ComplexFloat cos() {
        return new ComplexFloat((float)(Math.cos(re) * Math.cosh(im)), (float)(-Math.sin(re) * Math.sinh(im)));
    }

    // return a new ComplexFloat object whose value is the complex tangent of this
    public ComplexFloat tan() {
        return sin().divides(cos());
    }

    // a static version of plus
    public static ComplexFloat plus(ComplexFloat a, ComplexFloat b) {
        float real = a.re + b.re;
        float imag = a.im + b.im;
        ComplexFloat sum = new ComplexFloat(real, imag);
        return sum;
    }
}
