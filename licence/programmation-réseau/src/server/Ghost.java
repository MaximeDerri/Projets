package src.server;

public class Ghost extends Square {

    protected Ghost() {
        super('G', false);
    }

    @Override
    public String toString() {
        return Common.BRIGHT_BLUE_BOLD + displayChar + Common.RESET;
    }

}
