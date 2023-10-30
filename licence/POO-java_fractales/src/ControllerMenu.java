
public class ControllerMenu {
	private ViewMenu view;
	private ModelMenu model;

	private ControllerMenu(ModelMenu m) {
		model = m;
	}

	public static ControllerMenu createControllerMenu(ModelMenu m) {
		if (m == null)
			throw new IllegalArgumentException("model is not set");

		return new ControllerMenu(m);
	}

	public void setViewMenu(ViewMenu v) {
		if (v == null)
			throw new IllegalArgumentException("view is not set");
		view = v;
	}

	public void setSelectedFractalType(String select) {
		// searching
		if (select.equals(FractalTypes.JULIA.getName())) {
			// can precise the C ComplexNumber
			model.getBuilder().setType(FractalTypes.JULIA);
			view.getComplexCRe().setEnabled(true);
			view.getComplexCIm().setEnabled(true);
		} else if (select.equals(FractalTypes.MANDELBROT.getName())) {
			// cannot precise the C ComplexNumber
			model.getBuilder().setType(FractalTypes.MANDELBROT);
			view.getComplexCRe().setEnabled(false);
			view.getComplexCIm().setEnabled(false);
			model.getComplexC().setRe(0);
			model.getComplexC().setIm(0);
			view.getComplexCRe().setValue(0);
			view.getComplexCIm().setValue(0);
		} else
			throw new IllegalArgumentException("Problem with selected Fractal name.");
	}

	public void setCorrectFormattedTextX1(double n) {
		try {
			model.getBuilder().setX1(n);
			view.getFTx1().setValue(model.getBuilder().getX1());
		} catch (NumberFormatException ex) {
			ex.printStackTrace();
			System.exit(1);
		}
	}

	public void setCorrectFormattedTextX2(double n) {
		try {
			model.getBuilder().setX2(n);
			view.getFTx2().setValue(model.getBuilder().getX2());
		} catch (NumberFormatException ex) {
			ex.printStackTrace();
			System.exit(1);
		}
	}

	public void setCorrectFormattedTextY1(double n) {
		try {
			model.getBuilder().setY1(n);
			view.getFTy1().setValue(model.getBuilder().getY1());
		} catch (NumberFormatException ex) {
			ex.printStackTrace();
			System.exit(1);
		}
	}

	public void setCorrectFormattedTextY2(double n) {
		try {
			model.getBuilder().setY2(n);
			view.getFTy2().setValue(model.getBuilder().getY2());
		} catch (NumberFormatException ex) {
			ex.printStackTrace();
			System.exit(1);
		}
	}

	public void setCorrectFormattedTextZoom(int n) {
		try {
			model.getBuilder().setZoom(n);
			view.getFTzoom().setValue(model.getBuilder().getZoom());
		} catch (NumberFormatException ex) {
			ex.printStackTrace();
			System.exit(1);
		}
	}

	public void setCorrectFormattedTextIter(int n) {
		try {
			model.getBuilder().setIter(n);
			view.getFTiter().setValue(model.getBuilder().getIter());
		} catch (NumberFormatException ex) {
			ex.printStackTrace();
			System.exit(1);
		}
	}

	public void setCorrectFormattedTextStep(double n) {
		try {
			model.getBuilder().setStep(n);
			view.getFTstep().setValue(model.getBuilder().getStep());
		} catch (NumberFormatException ex) {
			ex.printStackTrace();
			System.exit(1);
		}
	}

	public void setCorrectFormattedTextPow(int n) {
		try {
			model.getBuilder().setPow(n);
			view.getFTpow().setValue(model.getBuilder().getPow());
		} catch (NumberFormatException ex) {
			ex.printStackTrace();
			System.exit(1);
		}
	}

	public void setCorrectFormattedTextComplexCRe(double n) {
		try {
			model.getComplexC().setRe(n);
			view.getComplexCRe().setValue(model.getComplexC().getRe());
		} catch (NumberFormatException ex) {
			ex.printStackTrace();
			System.exit(1);
		}
	}

	public void setCorrectFormattedTextComplexCIm(double n) {
		try {
			model.getComplexC().setIm(n);
			view.getComplexCIm().setValue(model.getComplexC().getIm());
		} catch (NumberFormatException ex) {
			ex.printStackTrace();
			System.exit(1);
		}
	}

	public void resetConfiguratation() {
		// based on a julia set
		view.getTypesBox().setSelectedIndex(0);
		model.setBuilder(new Fractal.BuilderFractal());
		view.getFTx1().setValue(model.getBuilder().getX1());
		view.getFTx2().setValue(model.getBuilder().getX2());
		view.getFTy1().setValue(model.getBuilder().getY1());
		view.getFTy2().setValue(model.getBuilder().getY2());
		view.getFTzoom().setValue(model.getBuilder().getZoom());
		view.getFTiter().setValue(model.getBuilder().getIter());
		view.getFTstep().setValue(model.getBuilder().getStep());
		view.getFTpow().setValue(model.getBuilder().getPow());
		model.getComplexC().setRe(0);
		model.getComplexC().setIm(0);
		view.getComplexCRe().setValue(0);
		view.getComplexCIm().setValue(0);
	}

	public void executeDraw() {
		try {
			Fractal fract = model.getBuilder().buildFractal();
			try {
				ModelDraw modelDraw = ModelDraw.createModelDraw(fract, ComplexNumber.copyOf(model.getComplexC()));
				ViewDraw viewDraw = ViewDraw.createViewDraw(modelDraw);
				viewDraw.setVisible(true);
			} catch (IllegalArgumentException ex) { // Pb from new window
				resetConfiguratation();
			}
		} catch (IllegalArgumentException ex) { // Pb form builder
			resetConfiguratation();
		}
	}

}
