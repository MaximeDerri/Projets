package src.server;

import java.nio.channels.*;
import java.net.*;

class Launcher {

    private static QueueService mainLobby;
    public static final Logger logger = new Logger();

    public static void main(String[] args) {
        try {
            int port = Common.definePort(args);
            Selector selector = Selector.open();
            // InetSocketAddress serverAddress = new InetSocketAddress("localhost", port);
            InetSocketAddress serverAddress = new InetSocketAddress("192.168.70.236", port);
            ServerSocketChannel serverChannel = ServerSocketChannel.open();
            serverChannel.bind(serverAddress);
            serverChannel.configureBlocking(false);
            serverChannel.register(selector, SelectionKey.OP_ACCEPT);
            mainLobby = new QueueService(selector, serverChannel);
            Thread t = new Thread(mainLobby);
            t.start();
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
        }
    }

    protected static QueueService getQueueService() {
        return mainLobby;
    }

}
