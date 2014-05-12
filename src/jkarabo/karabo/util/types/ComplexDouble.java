package karabo.util.types;

public class ComplexDouble {
    private final double re;   // the real part
    private final double im;   // the imaginary part

    // create a new object with the given real and imaginary parts
    public ComplexDouble(double real, double imag) {
        re = real;
        im = imag;
    }

    public ComplexDouble(String c) {
        String s = c.substring(2, c.length() - 1);
        String[] sa = s.split("[,]");
        if (sa.length == 2) {
            re = Double.parseDouble(sa[0]);
            im = Double.parseDouble(sa[1]);
        } else {
            re = Double.parseDouble(sa[0]);
            im = 0.0;
        }
    }
    // return a string representation of the invoking ComplexDouble object
    public String toString() {
        if (im == 0) return re + "";
        if (re == 0) return im + "i";
        if (im <  0) return re + " - " + (-im) + "i";
        return re + " + " + im + "i";
    }

    // return abs/modulus/magnitude and angle/phase/argument
    public double abs()   { return Math.hypot(re, im); }  // Math.sqrt(re*re + im*im)
    public double phase() { return Math.atan2(im, re); }  // between -pi and pi

    // return a new ComplexDouble object whose value is (this + b)
    public ComplexDouble plus(ComplexDouble b) {
        ComplexDouble a = this;             // invoking object
        double real = a.re + b.re;
        double imag = a.im + b.im;
        return new ComplexDouble(real, imag);
    }

    // return a new ComplexDouble object whose value is (this - b)
    public ComplexDouble minus(ComplexDouble b) {
        ComplexDouble a = this;
        double real = a.re - b.re;
        double imag = a.im - b.im;
        return new ComplexDouble(real, imag);
    }

    // return a new ComplexDouble object whose value is (this * b)
    public ComplexDouble times(ComplexDouble b) {
        ComplexDouble a = this;
        double real = a.re * b.re - a.im * b.im;
        double imag = a.re * b.im + a.im * b.re;
        return new ComplexDouble(real, imag);
    }

    // scalar multiplication
    // return a new object whose value is (this * alpha)
    public ComplexDouble times(double alpha) {
        return new ComplexDouble(alpha * re, alpha * im);
    }

    // return a new ComplexDouble object whose value is the conjugate of this
    public ComplexDouble conjugate() {  return new ComplexDouble(re, -im); }

    // return a new ComplexDouble object whose value is the reciprocal of this
    public ComplexDouble reciprocal() {
        double scale = re*re + im*im;
        return new ComplexDouble(re / scale, -im / scale);
    }

    // return the real or imaginary part
    public double re() { return re; }
    public double im() { return im; }

    // return a / b
    public ComplexDouble divides(ComplexDouble b) {
        ComplexDouble a = this;
        return a.times(b.reciprocal());
    }

    // return a new ComplexDouble object whose value is the complex exponential of this
    public ComplexDouble exp() {
        return new ComplexDouble(Math.exp(re) * Math.cos(im), Math.exp(re) * Math.sin(im));
    }

    // return a new ComplexDouble object whose value is the complex sine of this
    public ComplexDouble sin() {
        return new ComplexDouble(Math.sin(re) * Math.cosh(im), Math.cos(re) * Math.sinh(im));
    }

    // return a new ComplexDouble object whose value is the complex cosine of this
    public ComplexDouble cos() {
        return new ComplexDouble(Math.cos(re) * Math.cosh(im), -Math.sin(re) * Math.sinh(im));
    }

    // return a new ComplexDouble object whose value is the complex tangent of this
    public ComplexDouble tan() {
        return sin().divides(cos());
    }
    


    // a static version of plus
    public static ComplexDouble plus(ComplexDouble a, ComplexDouble b) {
        double real = a.re + b.re;
        double imag = a.im + b.im;
        ComplexDouble sum = new ComplexDouble(real, imag);
        return sum;
    }
}