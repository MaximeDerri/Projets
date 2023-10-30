package src.server;

import java.nio.*;
import java.util.*;
import java.nio.channels.*;
import java.io.*;
import src.server.Common.*;

public class QueueService implements Runnable {

    private Selector clientSelector;
    private ServerSocketChannel serverChannel;
    private static HashMap<Integer, LobbyService> activeLobbies = new HashMap<>();
    private final String SERVICE_NAME = "QueueService";

    QueueService(Selector clientSelector, ServerSocketChannel serverChannel) {
        this.clientSelector = clientSelector;
        this.serverChannel = serverChannel;
    }

    @Override
    @SuppressWarnings("java:S2189")
    public void run() {
        try {
            while (true) {
                clientSelector.selectNow();
                Set<SelectionKey> selectedKeys = clientSelector.selectedKeys();
                Iterator<SelectionKey> iter = selectedKeys.iterator();
                while (iter.hasNext()) {
                    SelectionKey key = iter.next();
                    if (key.isAcceptable()) {
                        connectClient();
                    }
                    if (key.isReadable()) {
                        commandParser(key);
                    }
                    iter.remove();
                }
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
        }
    }

    private void connectClient() throws IOException {
        SocketChannel client = serverChannel.accept();
        Launcher.logger.addClient("client:" + Common.getSocketChannelID(client));
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client), "Has connected to the server",
                false);
        client.configureBlocking(false);
        SelectionKey key = client.register(clientSelector, SelectionKey.OP_READ | SelectionKey.OP_WRITE);
        sendGAMEqMessages(client, key, SERVICE_NAME);
    }

    protected synchronized void addClient(SocketChannel sc) throws IOException {
        SelectionKey key = sc.register(clientSelector, SelectionKey.OP_READ | SelectionKey.OP_WRITE);
    }

    private void commandParser(SelectionKey key) throws IOException {
        SocketChannel client = (SocketChannel) key.channel();
        ByteBuffer bbuf = ByteBuffer.allocate(100);
        int sizeRec;
        if ((sizeRec = client.read(bbuf)) == -1) {
            Common.protocolViolation("Has disconnected from " + SERVICE_NAME, client, key);
        }
        if (sizeRec > 0) {
            String receivedMessage = Common.stringOfByteMessage(bbuf.array());
            Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client), receivedMessage, false);
            String lastThreeChars = receivedMessage.substring(receivedMessage.length() - 3);
            if (lastThreeChars.equals(ProtocolMessages.END)) {
                String[] rcvMessageSplit = receivedMessage.substring(0, receivedMessage.length() - 3).split(" ");
                switch (rcvMessageSplit[0]) {
                    case ProtocolMessages.NEWPL:
                        sendNEWPLResponse(client, key, rcvMessageSplit);
                        break;
                    case ProtocolMessages.REGIS:
                        sendREGISResponse(client, key, rcvMessageSplit);
                        break;
                    case ProtocolMessages.LISTq:
                        sendLISTqMessages(client, key, rcvMessageSplit, SERVICE_NAME);
                        break;
                    case ProtocolMessages.GAMEq:
                        sendGAMEqMessages(client, key, SERVICE_NAME);
                        break;
                    case ProtocolMessages.SIZEq:
                        sendSIZEqMessages(client, key, rcvMessageSplit, SERVICE_NAME);
                        break;
                    case ProtocolMessages.STARq:
                        sendSTARqResponse(Integer.valueOf(rcvMessageSplit[1]), client, key, SERVICE_NAME);
                        break;
                    case ProtocolMessages.UNREG:
                        if (client.write(Common.dunnoMessageByteArray(client).flip()) == 0) {
                            Common.protocolViolation("Failed to send DUNNO message in" + SERVICE_NAME + "for UNREG",
                                    client, key);
                        }
                        break;
                    default:
                        Common.protocolViolation(
                                "Unrecognised command received in " + SERVICE_NAME + " parser: " + receivedMessage,
                                client, key);
                }
            } else {
                Common.protocolViolation("Parsed command lacks the correct termination sequence in " + SERVICE_NAME
                        + " parser:" + receivedMessage, client, key);
            }
        }
    }

    protected synchronized int hostLobby(Player p, SocketChannel sc) throws IOException {
        int lobbyIndex = activeLobbies.size();
        if (lobbyIndex < 255) {
            String multicastAddress = Common.generateMulticastAddress();
            while (multicastAddressInUse(multicastAddress)) {
                multicastAddress = Common.generateMulticastAddress();
            }
            String multicastPort = Common.generateMulticastPort();
            while (multicastPortInUse(multicastAddress)) {
                multicastAddress = Common.generateMulticastPort();
            }
            LobbyService l = new LobbyService(multicastAddress, multicastPort, lobbyIndex);
            activeLobbies.put(lobbyIndex, l);
            if (addPlayerToLobby(p, lobbyIndex, sc)) {
                new Thread(l).start();
                return lobbyIndex;
            }
            return -1;
        }
        return -1;
    }

    private boolean multicastAddressInUse(String mca) {
        for (Map.Entry<Integer, LobbyService> entry : activeLobbies.entrySet()) {
            LobbyService l = entry.getValue();
            if (l.getMulticastAddress().equals(mca)) {
                return true;
            }
        }
        return false;
    }

    private boolean multicastPortInUse(String mcp) {
        for (Map.Entry<Integer, LobbyService> entry : activeLobbies.entrySet()) {
            LobbyService l = entry.getValue();
            if (l.getMulticastPort().equals(mcp)) {
                return true;
            }
        }
        return false;
    }

    protected synchronized boolean addPlayerToLobby(Player p, int l, SocketChannel sc) throws IOException {
        if (activeLobbies.containsKey(l)) {
            if (activeLobbies.get(l).isWaiting()) {
                SelectionKey key = sc.keyFor(clientSelector);
                key.cancel();
                return activeLobbies.get(l).addPlayer(p, sc);
            }
        }
        return false;
    }

    protected static synchronized HashMap<Integer, Integer> getAllActiveLobbyInfo(boolean openOnly) {
        HashMap<Integer, Integer> lobbyInfo = new HashMap<>();
        for (Map.Entry<Integer, LobbyService> entry : activeLobbies.entrySet()) {
            LobbyService l = entry.getValue();
            if ((openOnly && l.isWaiting()) || !openOnly) {
                lobbyInfo.put(entry.getKey(), l.playerCount());
            }
        }
        return lobbyInfo;
    }

    protected static void sendGAMEqMessages(SocketChannel client, SelectionKey key, String serviceName)
            throws IOException {
        HashMap<Integer, Integer> lobbyInfo = getAllActiveLobbyInfo(true);
        if (client.write(Common.gamesMessageByteArray(lobbyInfo.size(), client).flip()) == 0) {
            Common.protocolViolation("Failed to send GAMES message in " + serviceName, client, key);
        } else {
            for (Map.Entry<Integer, Integer> entry : lobbyInfo.entrySet()) {
                if (client.write(Common.ogameMessageByteArray(entry.getKey(), entry.getValue(), client).flip()) == 0) {
                    Common.protocolViolation("Failed to send OGAME message in " + serviceName, client, key);
                }
            }
        }
    }

    protected static void sendSIZEqMessages(SocketChannel client, SelectionKey key, String[] rcvMessageSplit,
            String serviceName)
            throws IOException {
        int lobbyIndex = Integer.parseInt(String.valueOf(rcvMessageSplit[1]));
        if (activeLobbies.containsKey(lobbyIndex)) {
            LobbyService l = activeLobbies.get(lobbyIndex);
            int width = l.getLabyrinthWidth();
            int height = l.getLabyrinthHeight();
            if (client.write(Common.sizeeMessageByteArray(lobbyIndex, height, width, client).flip()) == 0) {
                Common.protocolViolation("Failed to send SIZE! message in " + serviceName, client, key);
            }
        } else {
            if (client.write(Common.dunnoMessageByteArray(client).flip()) == 0) {
                Common.protocolViolation("Failed to send DUNNO message in " + serviceName + "for SIZE?", client, key);
            }
        }
    }

    protected static void sendLISTqMessages(SocketChannel client, SelectionKey key, String[] rcvMessageSplit,
            String serviceName) throws IOException {
        int lobbyIndex = Integer.parseInt(String.valueOf(rcvMessageSplit[1]));
        if (activeLobbies.containsKey(lobbyIndex)) {
            LobbyService l = activeLobbies.get(lobbyIndex);
            int playerCount = l.playerCount();
            if (client.write(Common.listeMessageByteArray(lobbyIndex, playerCount, client).flip()) == 0) {
                Common.protocolViolation("Failed to send LIST! message in " + serviceName, client, key);
            } else {
                String[] playerIDs = l.getPlayerIDs();
                for (String s : playerIDs) {
                    if (client.write(Common.playrMessageByteArray(s, client).flip()) == 0) {
                        Common.protocolViolation("Failed to send PLAYR message in " + serviceName, client, key);
                    }
                }
            }
        } else {
            if (client.write(Common.dunnoMessageByteArray(client).flip()) == 0) {
                Common.protocolViolation("Failed to send DUNNO message in " + serviceName + "for LIST?", client, key);
            }
        }
    }

    protected static synchronized void sendSTARqResponse(int lobbyIndex, SocketChannel client, SelectionKey key,
            String serviceName) throws IOException {
        if (activeLobbies.containsKey(lobbyIndex)) {
            HashMap<Player, Integer> scoreboard = activeLobbies.get(lobbyIndex).getScoreboard();
            if (client.write(Common.stareMessageByteArray(lobbyIndex, scoreboard.size(), client)
                    .flip()) == 0) {
                Common.protocolViolation("Failed to send STAR! message in " + serviceName, client, key);
            } else {
                for (Map.Entry<Player, Integer> entry : scoreboard.entrySet()) {
                    int toReturn;
                    if (entry.getValue() > -1) {
                        toReturn = 1;
                    } else {
                        toReturn = 0;
                    }
                    if (client.write(Common.starpMessageByteArray(entry.getKey().getID(),
                            toReturn, client).flip()) == 0) {
                        Common.protocolViolation("Failed to send STARP messagein " + serviceName, client, key);
                    }
                }
            }
        } else {
            if (client.write(Common.dunnoMessageByteArray(client).flip()) == 0) {
                Common.protocolViolation("Failed to send DUNNO message in " + serviceName + "for LIST?", client, key);
            }
        }
    }

    private void sendREGISResponse(SocketChannel client, SelectionKey key, String[] rcvMessageSplit)
            throws IOException {
        Player newPlayer = new Player(rcvMessageSplit[1], rcvMessageSplit[2]);
        int lobbyIndex = Integer.parseInt(String.valueOf(rcvMessageSplit[3]));
        if (addPlayerToLobby(newPlayer, lobbyIndex, client)) {
            if (client.write(Common.regokMessageByteArray(lobbyIndex, client).flip()) == 0) {
                Common.protocolViolation("Failed to send REGOK message in " + SERVICE_NAME, client,
                        key);
            }
        } else {
            if (client.write(Common.regnoMessageByteArray(client).flip()) == 0) {
                Common.protocolViolation("Failed to send REGNO message in " + SERVICE_NAME, client,
                        key);
            }
        }
    }

    private void sendNEWPLResponse(SocketChannel client, SelectionKey key, String[] rcvMessageSplit)
            throws IOException {
        Player newPlayer = new Player(rcvMessageSplit[1], rcvMessageSplit[2]);
        int lobbyIndex = hostLobby(newPlayer, client);
        if (lobbyIndex >= 0) {
            if (client.write(Common.regokMessageByteArray(lobbyIndex, client).flip()) == 0) {
                Common.protocolViolation("Failed to send REGOK message in " + SERVICE_NAME, client,
                        key);
            }
        } else {
            if (client.write(Common.regnoMessageByteArray(client).flip()) == 0) {
                Common.protocolViolation("Failed to send REGNO message in " + SERVICE_NAME, client,
                        key);
            }
        }
    }

}