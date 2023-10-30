import java.lang.Runnable;

public abstract class FractalRunnable implements Runnable {
    private volatile boolean running;

    public FractalRunnable() {
        super();
        running = true;
    }

    @Override
    public void run() {
        throw new UnsupportedOperationException("run need to be override");
    }

    public void kill() {
        running = false;
    }

    public boolean getRunning() {
        return running;
    }

}