package src.server;

import java.util.*;
import java.nio.*;
import java.nio.channels.*;
import java.io.*;
import java.net.*;
import src.server.Common.*;
import java.time.Instant;
import java.awt.*;

class LobbyService implements Runnable {

    private Labyrinth maze;
    private HashMap<Player, Integer> scoreboard;
    private Status status;
    private Selector playerSelector;
    private final Integer lobbyIndex;
    private final String SERVICE_NAME = "LobbyService";
    private final String multicastAddress;
    private final String multicastPort;
    private long lastGhostMove;

    enum Status {
        WAITING, FULL, INGAME, FINISHED
    }

    LobbyService(String a, String p, Integer i) throws IOException {
        status = Status.WAITING;
        scoreboard = new HashMap<>();
        lobbyIndex = i;
        playerSelector = Selector.open();
        multicastAddress = a;
        multicastPort = p;
    }

    @Override
    @SuppressWarnings("java:S2189")
    public void run() {
        try {
            while (true) {
                playerSelector.selectNow();
                Set<SelectionKey> selectedKeys = playerSelector.selectedKeys();
                Iterator<SelectionKey> iter = selectedKeys.iterator();
                while (iter.hasNext()) {
                    SelectionKey key = iter.next();
                    if (key.isReadable()) {
                        if (scoreboard.get(key.attachment()) == -1) {
                            beforeStartParser(key);
                        } else if (status != Status.WAITING) {
                            afterStartParser(key);
                        }
                    }
                }
                if (status == Status.INGAME) {
                    long now = Instant.now().toEpochMilli();
                    if (now - lastGhostMove > 45000) {
                        lastGhostMove = now;
                        maze.moveGhosts(maze.getGhostNum());
                        Set<Point> ghostPoints = maze.getGhostPoints();
                        for (Point point : ghostPoints) {
                            String x = String.valueOf((int) point.getX());
                            String y = String.valueOf((int) point.getY());
                            x = "0".repeat(3 - x.length()) + x;
                            y = "0".repeat(3 - y.length()) + y;
                            sendMulticastMessage(ProtocolMessages.GHOST + " " + x + " " + y + ProtocolMessages.ENDUDP);
                        }
                        Launcher.logger.updateLabyrinth(multicastAddress, multicastPort, maze);
                    }
                }
                if (status == Status.FINISHED && playerSelector.keys().isEmpty()) {
                    status = Status.WAITING;
                    scoreboard = new HashMap<>();
                }
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
        }
    }

    private void beforeStartParser(SelectionKey key) throws IOException {
        SocketChannel player = (SocketChannel) key.channel();
        ByteBuffer bbuf = ByteBuffer.allocate(100);
        int sizeRec;
        if ((sizeRec = player.read(bbuf)) == -1) {
            disconnectPlayer("Has disconnected from " + SERVICE_NAME + " before game start", player, key);
        }
        if (sizeRec > 0) {
            String receivedMessage = Common.stringOfByteMessage(bbuf.array());
            Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(player), receivedMessage, false);
            String lastThreeChars = receivedMessage.substring(receivedMessage.length() - 3);
            if (lastThreeChars.equals(ProtocolMessages.END)) {
                String[] rcvMessageSplit = receivedMessage.substring(0, receivedMessage.length() - 3).split(" ");
                switch (rcvMessageSplit[0]) {
                    case ProtocolMessages.LISTq:
                        QueueService.sendLISTqMessages(player, key, rcvMessageSplit, SERVICE_NAME);
                        break;
                    case ProtocolMessages.GAMEq:
                        QueueService.sendGAMEqMessages(player, key, SERVICE_NAME);
                        break;
                    case ProtocolMessages.SIZEq:
                        QueueService.sendSIZEqMessages(player, key, rcvMessageSplit, SERVICE_NAME);
                        break;
                    case ProtocolMessages.UNREG:
                        sendUNREGResponse(player, key);
                        break;
                    case ProtocolMessages.REGIS:
                        if (player.write(Common.regnoMessageByteArray(player).flip()) == 0) {
                            disconnectPlayer("Failed to send REGNO message in " + SERVICE_NAME, player, key);
                        }
                        break;
                    case ProtocolMessages.STARq:
                        QueueService.sendSTARqResponse(Integer.valueOf(rcvMessageSplit[1]), player, key, SERVICE_NAME);
                        ;
                        break;
                    case ProtocolMessages.START:
                        scoreboard.replace((Player) key.attachment(), 0);
                        if (allReady()) {
                            startGame();
                        }
                        break;
                    default:
                        disconnectPlayer(
                                "Unrecognised command received in " + SERVICE_NAME + "pre game parser: "
                                        + receivedMessage,
                                player, key);
                }
            } else {
                disconnectPlayer("Parsed command lacks the correct termination sequence in " + SERVICE_NAME
                        + " parser:" + receivedMessage, player, key);
            }
        }
    }

