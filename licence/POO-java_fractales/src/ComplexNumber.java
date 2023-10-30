public class ComplexNumber {
	private double re;
	private double im;

	private ComplexNumber(double r, double i) {
		re = r;
		im = i;
	}

	public static ComplexNumber createComplexNumber(double r, double i) {
		return new ComplexNumber(r, i);
	}

	public static ComplexNumber copyOf(ComplexNumber c) throws IllegalArgumentException {
		if (c == null)
			throw new IllegalArgumentException();
		return new ComplexNumber(c.re, c.im);
	}

	@Override
	public String toString() {
		return re + " + i*(" + im + ")";
	}

	public ComplexNumber addComplexNumber(ComplexNumber z0) {
		return new ComplexNumber(re + z0.re, im + z0.im);
	}

	public ComplexNumber mulComplexNumber(ComplexNumber z0) {
		return new ComplexNumber((re * z0.re) - (im * z0.im), (re * z0.im) + (im * z0.re));
	}

	public ComplexNumber powComplexNumber(ComplexNumber z0, int p) {
		if (p <= 0)
			return new ComplexNumber(0, 0);
		if (p == 1)
			return z0;
		ComplexNumber ret = copyOf(z0);
		for (int i = p; i >= 2; i--) {
			ret = ret.mulComplexNumber(z0);
		}
		return ret;
	}

	public double modulusComplexNumber() {
		return Math.sqrt(Math.pow(re, 2) + Math.pow(im, 2));
	}

	public double getRe() {
		return re;
	}

	public void setRe(double re) {
		this.re = re;
	}

	public double getIm() {
		return im;
	}

	public void setIm(double im) {
		this.im = im;
	}

}
