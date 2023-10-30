package src.server;

import java.nio.channels.*;
import java.nio.*;
import java.util.*;
import java.io.*;

class Common {

    private static Random random = new Random();

    public static final String RESET = "\033[0m";

    // Regular Colors
    public static final String RED = "\033[0;31m";
    public static final String GREEN = "\033[0;32m";
    public static final String YELLOW = "\033[0;33m";
    public static final String BLUE = "\033[0;34m";
    public static final String PURPLE = "\033[0;35m";

    // Bold
    public static final String RED_BOLD = "\033[1;31m";
    public static final String GREEN_BOLD = "\033[1;32m";
    public static final String YELLOW_BOLD = "\033[1;33m";
    public static final String BLUE_BOLD = "\033[1;34m";
    public static final String BRIGHT_BLUE_BOLD = "\033[1;96m";
    public static final String PURPLE_BOLD = "\033[1;35m";
    public static final String WHITE_BOLD = "\033[1;37m";

    // Underline
    public static final String RED_UNDERLINED = "\033[4;31m";
    public static final String GREEN_UNDERLINED = "\033[4;32m";
    public static final String YELLOW_UNDERLINED = "\033[4;33m";
    public static final String BLUE_UNDERLINED = "\033[4;34m";
    public static final String PURPLE_UNDERLINED = "\033[4;35m";
    public static final String WHITE_UNDERLINED = "\033[4;37m";

    // Background
    public static final String BLACK_BACKGROUND = "\033[40m";
    public static final String RED_BACKGROUND = "\033[41m";
    public static final String GREEN_BACKGROUND = "\033[42m";
    public static final String YELLOW_BACKGROUND = "\033[43m";
    public static final String BLUE_BACKGROUND = "\033[44m";
    public static final String PURPLE_BACKGROUND = "\033[45m";
    public static final String WHITE_BACKGROUND = "\033[47m";

    class ProtocolMessages {
        // Standard TCP
        static final String GAMES = "GAMES";
        static final String OGAME = "OGAME";
        static final String NEWPL = "NEWPL";
        static final String REGIS = "REGIS";
        static final String REGOK = "REGOK";
        static final String REGNO = "REGNO";
        static final String START = "START";
        static final String UNREG = "UNREG";
        static final String UNROK = "UNROK";
        static final String DUNNO = "DUNNO";
        static final String SIZEq = "SIZE?";
        static final String SIZEe = "SIZE!";
        static final String LISTq = "LIST?";
        static final String LISTe = "LIST!";
        static final String PLAYR = "PLAYR";
        static final String GAMEq = "GAME?";
        static final String WELCO = "WELCO";
        static final String POSIT = "POSIT";
        static final String UPMOV = "UPMOV";
        static final String DOMOV = "DOMOV";
        static final String LEMOV = "LEMOV";
        static final String RIMOV = "RIMOV";
        static final String MOVEe = "MOVE!";
        static final String MOVEF = "MOVEF";
        static final String IQUIT = "IQUIT";
        static final String GOBYE = "GOBYE";
        static final String GLISq = "GLIS?";
        static final String GLISe = "GLIS!";
        static final String GPLYR = "GPLYR";
        static final String MALLq = "MALL?";
        static final String MALLe = "MALL!";
        static final String SENDq = "SEND?";
        static final String SENDe = "SEND!";
        static final String NSEND = "NSEND";
        static final String END = "***";
        // Standard UDP
        static final String GHOST = "GHOST";
        static final String SCORE = "SCORE";
        static final String MESSA = "MESSA";
        static final String ENDGA = "ENDGA";
        static final String MESSP = "MESSP";
        static final String ENDUDP = "+++";
        // Extra
        static final String STARq = "STAR?";
        static final String STARe = "STAR!";
        static final String STARP = "STARP";
        static final String EXPLq = "EXPL?";
        static final String EXPLe = "EXPL!";
        static final String EXPLN = "EXPLN";
        static final String BOMBe = "BOMB!";
        static final String RADAq = "RADA?";
        static final String RADAe = "RADA!";
        static final String RADAN = "RADAN";
        static final String CASES = "CASES";
    }

    static String getSocketChannelID(SocketChannel sc) {
        String s = sc.toString().split(":")[2];
        return s.substring(0, s.length() - 1);
    }

    static byte intToOneByte(int i) throws IllegalArgumentException {
        if (i > 255) {
            throw new IllegalArgumentException("This integer is too large to encode on 1 byte :" + i);
        } else if (i < 0) {
            throw new IllegalArgumentException("This integer is negative and cannot be encoded :" + i);
        }
        return (byte) i;
    }

    static byte[] intToTwoBytes(int i) throws IllegalArgumentException {
        if (i > 999) {
            throw new IllegalArgumentException("This integer is too large to encode on 2 bytes :" + i);
        } else if (i < 0) {
            throw new IllegalArgumentException("This integer is negative and cannot be encoded :" + i);
        }
        byte[] barray = new byte[2];
        barray[1] = (byte) (i & 0xFF);
        barray[0] = (byte) ((i >>> 8) & 0xFF);
        return barray;
    }

