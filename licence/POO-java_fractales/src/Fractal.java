import java.awt.Color;
import java.awt.image.BufferedImage;
import java.util.function.BiConsumer;
import java.lang.Thread;
import java.util.LinkedList;

public class Fractal {

	public static int THREAD_AMOUNT = 4;

	private LinkedList<FractalRunnable> runnableList; //only usable here to stop
	private final FractalTypes type;
	private final double x1;
	private final double x2;
	private final double y1;
	private final double y2;
	private final double step;
	private final int zoom;
	private final int iter;
	private final int pow;

	private Fractal(BuilderFractal b) {
		type = b.type;
		x1 = b.x1;
		x2 = b.x2;
		y1 = b.y1;
		y2 = b.y2;
		step = b.step;
		zoom = b.zoom;
		iter = b.iter;
		pow = b.pow;
		runnableList = new LinkedList<FractalRunnable>();
	}

	@Override
	public String toString() {
		String s = "";
		s += "Type = " + type.getName() + ".\n";
		s += "x1 = " + x1 + " | x2 = " + x2 + " | y1 = " + y1 + " | y2 = " + y2 + ".\n";
		s += "Step = " + step + ".\n";
		s += "Zoom = " + zoom + ".\n";
		s += "Iterations = " + iter + ".\n";
		s += "Pow = " + pow + ".";

		return s;
	}

	public void stopRunning() {
		if(runnableList == null) return;
		for(int i = 0; i < runnableList.size(); i++) {
			FractalRunnable t = runnableList.get(i);
			if(t ==  null) continue;
			else t.kill();
		}
		runnableList.clear();
	}

	public int divergenceInd(ComplexNumber z, ComplexNumber c, int p) {
		int i = 0;
		int radius = 2;
		ComplexNumber tmp = ComplexNumber.copyOf(z);

		while (i < iter - 1 && tmp.modulusComplexNumber() <= radius) {
			tmp = tmp.powComplexNumber(tmp, p).addComplexNumber(c);
			i++;
		}
		return i;
	}

	/* ----- First CLass Function ----- */

	// Julia Set
	public BiConsumer<ModelDraw, ViewDraw.ImageDraw> getJuliaAlgo() {
		return new BiConsumer<ModelDraw, ViewDraw.ImageDraw>() {
			@Override
			public void accept(ModelDraw model, ViewDraw.ImageDraw imagePane) {
				if(model.getYImage() < THREAD_AMOUNT) {
					JuliaRunnable jr = JuliaRunnable.getNewJuliaRunnable(model, imagePane, 0, 0, model.getXImage(), model.getYImage());
					runnableList.add(jr);
					Thread t = new Thread(jr);
					t.start();
					try {
						t.join();
					}
					catch(InterruptedException e) {
						e.printStackTrace();
					}
				}
				else {
					//preparing
					LinkedList<Thread> threadList = new LinkedList<Thread>(); //for join();
					int amountOfY = model.getYImage() / THREAD_AMOUNT;
					int r = model.getYImage() % THREAD_AMOUNT;
					
					//first thread will take model.getYImage / THREAD_AMOUNT + r
					JuliaRunnable jrFirst  = JuliaRunnable.getNewJuliaRunnable(model, imagePane, 0, 0, model.getXImage(), amountOfY + r);
					runnableList.add(jrFirst);
					threadList.add(new Thread(jrFirst));
					for(int i = 1; i < THREAD_AMOUNT; i++) {
						JuliaRunnable jr = JuliaRunnable.getNewJuliaRunnable(model, imagePane, 0, (amountOfY * i) + r, model.getXImage(), (amountOfY * (i+1)) + r);
						runnableList.add(jr);
						threadList.add(new Thread(jr));
					}
					
					//start
					for(Thread t : threadList) {
						t.start();
					}
					
					//wait until every threads are done
					for(Thread t : threadList) {
						try {
							t.join();
						}
						catch(InterruptedException e) {
							continue; //actual thread is already done, let's continue
						}
					}
				}
				runnableList.clear(); //clear

				imagePane.paintComponent(imagePane.getGraphics());
			}
		};
	}

