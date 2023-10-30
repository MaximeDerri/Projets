import java.lang.Runnable;
import java.awt.Color;
import java.awt.image.BufferedImage;

public class MandelbrotRunnable extends FractalRunnable {
    private ModelDraw model;
    private ViewDraw.ImageDraw imagePane;
    private int indX1, indY1, indX2, indY2;

    private MandelbrotRunnable(ModelDraw m, ViewDraw.ImageDraw i, int x1, int y1, int x2, int y2) {
        model = m;
        imagePane = i;
        indX1 = x1;
        indX2 = x2;
        indY1 = y1;
        indY2 = y2;
    }

    public static MandelbrotRunnable getNewMandelbrotRunnable(ModelDraw m, ViewDraw.ImageDraw i, int x1, int y1, int x2, int y2) throws IllegalArgumentException {
        if(m == null || i == null || x1 > x2 || y1 > y2) throw new IllegalArgumentException("Unable to return a new JuliaRunnable - drawing is impossible.");
        return new MandelbrotRunnable(m, i, x1, y1, x2, y2);
    }

    @Override
    public void run() {
        BufferedImage image = imagePane.getImage();
        ComplexNumber z = ComplexNumber.createComplexNumber(0, 0);
        ComplexNumber c = ComplexNumber.createComplexNumber(0, 0);
        int ind = 0;
        int rgb = model.getRgb();

        for (int i = indX1; i < indX2; i++) {
            for (int j = indY1; j < indY2; j++) {
                if(!getRunning()) return; //stop the thread
                
                c.setRe(((double) i / model.getFractal().getZoom()) + model.getFractal().getX1());
                c.setIm(((double) j / model.getFractal().getZoom()) + model.getFractal().getY1());
                ind = model.getFractal().divergenceInd(z, c, model.getFractal().getPow());
                if (ind == model.getFractal().getIter() - 1)
                    image.setRGB(i, model.getYImage() - j - 1, Color.BLACK.getRGB());
                else
                    image.setRGB(i, model.getYImage() - j - 1, rgb * ind);
                    
                }
        }
    }

}