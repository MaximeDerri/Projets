import java.awt.Color;
import java.awt.Graphics;
import java.awt.event.ActionEvent;
import java.beans.PropertyChangeEvent;
import java.text.NumberFormat;

import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JFormattedTextField;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.border.BevelBorder;

public class ViewMenu extends JFrame{
	private ControllerMenu controller;
	private ModelMenu model;
	
	//select fractal type
	private final JComboBox<String> typesBox;
	
	/* ----- preparing builder ----- */
	private JPanel paneBuilder;	
	
	//L for label and FT for FormattedText
	private final JLabel Lx1;
	private final JFormattedTextField FTx1;
	private final JLabel Lx2;
	private final JFormattedTextField FTx2;
	private final JLabel Ly1;
	private final JFormattedTextField FTy1;
	private final JLabel Ly2;
	private final JFormattedTextField FTy2;
	private final JLabel Lzoom;
	private final JFormattedTextField FTzoom;
	private final JLabel Liter;
	private final JFormattedTextField FTiter;
	private final JLabel Lstep;
	private final JFormattedTextField FTstep;
	private final JLabel Lpow;
	private final JFormattedTextField FTpow;
	
	private JLabel imaginary_i;
	private JLabel complexC;
	private JFormattedTextField complexCRe;
	private JFormattedTextField complexCIm;
	/* ----- preparing builder ----- */
	
	private JButton reset;
	private JButton draw;
	