	// Mandelbrot Set
	public BiConsumer<ModelDraw, ViewDraw.ImageDraw> getMandelbrotAlgo() {
		return new BiConsumer<ModelDraw, ViewDraw.ImageDraw>() {

			@Override
			public void accept(ModelDraw model, ViewDraw.ImageDraw imagePane) {
				if(model.getYImage() < THREAD_AMOUNT) {
					MandelbrotRunnable mr = MandelbrotRunnable.getNewMandelbrotRunnable(model, imagePane, 0, 0, model.getXImage(), model.getYImage());
					runnableList.add(mr);
					Thread t = new Thread(mr);
					t.start();
					try {
						t.join();
					}
					catch(InterruptedException e) {
						e.printStackTrace();
					}
				}
				else {
					//preparing
					LinkedList<Thread> threadList = new LinkedList<Thread>(); //for join();
					int amountOfY = model.getYImage() / THREAD_AMOUNT;
					int r = model.getYImage() % THREAD_AMOUNT;
					
					MandelbrotRunnable mrFirst  = MandelbrotRunnable.getNewMandelbrotRunnable(model, imagePane, 0, 0, model.getXImage(), amountOfY + r);
					runnableList.add(mrFirst);
					threadList.add(new Thread(mrFirst));
					for(int i = 1; i < THREAD_AMOUNT; i++) {
						MandelbrotRunnable mr = MandelbrotRunnable.getNewMandelbrotRunnable(model, imagePane, 0, (amountOfY * i) + r, model.getXImage(), (amountOfY * (i+1)) + r);
						runnableList.add(mr);
						threadList.add(new Thread(mr));
					}
					
					//start
					for(Thread t : threadList) {
						t.start();
					}
					
					//wait until every threads are done
					for(Thread t : threadList) {
						try {
							t.join();
						}
						catch(InterruptedException e) {
							continue; //actual thread is already done, let's continue
						}
					}
				}
				runnableList.clear(); //clear

				imagePane.paintComponent(imagePane.getGraphics());
			}
		};
	}

	/* ----- First CLass Function ----- */

	public static class BuilderFractal {
		private FractalTypes type = FractalTypes.JULIA;
		private double x1 = -1f;
		private double x2 = 1;
		private double y1 = -1;
		private double y2 = 1;
		private double step = 0.01d;
		private int zoom = 100;
		private int iter = 1000;
		private int pow = 2;

		public Fractal buildFractal() throws IllegalArgumentException {
			// several tests, will not necessarily stop the program but will be used to
			// detect bad configurations
			if (iter < 10)
				throw new IllegalArgumentException();
			if (zoom < 100)
				throw new IllegalArgumentException();
			if (step < 0)
				throw new IllegalArgumentException();
			if (x1 > x2)
				throw new IllegalArgumentException();
			if (y1 > y2)
				throw new IllegalArgumentException();
			if (zoom * step < 1)
				throw new IllegalArgumentException();
			if (zoom * step > 10)
				throw new IllegalArgumentException();
			if (type == null)
				throw new IllegalArgumentException();
			if (pow < 0)
				throw new IllegalArgumentException();

			return new Fractal(this);
		}

		@Override
		public String toString() {
			String s = "";
			s += "Type: " + type.getName() + ".\n";
			s += "x1: " + x1 + " | x2: " + x2 + " | y1: " + y1 + " | y2: " + y2 + ".\n";
			s += "Step: " + step + ".\n";
			s += "Zoom: " + zoom + ".\n";
			s += "Iterations: " + iter + ".";
			return s;
		}

		public BuilderFractal setX1(double x) {
			x1 = x;
			return this;
		}

		public BuilderFractal setX2(double x) {
			x2 = x;
			return this;
		}

		public BuilderFractal setY1(double y) {
			y1 = y;
			return this;
		}

		public BuilderFractal setY2(double y) {
			y2 = y;
			return this;
		}

		public BuilderFractal setStep(double st) {
			step = st;
			return this;
		}

		public BuilderFractal setZoom(int z) {
			zoom = z;
			return this;
		}

		public BuilderFractal setIter(int i) {
			iter = i;
			return this;
		}

		public BuilderFractal setType(FractalTypes n) {
			type = n;
			return this;
		}

		public BuilderFractal setPow(int i) {
			pow = i;
			return this;
		}

		public FractalTypes getType() {
			return type;
		}

		public double getX1() {
			return x1;
		}

		public double getX2() {
			return x2;
		}

		public double getY1() {
			return y1;
		}

		public double getY2() {
			return y2;
		}

		public double getStep() {
			return step;
		}

		public int getZoom() {
			return zoom;
		}

		public int getIter() {
			return iter;
		}

		public int getPow() {
			return pow;
		}

	}

	public FractalTypes getType() {
		return type;
	}

	public double getX1() {
		return x1;
	}

	public double getX2() {
		return x2;
	}

	public double getY1() {
		return y1;
	}

	public double getY2() {
		return y2;
	}

	public double getStep() {
		return step;
	}

	public int getZoom() {
		return zoom;
	}

	public int getIter() {
		return iter;
	}

	public int getPow() {
		return pow;
	}

}
