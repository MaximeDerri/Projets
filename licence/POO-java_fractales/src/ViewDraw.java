import java.awt.Color;
import java.awt.Dimension;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.image.BufferedImage;

import javax.swing.Box;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JMenuBar;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.border.Border;

public class ViewDraw extends JFrame {
	private ControllerDraw controller;
	private ModelDraw model;
	private ImageDraw imagePane;
	private JScrollPane scrollPane;
	private JButton start;
	private JButton stop;
	private JButton save;
	private JButton saveTo;
	private JMenuBar menuBar;

	private ViewDraw(ControllerDraw c, ModelDraw m) throws IllegalArgumentException {
		controller = c;
		model = m;
		controller.setViewDraw(this);

		setSize(850, 900);
		setTitle("Fractal - " + model.getFractal().getType().getName());
		getContentPane().setLayout(null);
		getContentPane().setBackground(Color.WHITE);

		// to stop all the fractal's running threads...
		this.addWindowListener(new WindowAdapter() {
			@Override
			public void windowClosing(WindowEvent e) {
				controller.stopFractal();
				e.getWindow().dispose();
			}
		});

		imagePane = new ImageDraw();

		// scrollPane
		scrollPane = new JScrollPane();
		scrollPane.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_ALWAYS);
		scrollPane.setHorizontalScrollBarPolicy(JScrollPane.HORIZONTAL_SCROLLBAR_ALWAYS);
		scrollPane.getViewport().add(imagePane);
		scrollPane.setBounds(0, 0, 800, 800);
		scrollPane.setBackground(Color.WHITE);
		add(scrollPane);

		menuBar = new JMenuBar();

		start = new JButton("START");
		start.setForeground(Color.GREEN);
		start.setBackground(Color.WHITE);

		start.addActionListener((ActionEvent e) -> {
			controller.drawFractal();
		});
		menuBar.add(start);

		stop = new JButton("STOP");
		stop.setForeground(Color.RED);
		stop.setBackground(Color.WHITE);
		stop.setEnabled(false);

		stop.addActionListener((ActionEvent e) -> {
			stop.setEnabled(false); // usable only one time, after start button is pressed
			controller.stopFractal();
		});
		menuBar.add(stop);

		menuBar.add(Box.createHorizontalGlue());

		save = new JButton("SAVE");
		save.setForeground(Color.BLUE);
		save.setBackground(Color.WHITE);
		save.setEnabled(false); // only activate after start was finished

		save.addActionListener((ActionEvent e) -> {
			controller.saveFractal(false);
		});

		menuBar.add(save);

		saveTo = new JButton("SAVE TO");
		saveTo.setForeground(Color.ORANGE);
		saveTo.setBackground(Color.WHITE);
		saveTo.setEnabled(false); // only activate after start was finished

		saveTo.addActionListener((ActionEvent e) -> {
			controller.saveFractal(true);
		});

		menuBar.add(saveTo);

		setJMenuBar(menuBar);
	}

	public static ViewDraw createViewDraw(ModelDraw m) throws IllegalArgumentException {
		if (m == null)
			throw new IllegalArgumentException("Problem with ModelDraw on ViewDraw - createViewDraw()");
		ControllerDraw c = ControllerDraw.createControllerDraw(m);
		return new ViewDraw(c, m);
	}

	public class ImageDraw extends JPanel {
		private BufferedImage imageDraw;

		private ImageDraw() {
			try {
				imageDraw = new BufferedImage(model.getXImage(), model.getYImage(), BufferedImage.TYPE_INT_RGB);
				setPreferredSize(new Dimension(model.getXImage(), model.getYImage()));
				Graphics g = imageDraw.getGraphics();
				g.setColor(Color.WHITE);
				g.fillRect(0, 0, model.getXImage(), model.getYImage());
				setBackground(Color.white);
			} catch (OutOfMemoryError e) {
				e.printStackTrace();
				System.exit(1);
			}
		}

		@Override
		public void paintComponent(Graphics g) {
			super.paintComponent(g);
			int x = (getWidth() - imageDraw.getWidth(null)) / 2;
			int y = (getHeight() - imageDraw.getHeight(null)) / 2;
			g.drawImage(imageDraw, x, y, null);
		}

		public BufferedImage getImage() {
			return imageDraw;
		}
	}

	public ImageDraw getImagePane() {
		return imagePane;
	}

	public void setEnableStart(boolean b) {
		start.setEnabled(b);
	}

	public void setEnableStop(boolean b) {
		stop.setEnabled(b);
	}

	public void setEnableSave(boolean b) {
		save.setEnabled(b);
	}

	public void setEnableSaveTo(boolean b) {
		saveTo.setEnabled(b);
	}

}
