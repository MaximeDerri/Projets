package src.server;

import java.util.*;
import java.awt.*;

class Labyrinth {

    private final int width;
    private final int height;
    private Square[][] grid;
    private HashMap<Player, Point> playerCoords = new HashMap<>();
    private HashMap<Point, Ghost> ghostCoords = new HashMap<>();
    private int labyrinthXsize;
    private int labyrinthYsize;
    private int perimeter;
    private int topBorder;
    private int rightBorder;
    private int bottomBorder;
    private int leftBorder;

    protected Labyrinth(int x, int y) throws IllegalArgumentException {
        if (y >= 1000 || x >= 1000) {
            throw new IllegalArgumentException();
        }
        width = x + 1;
        height = y + 1;
        labyrinthYsize = (height - 2);
        labyrinthXsize = (width - 2);
        perimeter = ((labyrinthYsize) * 2) + ((labyrinthXsize) * 2) - 4;
        topBorder = labyrinthXsize - 1;
        rightBorder = topBorder + labyrinthYsize - 1;
        bottomBorder = rightBorder + labyrinthXsize - 1;
        leftBorder = perimeter - 1;
        grid = new Square[height][width];
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {
                grid[j][i] = new Wall();
            }
        }
        constructLabyrinth();
    }

    private void constructLabyrinth() {
        int y = Common.getRandomNumber(1, height - 1);
        int x = Common.getRandomNumber(1, width - 1);
        if (x % 2 == 0) {
            x -= 1;
        }
        if (y % 2 == 0) {
            y -= 1;
        }
        LinkedList<int[]> frontiers = new LinkedList<>();
        frontiers.add(new int[] { y, x, y, x });
        while (!frontiers.isEmpty()) {
            int[] neighbours = frontiers.remove(Common.getRandomNumber(0, frontiers.size()));
            y = neighbours[2];
            x = neighbours[3];
            if (!grid[y][x].isFreeSpace()) {
                grid[neighbours[0]][neighbours[1]] = new Passage();
                grid[y][x] = new Passage();
                if (y >= 3 && !grid[y - 2][x].isFreeSpace())
                    frontiers.add(new int[] { y - 1, x, y - 2, x });
                if (y < height - 3 && !grid[y + 2][x].isFreeSpace())
                    frontiers.add(new int[] { y + 1, x, y + 2, x });
                if (x >= 3 && !grid[y][x - 2].isFreeSpace())
                    frontiers.add(new int[] { y, x - 1, y, x - 2 });
                if (x < width - 3 && !grid[y][x + 2].isFreeSpace())
                    frontiers.add(new int[] { y, x + 1, y, x + 2 });
            }
        }
    }

    @Override
    public String toString() {
        StringBuilder s = new StringBuilder();
        s.append("\n");
        for (Square[] line : grid) {
            for (Square t : line) {
                s.append(t.toString() + " ");
            }
            s.append("\n");
        }
        return s.toString();
    }

    public String toFile() {
        StringBuilder s = new StringBuilder();
        for (Square[] line : grid) {
            for (Square t : line) {
                s.append(t.displayChar + " ");
            }
            s.append("\n");
        }
        return s.toString();
    }

    public String[] toCASESformat() {
        ArrayList<String> als = new ArrayList<>();
        StringBuilder s = new StringBuilder();
        int n = 0;
        int m = 0;
        for (Square[] line : grid) {
            if (n < 255) {
                for (Square t : line) {
                    if (m < 255) {
                        s.append(t.displayChar);
                        m++;
                    }
                }
                n++;
                als.add(s.toString());
                s = new StringBuilder();
                m = 0;
            }
        }
        return als.toArray(new String[0]);
    }

    protected Set<Point> getGhostPoints() {
        return ghostCoords.keySet();
    }

    protected void placeGhosts() {
        int n = Math.max(width, height);
        int placed = 0;
        Ghost g;
        while (placed < n) {
            int y = Common.getRandomNumber(1, height - 1);
            int x = Common.getRandomNumber(1, width - 1);
            if (grid[y][x].isFreeSpace()) {
                g = new Ghost();
                grid[y][x] = g;
                ghostCoords.put(new Point(x, y), g);
                placed++;
            }
        }
    }

    protected void moveGhosts(int n) {
        for (Point p : ghostCoords.keySet()) {
            grid[(int) p.getY()][(int) p.getX()] = new Passage();
        }
        ghostCoords = new HashMap<>();
        int placed = 0;
        Ghost g;
        while (placed < n) {
            int y = Common.getRandomNumber(1, height - 1);
            int x = Common.getRandomNumber(1, width - 1);
            if (grid[y][x].isFreeSpace()) {
                g = new Ghost();
                grid[y][x] = g;
                ghostCoords.put(new Point(x, y), g);
                placed++;
            }
        }
    }

    protected int getGhostNum() {
        return ghostCoords.size();
    }

    protected void placePlayers(Player[] playerList) throws IllegalArgumentException, IndexOutOfBoundsException {
        int n = playerList.length;
        if ((perimeter / 5) < n) {
            throw new IllegalArgumentException("Too many players for this size of the labyrinth");
        }
        int counter = 0;
        float spacing;
        for (int i = 0; i < n; i++) {
            if (counter <= topBorder) {
                grid[1][counter + 1] = playerList[i];
                playerCoords.put(playerList[i], new Point(counter + 1, 1));
            } else if (counter <= rightBorder) {
                grid[counter - topBorder + 1][labyrinthXsize] = playerList[i];
                playerCoords.put(playerList[i], new Point(labyrinthXsize, counter - topBorder + 1));
            } else if (counter <= bottomBorder) {
                grid[labyrinthYsize][labyrinthXsize - (counter - rightBorder)] = playerList[i];
                playerCoords.put(playerList[i], new Point(labyrinthXsize - (counter - rightBorder), labyrinthYsize));
            } else if (counter <= leftBorder) {
                grid[labyrinthYsize - (counter - bottomBorder)][1] = playerList[i];
                playerCoords.put(playerList[i], new Point(1, labyrinthYsize - (counter - bottomBorder)));
            } else {
                throw new IndexOutOfBoundsException(
                        "Player is trying to be placed at index:" + counter + " which is outside of the labyrinth");
            }
            spacing = ((float) perimeter - counter) / ((float) (n - i));
            counter = (int) (counter + spacing);
        }
    }

    protected int getXofPlayer(Player p) {
        return (int) playerCoords.get(p).getX();
    }

    protected int getYofPlayer(Player p) {
        return (int) playerCoords.get(p).getY();
    }

    protected void removePlayer(Player p) {
        int x = (int) playerCoords.get(p).getX();
        int y = (int) playerCoords.get(p).getY();
        grid[y][x] = new Passage();
        playerCoords.remove(p);
    }

    protected void useBomb(Player p) {
        int x = (int) playerCoords.get(p).getX();
        int y = (int) playerCoords.get(p).getY();
        int[] possibleX = { x - 1, x, x + 1 };
        int[] possibleY = { y - 1, y, y + 1 };
        for (int ix : possibleX) {
            if (ix != width - 1 && ix > 0) {
                for (int iy : possibleY) {
                    if ((ix != x || iy != y) && (grid[iy][ix].displayChar != 'G' && grid[iy][ix].displayChar != 'P')
                            && (iy != height - 1 && iy > 0)) {
                        grid[iy][ix] = new Passage();
                    }
                }
            }
        }
    }

    protected Boolean moveRight(Player p) {
        Point point = playerCoords.get(p);
        int x = (int) point.getX();
        int y = (int) point.getY();
        char c = grid[y][x + 1].displayChar;
        boolean foundGhost = false;
        if (c == '#' || c == 'P') {
            return null;
        } else if (c == 'G') {
            ghostCoords.remove(new Point(x + 1, y));
            foundGhost = true;
        }
        playerCoords.replace(p, new Point(x + 1, y));
        grid[y][x] = new Passage();
        grid[y][x + 1] = p;
        return foundGhost;
    }

    protected Boolean moveLeft(Player p) {
        Point point = playerCoords.get(p);
        int x = (int) point.getX();
        int y = (int) point.getY();
        char c = grid[y][x - 1].displayChar;
        boolean foundGhost = false;
        if (c == '#' || c == 'P') {
            return null;
        } else if (c == 'G') {
            ghostCoords.remove(new Point(x - 1, y));
            foundGhost = true;
        }
        playerCoords.replace(p, new Point(x - 1, y));
        grid[y][x] = new Passage();
        grid[y][x - 1] = p;
        return foundGhost;
    }

    protected Boolean moveUp(Player p) {
        Point point = playerCoords.get(p);
        int x = (int) point.getX();
        int y = (int) point.getY();
        char c = grid[y - 1][x].displayChar;
        boolean foundGhost = false;
        if (c == '#' || c == 'P') {
            return null;
        } else if (c == 'G') {
            ghostCoords.remove(new Point(x, y - 1));
            foundGhost = true;
        }
        playerCoords.replace(p, new Point(x, y - 1));
        grid[y][x] = new Passage();
        grid[y - 1][x] = p;
        return foundGhost;
    }

    protected Boolean moveDown(Player p) {
        Point point = playerCoords.get(p);
        int x = (int) point.getX();
        int y = (int) point.getY();
        char c = grid[y + 1][x].displayChar;
        boolean foundGhost = false;
        if (c == '#' || c == 'P') {
            return null;
        } else if (c == 'G') {
            ghostCoords.remove(new Point(x, y + 1));
            foundGhost = true;
        }
        playerCoords.replace(p, new Point(x, y + 1));
        grid[y][x] = new Passage();
        grid[y + 1][x] = p;
        return foundGhost;
    }
}