    private void afterStartParser(SelectionKey key) throws IOException {
        SocketChannel player = (SocketChannel) key.channel();
        ByteBuffer bbuf = ByteBuffer.allocate(100);
        int sizeRec;
        if ((sizeRec = player.read(bbuf)) == -1) {
            disconnectPlayer("Has disconnected from " + SERVICE_NAME + " mid-game", player, key);
        }
        if (sizeRec > 0 && status != Status.FINISHED) {
            String receivedMessage = Common.stringOfByteMessage(bbuf.array());
            Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(player), receivedMessage, false);
            String lastThreeChars = receivedMessage.substring(receivedMessage.length() - 3);
            if (lastThreeChars.equals(ProtocolMessages.END)) {
                String[] rcvMessageSplit = receivedMessage.substring(0, receivedMessage.length() - 3).split(" ");
                switch (rcvMessageSplit[0]) {
                    case ProtocolMessages.UPMOV:
                        sendMOVresponse(Integer.valueOf(rcvMessageSplit[1]), "up", player, key);
                        Launcher.logger.updateLabyrinth(multicastAddress, multicastPort, maze);
                        break;
                    case ProtocolMessages.DOMOV:
                        sendMOVresponse(Integer.valueOf(rcvMessageSplit[1]), "down", player, key);
                        Launcher.logger.updateLabyrinth(multicastAddress, multicastPort, maze);
                        break;
                    case ProtocolMessages.LEMOV:
                        sendMOVresponse(Integer.valueOf(rcvMessageSplit[1]), "left", player, key);
                        Launcher.logger.updateLabyrinth(multicastAddress, multicastPort, maze);
                        break;
                    case ProtocolMessages.RIMOV:
                        sendMOVresponse(Integer.valueOf(rcvMessageSplit[1]), "right", player, key);
                        Launcher.logger.updateLabyrinth(multicastAddress, multicastPort, maze);
                        break;
                    case ProtocolMessages.IQUIT:
                        sendGOBYE(player, key);
                        break;
                    case ProtocolMessages.GLISq:
                        sendGLISqresponse(player, key);
                        break;
                    case ProtocolMessages.MALLq:
                        sendMALLqresponse(Common.stringOfSentMessage(bbuf.array(), 6), player, key);
                        break;
                    case ProtocolMessages.SENDq:
                        sendSENDqresponse(rcvMessageSplit[1], Common.stringOfSentMessage(bbuf.array(), 15), player,
                                key);
                        break;
                    case ProtocolMessages.EXPLq:
                        sendEXPLqresponse(player, key);
                        Launcher.logger.updateLabyrinth(multicastAddress, multicastPort, maze);
                        break;
                    case ProtocolMessages.RADAq:
                        sendRADAqresponse(player, key);
                        break;
                    default:
                        disconnectPlayer("Unrecognised command received in " + SERVICE_NAME + " mid-game parser: "
                                + receivedMessage, player, key);
                }
            } else {
                disconnectPlayer("Parsed command lacks the correct termination sequence in " + SERVICE_NAME
                        + " parser:" + receivedMessage, player, key);
            }
        } else if (sizeRec > 0 && status == Status.FINISHED) {
            sendGOBYE(player, key);
        }
    }

    private void disconnectPlayer(String s, SocketChannel sc, SelectionKey key) throws IOException {
        scoreboard.remove(key.attachment());
        if (status == Status.INGAME) {
            maze.removePlayer((Player) key.attachment());
        }
        Common.protocolViolation(s, sc, key);
        checkForAtLeastOnePlayer();
    }

    protected boolean isWaiting() {
        return status == Status.WAITING;
    }

    protected HashMap<Player, Integer> getScoreboard() {
        return scoreboard;
    }

    protected String getMulticastAddress() {
        return multicastAddress;
    }

    protected String getMulticastPort() {
        return multicastPort;
    }

    protected int playerCount() {
        return scoreboard.size();
    }

    private void checkForAtLeastOnePlayer() {
        if (scoreboard.size() == 0 && status != Status.WAITING) {
            status = Status.FINISHED;
        }
    }

    private boolean allReady() {
        for (Map.Entry<Player, Integer> entry : scoreboard.entrySet()) {
            if (entry.getValue() == -1) {
                return false;
            }
        }
        return true;
    }

    protected int getLabyrinthWidth() {
        return Math.max(12, (scoreboard.size() / 2) * 6);
    }

    protected int getLabyrinthHeight() {
        return Math.max(10, (scoreboard.size() / 2) * 4);
    }

    protected synchronized String[] getPlayerIDs() {
        ArrayList<String> playerIDList = new ArrayList<>();
        scoreboard.keySet().forEach(key -> playerIDList.add(key.getID()));
        return playerIDList.toArray(new String[0]);
    }

    protected synchronized void startGame() throws IOException {
        status = Status.INGAME;
        maze = new Labyrinth(getLabyrinthWidth(), getLabyrinthHeight());
        maze.placePlayers(scoreboard.keySet().toArray(new Player[0]));
        maze.placeGhosts();
        lastGhostMove = Instant.now().toEpochMilli();
        int ghostNum = maze.getGhostNum();
        Launcher.logger.addGeneratedLabyrinth(multicastAddress, multicastPort, maze);
        Launcher.logger.addUDPpartyLog(multicastAddress, multicastPort);
        for (SelectionKey key : playerSelector.keys()) {
            SocketChannel player = (SocketChannel) key.channel();
            if (player.write(Common.welcoMessageByteArray(lobbyIndex, getLabyrinthHeight() + 1, getLabyrinthWidth() + 1,
                    ghostNum, multicastAddress, multicastPort, player).flip()) == 0) {
                disconnectPlayer("Failed to send WELCO message", player, key);
            }
        }
        for (SelectionKey key : playerSelector.keys()) {
            SocketChannel player = (SocketChannel) key.channel();
            Player p = (Player) key.attachment();
            if (player.write(Common.positMessageByteArray(p.getID(), String.valueOf(maze.getXofPlayer(p)),
                    String.valueOf(maze.getYofPlayer(p)), player).flip()) == 0) {
                disconnectPlayer("Failed to send POSIT message", player, key);
            }
        }
    }

    protected synchronized boolean addPlayer(Player p, SocketChannel playerSocket) throws IOException {
        if (scoreboard.containsKey(p) || usedUDPClientPort(p.getUDP())) {
            return false;
        }
        scoreboard.put(p, -1);
        SelectionKey key = playerSocket.register(playerSelector, SelectionKey.OP_READ | SelectionKey.OP_WRITE);
        key.attach(p);
        if (scoreboard.size() == 255) {
            status = Status.FULL;
        }
        return true;
    }

    private boolean usedUDPClientPort(String s) {
        for (Map.Entry<Player, Integer> entry : scoreboard.entrySet()) {
            if (entry.getKey().getUDP().equals(s)) {
                return true;
            }
        }
        return false;
    }

    private void sendGOBYE(SocketChannel player, SelectionKey key) throws IOException {
        if (player.write(Common.gobyeMessageByteArray(player).flip()) == 0) {
            disconnectPlayer("Failed to send GOBYE message", player, key);
        }
        scoreboard.remove(key.attachment());
        if (status == Status.INGAME) {
            maze.removePlayer((Player) key.attachment());
        }
        key.attach(null);
        player.close();
        key.cancel();
        checkForAtLeastOnePlayer();
    }

    private synchronized void sendUNREGResponse(SocketChannel player, SelectionKey key) throws IOException {
        scoreboard.remove(key.attachment());
        key.cancel();
        Launcher.getQueueService().addClient((SocketChannel) key.channel());
        if (player.write(Common.unrokMessageByteArray(lobbyIndex, player).flip()) == 0) {
            disconnectPlayer("Failed to send UNROK message in " + SERVICE_NAME, player, key);
        }
    }

    private void sendMOVresponse(int distance, String direction, SocketChannel player, SelectionKey key)
            throws IOException {
        Player p = (Player) key.attachment();
        boolean foundGhost = false;
        Boolean b = false;
        for (int i = 0; i < distance; i++) {
            switch (direction) {
                case "up":
                    b = maze.moveUp(p);
                    break;
                case "down":
                    b = maze.moveDown(p);
                    break;
                case "left":
                    b = maze.moveLeft(p);
                    break;
                case "right":
                    b = maze.moveRight(p);
                    break;
            }
            if (b == null) {
                break;
            } else if (b) {
                foundGhost = true;
                int currPoints = scoreboard.get(p);
                scoreboard.put(p, currPoints + 1);
                String points = String.valueOf(currPoints + 1);
                String x = String.valueOf(maze.getXofPlayer(p));
                String y = String.valueOf(maze.getYofPlayer(p));
                x = "0".repeat(3 - x.length()) + x;
                y = "0".repeat(3 - y.length()) + y;
                points = "0".repeat(4 - points.length()) + points;
                sendMulticastMessage(ProtocolMessages.SCORE + " " + p.getID() + " " + points + " " + x + " " + y
                        + ProtocolMessages.ENDUDP);
            }
        }
        if (foundGhost) {
            if (player.write(Common.movefMessageByteArray(String.valueOf(maze.getXofPlayer(p)),
                    String.valueOf(maze.getYofPlayer(p)), String.valueOf(scoreboard.get(p)), player).flip()) == 0) {
                disconnectPlayer("Failed to send MOVEF message", player, key);
            }
            if (maze.getGhostNum() == 0) {
                status = Status.FINISHED;
                Player maxPlayer = getMaxPlayer();
                String points = String.valueOf(scoreboard.get(maxPlayer));
                points = "0".repeat(4 - points.length()) + points;
                sendMulticastMessage(
                        ProtocolMessages.ENDGA + " " + maxPlayer.getID() + " " + points + ProtocolMessages.ENDUDP);
            }
        } else {
            if (player.write(Common.moveeMessageByteArray(String.valueOf(maze.getXofPlayer(p)),
                    String.valueOf(maze.getYofPlayer(p)), player).flip()) == 0) {
                disconnectPlayer("Failed to send MOVE! message", player, key);
            }
        }
    }

    private Player getMaxPlayer() {
        int max = -1;
        Player maxPlayer = new Player("testtest", "9999");
        for (Map.Entry<Player, Integer> entry : scoreboard.entrySet()) {
            if (entry.getValue() > max) {
                max = entry.getValue();
                maxPlayer = entry.getKey();
            }
        }
        return maxPlayer;
    }

    private void sendGLISqresponse(SocketChannel player, SelectionKey key) throws IOException {
        if (player.write(Common.gliseMessageByteArray(scoreboard.size(), player).flip()) == 0) {
            disconnectPlayer("Failed to send GLIS! message", player, key);
        }
        for (Map.Entry<Player, Integer> entry : scoreboard.entrySet()) {
            if (player.write(Common
                    .gplyrMessageByteArray(entry.getKey().getID(), String.valueOf(maze.getXofPlayer(entry.getKey())),
                            String.valueOf(maze.getYofPlayer(entry.getKey())),
                            String.valueOf(scoreboard.get(entry.getKey())), player)
                    .flip()) == 0) {
                disconnectPlayer("Failed to send GPLYR message", player, key);
            }
        }
    }

    private void sendEXPLqresponse(SocketChannel player, SelectionKey key) throws IOException {
        Player p = (Player) key.attachment();
        boolean b = p.useBomb();
        if (b) {
            if (player.write(Common.expleMessageByteArray(player).flip()) == 0) {
                disconnectPlayer("Failed to send EXPL! message", player, key);
            }
            maze.useBomb(p);
            String x = String.valueOf(maze.getXofPlayer(p));
            String y = String.valueOf(maze.getYofPlayer(p));
            x = "0".repeat(3 - x.length()) + x;
            y = "0".repeat(3 - y.length()) + y;
            sendMulticastMessage(
                    ProtocolMessages.BOMBe + " " + p.getID() + " " + x + " " + y + ProtocolMessages.ENDUDP);
        } else {
            if (player.write(Common.explnMessageByteArray(player).flip()) == 0) {
                disconnectPlayer("Failed to send EXPLN message", player, key);
            }
        }
    }

    private void sendRADAqresponse(SocketChannel player, SelectionKey key) throws IOException {
        Player p = (Player) key.attachment();
        boolean b = p.useRadar();
        if (b) {
            String[] casesList = maze.toCASESformat();
            int n = casesList.length;
            int y = casesList[0].length();
            if (player.write(Common.radaeMessageByteArray(n, y, player).flip()) == 0) {
                disconnectPlayer("Failed to send RADA! message", player, key);
            }
            for (String s : casesList) {
                if (player.write(Common.casesMessageByteArray(y, s, player).flip()) == 0) {
                    disconnectPlayer("Failed to send CASES message", player, key);
                }
            }
        } else {
            if (player.write(Common.radanMessageByteArray(player).flip()) == 0) {
                disconnectPlayer("Failed to send RADAN message", player, key);
            }
        }
    }

    private void sendMALLqresponse(String mess, SocketChannel player, SelectionKey key) throws IOException {
        sendMulticastMessage(ProtocolMessages.MESSA + " " + ((Player) key.attachment()).getID() + " " + mess
                + ProtocolMessages.ENDUDP);
        if (player.write(Common.malleMessageByteArray(player).flip()) == 0) {
            disconnectPlayer("Failed to send MALL! message", player, key);
        }
    }

    private void sendSENDqresponse(String id, String mess, SocketChannel player, SelectionKey key) throws IOException {
        boolean possible = false;
        Player p = new Player("testtest", "9999");
        for (Player pl : scoreboard.keySet()) {
            if (pl.getID().equals(id)) {
                possible = true;
                p = pl;
                break;
            }
        }
        if (possible) {
            for (SelectionKey k : playerSelector.keys()) {
                if (((Player) k.attachment()).equals(p)) {
                    sendUDPMessage(p, ProtocolMessages.MESSP + " " + ((Player) key.attachment()).getID() + " " + mess + ProtocolMessages.ENDUDP,
                            (SocketChannel) k.channel());
                    if (player.write(Common.sendeMessageByteArray(player).flip()) == 0) {
                        disconnectPlayer("Failed to SENDe message", player, key);
                    }
                    break;
                }
            }
        } else {
            if (player.write(Common.nsendMessageByteArray(player).flip()) == 0) {
                disconnectPlayer("Failed to NSEND message", player, key);
            }
        }
    }

    private void sendMulticastMessage(String mess) throws IOException {
        Launcher.logger.addToLobbyUDPLog(multicastAddress, multicastPort, mess);
        DatagramSocket dso = new DatagramSocket();
        byte[] data;
        data = mess.getBytes();
        InetSocketAddress ia = new InetSocketAddress(multicastAddress, Integer.valueOf(multicastPort));
        DatagramPacket pq = new DatagramPacket(data, data.length, ia);
        dso.send(pq);
        dso.close();
    }

    private void sendUDPMessage(Player p, String mess, SocketChannel sc) throws IOException {
        Launcher.logger.addToLobbyUDPLog(multicastAddress, multicastPort, mess);
        DatagramSocket dso = new DatagramSocket();
        byte[] data;
        data = mess.getBytes();
        DatagramPacket pq = new DatagramPacket(data, data.length, sc.socket().getInetAddress(),
                Integer.valueOf(p.getUDP()));
        dso.send(pq);
        dso.close();
    }
}
