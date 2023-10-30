package src.server;

public class Passage extends Square {

    protected Passage() {
        super(' ', true);
    }

    @Override
    public String toString() {
        return String.valueOf(displayChar);
    }
}
