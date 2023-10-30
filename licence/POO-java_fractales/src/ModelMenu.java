import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;

import javax.imageio.ImageIO;

public class ModelMenu {
	private BufferedImage menuBg;
	private Fractal.BuilderFractal builder; // can be changed when reset is press
	private final String[] listComboBoxTypes = { FractalTypes.JULIA.getName(), FractalTypes.MANDELBROT.getName() };
	private ComplexNumber complexC = ComplexNumber.createComplexNumber(0, 0);

	private ModelMenu() {
		try {
			File directory = new File(".");
			String path = directory.getCanonicalPath() + File.separator + "img" + File.separator + "menuBgFractal.png";
			menuBg = ImageIO.read(new File(path));
		} catch (IOException ex) {
			ex.printStackTrace();
			menuBg = null; // we will have menu without background...
		}
		builder = new Fractal.BuilderFractal();
	}

	public static ModelMenu createModelMenu() {
		return new ModelMenu();
	}

	public BufferedImage getMenuBg() {
		return menuBg;
	}

	public Fractal.BuilderFractal getBuilder() {
		return builder;
	}

	public void setBuilder(Fractal.BuilderFractal b) {
		builder = b;
	}

	public String[] getTypes() {
		return listComboBoxTypes;
	}

	public ComplexNumber getComplexC() {
		return complexC;
	}

}
