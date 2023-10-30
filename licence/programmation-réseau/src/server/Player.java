package src.server;

class Player extends Square {

    private String id;
    private String udpPort;
    private int radarCount = 6;
    private int bombCount = 6;

    Player(String i, String u) {
        super('P', false);
        if (i.length() != 8) {
            throw new IllegalArgumentException("Player id must be 8 characters long");
        }
        if (u.length() != 4) {
            throw new IllegalArgumentException("Player's UDP port must be 4 characters long");
        }
        id = i;
        udpPort = u;
    }

    protected String getID() {
        return id;
    }

    protected String getUDP() {
        return udpPort;
    }

    protected boolean useBomb() {
        if (bombCount > 0) {
            bombCount -= 1;
            return true;
        }
        return false;
    }

    protected boolean useRadar() {
        if (radarCount > 0) {
            radarCount -= 1;
            return true;
        }
        return false;
    }

    @Override
    public String toString() {
        return Common.RED_BOLD + displayChar + Common.RESET;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (this.getClass() != obj.getClass())
            return false;
        Player p = (Player) obj;
        return this.id.equals(p.id);
    }

    @Override
    public int hashCode() {
        int result = 17;
        result = 37 * result + id.hashCode();
        return result;
    }
}