	//cases: Z(r + i) C(r + i)
	private ViewMenu(ControllerMenu c, ModelMenu m) {
		controller = c;
		model = m;
		controller.setViewMenu(this);
		
		/* ----- window  ----- */
		setLocation(100, 100);
		setSize(900, 700);
		setResizable(false);
		setTitle("Fractal Factory");
		
		if(model.getMenuBg() != null) {
			setContentPane(new MenuBackground());
		}
		
		getContentPane().setLayout(null);
		setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		/* ----- window  ----- */
		
		
		paneBuilder = new JPanel();
		paneBuilder.setLayout(null);
		paneBuilder.setOpaque(false);
		
		paneBuilder.setBounds(0, 20, 400, 400);
		add(paneBuilder);
		
		reset = new JButton("RESET");
		reset.setForeground(Color.RED);
		reset.setBackground(Color.WHITE);
		reset.setBounds(80, 350, 100, 30);
		
		reset.addActionListener((ActionEvent e) -> {
			controller.resetConfiguratation();
		});
		reset.setBorder(new BevelBorder(BevelBorder.RAISED));
		paneBuilder.add(reset);
		
		draw = new JButton("DRAW");
		draw.setForeground(Color.GREEN);
		draw.setBackground(Color.WHITE);
		draw.setBounds(215, 350, 100, 30);
		draw.addActionListener((ActionEvent e) -> {
			controller.executeDraw();
		});
		draw.setBorder(new BevelBorder(BevelBorder.RAISED));
		paneBuilder.add(draw);
		
		
		/* ----- select fractal type  ----- */
		typesBox = new JComboBox<String>(model.getTypes());
		typesBox.setBackground(Color.WHITE);
		typesBox.setForeground(Color.BLACK);
		typesBox.setBounds(30, 20, 150, 20);
		paneBuilder.add(typesBox);
		
		typesBox.addActionListener((ActionEvent e) -> {
			controller.setSelectedFractalType((String)typesBox.getSelectedItem());
		});
		/* ----- select fractal type  ----- */
		
		
		/* ----- preparing builder  ----- */
		NumberFormat n = null; //used for format
		
		//x1
		FTx1 = new JFormattedTextField(n);
		FTx1.setValue(model.getBuilder().getX1());
		FTx1.addPropertyChangeListener((PropertyChangeEvent e) -> {
			controller.setCorrectFormattedTextX1(Double.parseDouble(FTx1.getValue().toString()));
		});
		FTx1.setBorder(new BevelBorder(BevelBorder.RAISED));
		
		Lx1 = new JLabel("x1:");
		Lx1.setForeground(Color.YELLOW);
		FTx1.setBounds(80, 60, 100, 20);
		Lx1.setBounds(30, 60, 50, 20);
		
		paneBuilder.add(Lx1);
		paneBuilder.add(FTx1);
		
		
		//x2	
		FTx2 = new JFormattedTextField(n);
		FTx2.setValue(model.getBuilder().getX2());
		FTx2.addPropertyChangeListener((PropertyChangeEvent e) -> {
			controller.setCorrectFormattedTextX2(Double.parseDouble(FTx2.getValue().toString()));
		});
		FTx2.setBorder(new BevelBorder(BevelBorder.RAISED));
		
		Lx2 = new JLabel("x2:");
		Lx2.setForeground(Color.YELLOW);
		FTx2.setBounds(80, 90, 100, 20);
		Lx2.setBounds(30, 90, 50, 20);
		
		paneBuilder.add(Lx2);
		paneBuilder.add(FTx2);
		
		
		//y1
		FTy1 = new JFormattedTextField(n);
		FTy1.setValue(model.getBuilder().getY1());
		FTy1.addPropertyChangeListener((PropertyChangeEvent e) -> {
			controller.setCorrectFormattedTextY1(Double.parseDouble(FTy1.getValue().toString()));
		});
		FTy1.setBorder(new BevelBorder(BevelBorder.RAISED));
		
		Ly1 = new JLabel("y1:");
		Ly1.setForeground(Color.YELLOW);
		FTy1.setBounds(80, 120, 100, 20);
		Ly1.setBounds(30, 120, 50, 20);
		
		paneBuilder.add(Ly1);
		paneBuilder.add(FTy1);
		
		
		//y2
		FTy2 = new JFormattedTextField(n);
		FTy2.setValue(model.getBuilder().getY2());
		FTy2.addPropertyChangeListener((PropertyChangeEvent e) -> {
			controller.setCorrectFormattedTextY2(Double.parseDouble(FTy2.getValue().toString()));
		});
		FTy2.setBorder(new BevelBorder(BevelBorder.RAISED));
		
		Ly2 = new JLabel("y2:");
		Ly2.setForeground(Color.YELLOW);
		FTy2.setBounds(80, 150, 100, 20);
		Ly2.setBounds(30, 150, 50, 20);
		
		paneBuilder.add(Ly2);
		paneBuilder.add(FTy2);
		
		
		//zoom
		FTzoom = new JFormattedTextField(n);
		FTzoom.setValue(model.getBuilder().getZoom());
		FTzoom.addPropertyChangeListener((PropertyChangeEvent e) -> {
			controller.setCorrectFormattedTextZoom(Integer.parseInt(FTzoom.getValue().toString()));
		});
		FTzoom.setBorder(new BevelBorder(BevelBorder.RAISED));
		
		Lzoom = new JLabel("zoom:");
		Lzoom.setForeground(Color.YELLOW);
		FTzoom.setBounds(80, 180, 100, 20);
		Lzoom.setBounds(30, 180, 50, 20);
		
		paneBuilder.add(Lzoom);
		paneBuilder.add(FTzoom);
		
		
		//iter
		FTiter = new JFormattedTextField(n);
		FTiter.setValue(model.getBuilder().getIter());
		FTiter.addPropertyChangeListener((PropertyChangeEvent e) -> {
			controller.setCorrectFormattedTextIter(Integer.parseInt(FTiter.getValue().toString()));
		});
		FTiter.setBorder(new BevelBorder(BevelBorder.RAISED));
		
		Liter = new JLabel("iter:");
		Liter.setForeground(Color.YELLOW);
		FTiter.setBounds(80, 210, 100, 20);
		Liter.setBounds(30, 210, 50, 20);
		
		paneBuilder.add(Liter);
		paneBuilder.add(FTiter);	

		
		//step
		FTstep = new JFormattedTextField(n);
		FTstep.setValue(model.getBuilder().getStep());
		FTstep.addPropertyChangeListener((PropertyChangeEvent e) -> {
			controller.setCorrectFormattedTextStep(Double.parseDouble(FTstep.getValue().toString()));
		});
		FTstep.setBorder(new BevelBorder(BevelBorder.RAISED));
		
		Lstep = new JLabel("step:");
		Lstep.setForeground(Color.YELLOW);
		FTstep.setBounds(80, 240, 100, 20);
		Lstep.setBounds(30, 240, 50, 20);
		
		paneBuilder.add(Lstep);
		paneBuilder.add(FTstep);
		
		
		//pow
		FTpow = new JFormattedTextField(n);
		FTpow.setValue(model.getBuilder().getPow());
		FTpow.addPropertyChangeListener((PropertyChangeEvent e) -> {
			controller.setCorrectFormattedTextPow(Integer.parseInt(FTpow.getValue().toString()));
		});
		FTpow.setBorder(new BevelBorder(BevelBorder.RAISED));
		
		Lpow = new JLabel("pow:");
		Lpow.setForeground(Color.YELLOW);
		FTpow.setBounds(80, 270, 100, 20);
		Lpow.setBounds(30, 270, 50, 20);
		
		paneBuilder.add(Lpow);
		paneBuilder.add(FTpow);
		
		
		// --- complex number
		
		imaginary_i = new JLabel("+ i");
		complexC = new JLabel("C:");
		
		complexCRe = new JFormattedTextField(n);
		complexCRe.setValue(Double.valueOf(0));
		
		complexCRe.addPropertyChangeListener((PropertyChangeEvent e) -> {
			controller.setCorrectFormattedTextComplexCRe(Double.parseDouble(complexCRe.getValue().toString()));
		});
		complexCRe.setBorder(new BevelBorder(BevelBorder.RAISED));
		
		complexCIm = new JFormattedTextField(n);
		complexCIm.setValue(Double.valueOf(0));
		
		complexCIm.addPropertyChangeListener((PropertyChangeEvent e) -> {
			controller.setCorrectFormattedTextComplexCIm(Double.parseDouble(complexCIm.getValue().toString()));
		});
		complexCIm.setBorder(new BevelBorder(BevelBorder.RAISED));
		
		complexC.setBounds(30, 300, 50, 20);
		complexC.setForeground(Color.YELLOW);
		paneBuilder.add(complexC);
		complexCRe.setBounds(80, 300, 100, 20);
		paneBuilder.add(complexCRe);
		imaginary_i.setBounds(190, 300, 50, 20);
		imaginary_i.setForeground(Color.YELLOW);
		paneBuilder.add(imaginary_i);
		complexCIm.setBounds(215, 300, 100, 20);
		paneBuilder.add(complexCIm);
		/* ----- preparing builder  ----- */
		
		
	}
	
	
	
