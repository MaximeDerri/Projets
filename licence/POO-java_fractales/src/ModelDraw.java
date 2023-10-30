
public class ModelDraw {
	private Fractal fractal;
	private ComplexNumber z;
	private ComplexNumber c;
	private int xImage;
	private int yImage;
	private int rgb;
	
	private ModelDraw(Fractal f, ComplexNumber cn) {
		fractal = f;
		c = cn;
		z = ComplexNumber.createComplexNumber(0, 0);
		xImage = (int) ((fractal.getX2() - fractal.getX1()) * fractal.getZoom());
		yImage = (int) ((fractal.getY2() - fractal.getY1()) * fractal.getZoom());
		rgb = 500 * (int) ((Math.sqrt(100)) * 500); //later will be implement a color choice
	}
	
	public static ModelDraw createModelDraw(Fractal f, ComplexNumber c) throws IllegalArgumentException {
		if(f == null) throw new IllegalArgumentException("Problem with fractal on ModelDraw - createModelDraw()");
		if(c == null) throw new IllegalArgumentException("Problem with complex on ModelDraw - createModelDraw()");
		return new ModelDraw(f, c);
	}
	
	public Fractal getFractal() {
		return fractal;
	}
	
	public int getXImage() {
		return xImage;
	}
	
	public int getYImage() {
		return yImage;
	}
	
	public ComplexNumber getZ() {
		return z;
	}
	
	public ComplexNumber getC() {
		return c;
	}

	public int getRgb() {
		return rgb;
	}

}
