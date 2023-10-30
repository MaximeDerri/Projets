package src.server;

public class Square {

    public final char displayChar;
    private boolean freeSpace;

    protected Square(char c, boolean f) {
        displayChar = c;
        freeSpace = f;
    }

    protected boolean isFreeSpace() {
        return freeSpace;
    }

    @Override
    public String toString() {
        return Common.WHITE_BACKGROUND + Common.PURPLE_BOLD + "S" + Common.RESET;
    }

}
