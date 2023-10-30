package src.server;

public class Wall extends Square {

    protected Wall() {
        super('#', false);
    }

    @Override
    public String toString() {
        return Common.GREEN_BOLD + displayChar + Common.RESET;
    }

}
