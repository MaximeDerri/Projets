import javax.management.RuntimeErrorException;
import javax.swing.JFrame;

public class Launcher {

	public static void main(String[] args) {
		int length_args = args.length;
		FractalTypes type_of_fractal;
		Fractal.BuilderFractal fractal_builder = new Fractal.BuilderFractal();
		try {
			if (length_args == 0) {
				ViewMenu view = ViewMenu.createViewMenu();
				view.setVisible(true);
			} else if (length_args >= 1 && length_args <= 2) {
				switch (args[0]) {
					case "-help":
						commandHelp();
						break;
					case "-julia":
						type_of_fractal = FractalTypes.JULIA;
						launch_draw_panel(type_of_fractal, fractal_builder, args, 10);
						break;
					case "-mandelbrot":
						type_of_fractal = FractalTypes.MANDELBROT;
						launch_draw_panel(type_of_fractal, fractal_builder, args, 8);
						break;
					default:
						throw new IllegalArgumentException("'" + args[0] + "' is not a valid command");
				}
			} else {
				throw new IllegalArgumentException(length_args + " arguments given when between 0 and 2 were expected");
			}
		} catch (IllegalArgumentException iae) {
			System.out.println("\n" + iae);
			commandHelp();
		} catch (RuntimeErrorException ree) {
			System.out.println(ree);
		}
	}

	private static void launch_draw_panel(FractalTypes type_of_fractal, Fractal.BuilderFractal fractal_builder,
			String[] args, int num_of_param_of_set) throws IllegalArgumentException {
		if (args.length == 1) {
			fractal_builder.setType(type_of_fractal);
			drawInputFractal(fractal_builder.buildFractal(), 0.0, 0.0);
		} else {
			Double d1;
			Double d2;
			if (validateCommandInput(type_of_fractal.getName(), args, num_of_param_of_set)) {
				String[] split_values = ((args[1].split("#")[1]).split(":"));
				if (type_of_fractal.getName().equals("Mandelbrot Set")) {
					d1 = 0.0;
					d2 = 0.0;
				} else {
					d1 = Double.valueOf(split_values[8]);
					d2 = Double.valueOf(split_values[9]);
				}
				assign_fractal_values(fractal_builder, split_values, type_of_fractal);
				drawInputFractal(fractal_builder.buildFractal(), d1, d2);
			}
		}

	}

	private static void drawInputFractal(Fractal f, Double d1, Double d2) {
		try {
			ModelDraw modelDraw = ModelDraw.createModelDraw(f,
					ComplexNumber.copyOf(ComplexNumber.createComplexNumber(
							d1, d2)));
			ViewDraw viewDraw = ViewDraw.createViewDraw(modelDraw);
			viewDraw.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
			viewDraw.setVisible(true);
		} catch (IllegalArgumentException ex) {
			throw new RuntimeErrorException(null, "Problem with Draw window (command line)");
		}

	}

	private static void assign_fractal_values(Fractal.BuilderFractal f, String[] split_values, FractalTypes ft) {
		f.setType(ft);
		f.setX1(Double.valueOf(split_values[0]));
		f.setX2(Double.valueOf(split_values[1]));
		f.setY1(Double.valueOf(split_values[2]));
		f.setY2(Double.valueOf(split_values[3]));
		f.setZoom(Integer.valueOf(split_values[4]));
		f.setIter(Integer.valueOf(split_values[5]));
		f.setStep(Double.valueOf(split_values[6]));
		f.setPow(Integer.valueOf(split_values[7]));
	}

	private static boolean validateCommandInput(String setName, String[] args, int num_of_param_of_set)
			throws IllegalArgumentException {
		int length_args = args.length;
		String[] split_values;
		String[] arg_var_names = { "x1", "x2", "y1", "y2", "zoom", "iter", "step", "pow", "real_C", "imag_C" };
		if (length_args == 1) {
			System.out.println("Using default values for " + setName);
			return true;
		} else if (length_args == 2) {
			try {
				split_values = ((args[1].split("#")[1]).split(":"));
			} catch (Exception e) {
				throw new IllegalArgumentException(
						"** Unexpected structure for value parameter found '" + args[1] + "' **");
			}
			if (split_values.length != num_of_param_of_set) {
				throw new IllegalArgumentException(
						"** Invalid number of arguments passed " + num_of_param_of_set + " expected ("
								+ split_values.length + " found) **");
			} else {
				System.out.println("Chosen values for drawing of " + setName + ":");
				for (int i = 0; i < split_values.length; i++) {
					if (i == 4 || i == 5 || i == 7) {
						if (!isInteger(split_values[i])) {
							throw new IllegalArgumentException(
									"** Unexpected type for variable '" + arg_var_names[i] + "' (int expected '"
											+ split_values[i] + "' given) ***");
						}
					} else {
						if (!isDouble(split_values[i])) {
							throw new IllegalArgumentException(
									"** Unexpected type for variable '" + arg_var_names[i]
											+ "' (double expected '" + split_values[i] + "' given) **");
						}
					}
					System.out.println(arg_var_names[i] + "\t -> " + split_values[i]);
				}
				return true;
			}
		}
		return false;
	}

	private static boolean isDouble(String s) {
		try {
			Double.valueOf(s);
			return true;
		} catch (Exception e) {
			return false;
		}
	}

	private static boolean isInteger(String s) {
		try {
			Integer.valueOf(s);
			return true;
		} catch (Exception e) {
			return false;
		}
	}

	private static void commandHelp() {
		System.out.println("\nCommands:\n");
		System.out.println("'-help'\t\t-> For help with commands");
		System.out.println("''\t\t-> No commands for the graphical user interface");
		System.out.println("'-julia'\t-> For generating the Julia set with default values");
		System.out.println("'-mandelbrot'\t-> For generating the Mandelbrot set with default values");
		System.out.println("\nTo choose values for the set generation parse a commnad as follows (fill in the {})");
		System.out.println(
				"./launch -{set} values#{x1}:{x2}:{y1}:{y2}:{zoom}:{iter}:{step}:{pow}:{real_component_of_C}:{imaginary_component_of_C}");
		System.out
				.println(
						"**NOTE** Components of C do not effect Mandelbrot and are not required for it's generation\n");
		System.out.println("EXAMPLES:\n./launch -julia values#-1.0:1.0:-1.0:1.0:400:1000:0.01:2:0.234:0.1");
		System.out.println("./launch -mandelbrot values#-1.0:1.0:-1.0:1.0:400:1000:0.01:2");
		System.out.println("");
	}

}
