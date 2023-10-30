import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.function.BiConsumer;
import java.lang.Thread;

import javax.imageio.ImageIO;
import javax.swing.JFileChooser;
import javax.swing.filechooser.FileSystemView;

public class ControllerDraw {
	private ViewDraw view;
	private ModelDraw model;

	private ControllerDraw(ModelDraw m) {
		model = m;
	}

	public static ControllerDraw createControllerDraw(ModelDraw m) throws IllegalArgumentException {
		if (m == null)
			throw new IllegalArgumentException("Problem with ModelDraw on createControllerDraw()");
		return new ControllerDraw(m);
	}

	public void setViewDraw(ViewDraw v) throws IllegalArgumentException {
		if (v == null)
			throw new IllegalArgumentException("Problem with ViewDraw on ControllerDraw - setViewDraw()");
		view = v;
	}

	public void drawFractal() throws IllegalArgumentException {
		view.setEnableStart(false);
		BiConsumer<ModelDraw, ViewDraw.ImageDraw> cons;

		if (FractalTypes.JULIA == model.getFractal().getType()) {
			cons = model.getFractal().getJuliaAlgo();
		} else if (FractalTypes.MANDELBROT == model.getFractal().getType()) {
			cons = model.getFractal().getMandelbrotAlgo();
		} else {
			throw new IllegalArgumentException("Problem with fractal type in ControllerDraw - drawFractal()");
		}

		new Thread(() -> {
			view.setEnableStop(true);
			cons.accept(model, view.getImagePane());
			view.setEnableSaveTo(true);
			view.setEnableSave(true);
			view.setEnableStop(false);
		}).start();

	}

	public void stopFractal() {
		model.getFractal().stopRunning();
		view.setEnableSave(false);
		view.setEnableSaveTo(false);
	}

	public void saveFractal(boolean customLocation) {
		view.setEnableSave(false);
		view.setEnableSaveTo(false);
		Fractal f = model.getFractal();

		String fileName = f.getType() + "_" + f.getX1() + "_" + f.getX2() + "_" + f.getY1() + "_" + f.getY2() + "_"
				+ f.getZoom() + "_" + f.getIter() + "_" + f.getStep() + "_" + f.getPow();
		fileName += "_" + model.getC().getRe() + "+i" + model.getC().getIm();

		File directory = new File(".");
		PrintWriter descriptionFractal = null;
		String path = "";
		try {
			if (!customLocation) {
				path = directory.getCanonicalPath() + File.separator + "fractals" + File.separator;
			} else {
				JFileChooser directoryChooser = new JFileChooser(FileSystemView.getFileSystemView().getHomeDirectory());
				directoryChooser.setDialogTitle("Select a directory to save the fractal: ");
				directoryChooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
				int chosenDirectory = directoryChooser.showOpenDialog(null);
				if (chosenDirectory == JFileChooser.APPROVE_OPTION) {
					if (directoryChooser.getSelectedFile().isDirectory()) {
						path = directoryChooser.getSelectedFile().toString() + File.separator;
					}
				}
			}
			ImageIO.write(view.getImagePane().getImage(), "PNG", new File(path + fileName + ".png"));
			descriptionFractal = new PrintWriter(new File(path + fileName + ".txt"));
			String tmp = f.toString() + "\n\nBegin with:\nComplex Z = " + model.getZ().toString()
					+ ".\nComplex C = "
					+ model.getC().toString() + ".";
			descriptionFractal.println(tmp);
			descriptionFractal.flush();
		} catch (IOException e) {
			System.out.println("Problem with saveFractal - not saved");
		} finally {
			descriptionFractal.close();
		}

	}

}