    static byte charToOneByte(char c) {
        return (byte) c;
    }

    static int definePort(String[] args) {
        try {
            Integer parsePort = Integer.parseInt(args[0]);
            if (parsePort > 1024 && parsePort < 65535) {
                return parsePort;
            }
            return 4501;
        } catch (Exception e) {
            return 4501;
        }
    }

    static void protocolViolation(String s, SocketChannel client, SelectionKey key) throws IOException {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client), s, false);
        key.attach(null);
        client.close();
        key.cancel();
    }

    public static int getRandomNumber(int min, int max) {
        return (int) ((Math.random() * (max - min)) + min);
    }

    static String generateMulticastAddress() {
        // 224.0.0.0 to 239.255.255.255
        int o1 = getRandomNumber(224, 240);
        int o2 = getRandomNumber(0, 256);
        int o3 = getRandomNumber(0, 256);
        int o4 = getRandomNumber(0, 256);
        return o1 + "." + o2 + "." + o3 + "." + o4;
    }

    static String generateMulticastPort() {
        // 1024 to 9999
        return String.valueOf(getRandomNumber(1024, 10000));
    }
    
    static String stringOfSentMessage(byte[] barray, int n) {
        StringBuilder s = new StringBuilder();
        for (byte b : barray) {
            char c = (char) b;
            if (c != '\0'){
                s.append(c);
            } else {
                break;
            }
        }
        System.out.println(s.length());
        return s.toString().substring(n, s.length() - 3);
    }

    static String stringOfByteMessage(byte[] barray) {
        StringBuilder builder = new StringBuilder();
        int count = 0;
        boolean keepParsing = true;
        int prev = -1;
        int secondPrev = -1;
        int thirdPrev = -1;
        int countStar = 0;
        while (keepParsing && count < 100) {
            byte b = barray[count];
            int ordinal = b & 0xFF;
            if (ordinal <= 9 || (90 < ordinal && ordinal < 97) || 122 < ordinal) {
                builder.append(Byte.toUnsignedInt(b));
            } else if (ordinal == 42 && ((9 < prev && prev <= 90) || (97 <= prev && prev <= 122)) && secondPrev == 32
                    && thirdPrev != 32) {
                int encodedIntToAdd = builder.charAt(builder.length() - 1);
                builder.setLength(builder.length() - 1);
                builder.append(encodedIntToAdd);
                builder.append((char) b);
            } else {
                builder.append((char) b);
                if (prev == 42) {
                    countStar++;
                } else {
                    countStar = 0;
                }
                if (countStar == 2) {
                    countStar = 0;
                    keepParsing = false;
                }
            }
            if (secondPrev != -1) {
                thirdPrev = secondPrev;
            }
            if (prev != -1) {
                secondPrev = prev;
            }
            prev = ordinal;
            count++;
        }
        return builder.toString();
    }

    static ByteBuffer gamesMessageByteArray(int lobbyInfo, SocketChannel client) {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.GAMES + " " + lobbyInfo + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(10);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.GAMES.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        bbuf.put(Common.intToOneByte(lobbyInfo));
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer unrokMessageByteArray(int i, SocketChannel client) {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.UNROK + " " + i + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(10);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.UNROK.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        bbuf.put(Common.intToOneByte(i));
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer regokMessageByteArray(int l, SocketChannel client) {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.REGOK + " " + l + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(10);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.REGOK.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        bbuf.put(Common.intToOneByte(l));
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer dunnoMessageByteArray(SocketChannel client) {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.DUNNO + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(8);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.DUNNO.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer gobyeMessageByteArray(SocketChannel client) {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.GOBYE + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(8);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.GOBYE.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer regnoMessageByteArray(SocketChannel client) {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.REGNO + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(8);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.REGNO.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer ogameMessageByteArray(int key, int value, SocketChannel client) {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.OGAME + " " + key + " " + value + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(12);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.OGAME.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        bbuf.put(Common.intToOneByte(key));
        bbuf.put(Common.charToOneByte(' '));
        bbuf.put(Common.intToOneByte(value));
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer listeMessageByteArray(int lobbyIndex, int playerCount, SocketChannel client) {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.LISTe + " " + lobbyIndex + " " + playerCount + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(12);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.LISTe.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        bbuf.put(Common.intToOneByte(lobbyIndex));
        bbuf.put(Common.charToOneByte(' '));
        bbuf.put(Common.intToOneByte(playerCount));
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer playrMessageByteArray(String s, SocketChannel client) {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.PLAYR + " " + s + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(17);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.PLAYR.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        for (char c : s.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer sizeeMessageByteArray(int lobbyIndex, int height, int width, SocketChannel client) {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.SIZEe + " " + lobbyIndex + " " + height + " " + width + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(16);
        for (char c : ProtocolMessages.SIZEe.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        bbuf.put(Common.intToOneByte(lobbyIndex));
        bbuf.put(Common.charToOneByte(' '));
        bbuf.put(Common.intToTwoBytes(height));
        bbuf.put(Common.charToOneByte(' '));
        bbuf.put(Common.intToTwoBytes(width));
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer welcoMessageByteArray(int lobbyIndex, int height, int width, int ghostNum, String ip, String port,
            SocketChannel client) {
        ip = ip + "#".repeat(15 - ip.length());
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.WELCO + " " + lobbyIndex + " " + height + " " + width + " " + ghostNum + " " + ip + " "
                        + port + ProtocolMessages.END,
                true);
        ByteBuffer bbuf = ByteBuffer.allocate(39);
        for (char c : ProtocolMessages.WELCO.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        bbuf.put(Common.intToOneByte(lobbyIndex));
        bbuf.put(Common.charToOneByte(' '));
        bbuf.put(Common.intToTwoBytes(height));
        bbuf.put(Common.charToOneByte(' '));
        bbuf.put(Common.intToTwoBytes(width));
        bbuf.put(Common.charToOneByte(' '));
        bbuf.put(Common.intToOneByte(ghostNum));
        bbuf.put(Common.charToOneByte(' '));
        for (char c : ip.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        for (char c : port.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer positMessageByteArray(String id, String x, String y, SocketChannel client) {
        x = "0".repeat(3 - x.length()) + x;
        y = "0".repeat(3 - y.length()) + y;
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.POSIT + " " + id + " " + x + " " + y + ProtocolMessages.END,
                true);
        ByteBuffer bbuf = ByteBuffer.allocate(25);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.POSIT.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        for (char c : id.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        for (char c : x.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        for (char c : y.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer stareMessageByteArray(int key, int value, SocketChannel client) {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.STARe + " " + key + " " + value + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(12);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.STARe.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        bbuf.put(Common.intToOneByte(key));
        bbuf.put(Common.charToOneByte(' '));
        bbuf.put(Common.intToOneByte(value));
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer starpMessageByteArray(String id, int b, SocketChannel client) {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.STARP + " " + id + " " + b + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(19);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.STARP.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        for (char c : id.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        bbuf.put(Common.intToOneByte(b));
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer moveeMessageByteArray(String x, String y, SocketChannel client) {
        x = "0".repeat(3 - x.length()) + x;
        y = "0".repeat(3 - y.length()) + y;
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.MOVEe + " " + x + " " + y + ProtocolMessages.END,
                true);
        ByteBuffer bbuf = ByteBuffer.allocate(16);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.MOVEe.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        for (char c : x.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        for (char c : y.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer movefMessageByteArray(String x, String y, String points, SocketChannel client) {
        x = "0".repeat(3 - x.length()) + x;
        y = "0".repeat(3 - y.length()) + y;
        points = "0".repeat(4 - points.length()) + points;
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.MOVEF + " " + x + " " + y + " " + points + ProtocolMessages.END,
                true);
        ByteBuffer bbuf = ByteBuffer.allocate(21);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.MOVEF.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        for (char c : x.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        for (char c : y.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        for (char c : points.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer gliseMessageByteArray(int lobbyInfo, SocketChannel client) {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.GLISe + " " + lobbyInfo + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(10);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.GLISe.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        bbuf.put(Common.intToOneByte(lobbyInfo));
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer gplyrMessageByteArray(String id, String x, String y, String points, SocketChannel client) {
        x = "0".repeat(3 - x.length()) + x;
        y = "0".repeat(3 - y.length()) + y;
        points = "0".repeat(4 - points.length()) + points;
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.GPLYR + " " + id + " " + x + " " + y + " " + points + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(30);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.GPLYR.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        for (char c : id.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        for (char c : x.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        for (char c : y.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        for (char c : points.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer expleMessageByteArray(SocketChannel client) {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.EXPLe + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(8);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.EXPLe.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer explnMessageByteArray(SocketChannel client) {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.EXPLN + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(8);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.EXPLN.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer malleMessageByteArray(SocketChannel client) {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.MALLe + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(8);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.MALLe.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer sendeMessageByteArray(SocketChannel client) {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.SENDe + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(8);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.SENDe.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer nsendMessageByteArray(SocketChannel client) {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.NSEND + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(8);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.NSEND.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer radanMessageByteArray(SocketChannel client) {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.RADAN + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(8);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.RADAN.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer radaeMessageByteArray(int key, int value, SocketChannel client) {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.RADAe + " " + key + " " + value + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(12);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.RADAe.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        bbuf.put(Common.intToOneByte(key));
        bbuf.put(Common.charToOneByte(' '));
        bbuf.put(Common.intToOneByte(value));
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

    static ByteBuffer casesMessageByteArray(int y, String s, SocketChannel client) {
        Launcher.logger.addToClientLog("client:" + Common.getSocketChannelID(client),
                ProtocolMessages.CASES + " " + s + ProtocolMessages.END, true);
        ByteBuffer bbuf = ByteBuffer.allocate(9 + y);
        bbuf.order(ByteOrder.BIG_ENDIAN);
        for (char c : ProtocolMessages.CASES.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        bbuf.put(Common.charToOneByte(' '));
        for (char c : s.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        for (char c : ProtocolMessages.END.toCharArray()) {
            bbuf.put(Common.charToOneByte(c));
        }
        return bbuf;
    }

}
