package src.server;

import java.io.*;
import java.nio.file.*;
import java.time.Instant;

public class Logger {

    public static String folderName;

    Logger() {
        long now = Instant.now().toEpochMilli();
        folderName = "server" + now;
        try {
            Path path = Paths.get("log/" + folderName);
            Files.createDirectories(path);

        } catch (IOException e) {
            System.out.println("Failed to create log directory:");
            e.printStackTrace();
        }
    }

    void addClient(String s) {
        try {
            File clientFileLog = new File("log/" + folderName + "/" + s + ".txt");
            if (!clientFileLog.createNewFile()) {
                new FileWriter("log/" + folderName + "/" + s + ".txt", false).close();
            }
        } catch (IOException e) {
            System.out.println("Failed to create client log file");
            e.printStackTrace();
        }
    }

    void addUDPpartyLog(String ip, String port) {
        try {
            File clientFileLog = new File("log/" + folderName + "/udp:" + ip + ":" + port + ".txt");
            if (!clientFileLog.createNewFile()) {
                new FileWriter("log/" + folderName + "/udp:" + ip + ":" + port + ".txt", false).close();
            }
        } catch (IOException e) {
            System.out.println("Failed to create udp log file");
            e.printStackTrace();
        }
    }

    void addGeneratedLabyrinth(String ip, String port, Labyrinth l) {
        try {
            File labyrinthFile = new File("log/" + folderName + "/labyrinth:" + ip + ":" + port + ".txt");
            if (labyrinthFile.createNewFile()) {
                updateLabyrinth(ip, port, l);
            } else {
                new FileWriter("log/" + folderName + "/udp:" + ip + ":" + port + ".txt", false).close();
            }
        } catch (IOException e) {
            System.out.println("Failed to create labyrinth file");
            e.printStackTrace();
        }
    }

    void updateLabyrinth(String ip, String port, Labyrinth l) {
        try {
            System.out.println(l.toString());
            FileWriter writer = new FileWriter("log/" + folderName + "/labyrinth:" + ip + ":" + port + ".txt");
            writer.write(l.toFile());
            writer.close();
        } catch (IOException e) {
            System.out.println("Failed to create labyrinth file");
            e.printStackTrace();
        }
    }

    void addToClientLog(String fileName, String toLog, boolean server) {
        try {
            if (server) {
                System.out.println("SERVER - " + toLog);
            } else {
                System.out.println(fileName + " - " + toLog);
            }
            FileWriter writer = new FileWriter("log/" + folderName + "/" + fileName + ".txt", true);
            if (server) {
                writer.write("SERVER - " + toLog + "\n");
            } else {
                writer.write(fileName + " - " + toLog + "\n");
            }
            writer.close();
        } catch (IOException e) {
            System.out.println("Failed to add to client log");
            e.printStackTrace();
        }
    }

    void addToLobbyUDPLog(String ip, String port, String toLog) {
        try {
            String s = "UDP - " + toLog;
            System.out.println(s);
            FileWriter writer = new FileWriter("log/" + folderName + "/udp:" + ip + ":" + port + ".txt", true);
            writer.write(s + "\n");
            writer.close();
        } catch (IOException e) {
            System.out.println("Failed to add multi message to udp lobby log");
            e.printStackTrace();
        }
    }

}
