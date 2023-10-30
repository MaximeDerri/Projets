
public enum FractalTypes {
	JULIA("Julia Set"),
	MANDELBROT("Mandelbrot Set");
	
	private String name;
	
	private FractalTypes(String n) {
		name = n;
	}
	
	public String getName() {
		return name;
	}

}