	public static ViewMenu createViewMenu() {
		ModelMenu model = ModelMenu.createModelMenu();
		ControllerMenu controller = ControllerMenu.createControllerMenu(model);
		
		return new ViewMenu(controller, model);
	}
	
	public static ViewMenu createViewMenu(ModelMenu m) {
		if(m == null) throw new IllegalArgumentException("model is not set.");
		ControllerMenu controller = ControllerMenu.createControllerMenu(m);
		
		return new ViewMenu(controller, m);
	}
	
	class MenuBackground extends JPanel {
		@Override
		public void paintComponent(Graphics g) {
			super.paintComponent(g);
			if(model.getMenuBg() != null) {
				g.drawImage(model.getMenuBg(), 0, 0, this);
			}
		}
	}
	
	public JComboBox<String> getTypesBox() {
		return typesBox;
	}

	public JFormattedTextField getFTx1() {
		return FTx1;
	}

	public JFormattedTextField getFTx2() {
		return FTx2;
	}
	
	public JFormattedTextField getFTy1() {
		return FTy1;
	}
	
	public JFormattedTextField getFTy2() {
		return FTy2;
	}
	
	public JFormattedTextField getFTzoom() {
		return FTzoom;
	}
	
	public JFormattedTextField getFTiter() {
		return FTiter;
	}
	
	public JFormattedTextField getFTstep() {
		return FTstep;
	}
	
	public JFormattedTextField getFTpow() {
		return FTpow;
	}
	
	public JFormattedTextField getComplexCRe() {
		return complexCRe;
	}
	
	public JFormattedTextField getComplexCIm() {
		return complexCIm;
	}
	
	

}
