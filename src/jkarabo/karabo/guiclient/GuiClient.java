/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.guiclient;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.ReadableByteChannel;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.SocketChannel;
import java.nio.channels.WritableByteChannel;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Set;
import java.util.concurrent.atomic.AtomicBoolean;
import karabo.util.BinarySerializer;
import karabo.util.BinarySerializerHash;
import karabo.util.Hash;
import karabo.util.Schema;
import karabo.util.vectors.VectorHash;
import org.apache.log4j.Logger;

/**
 *
 * @author Sergey Esenov serguei.essenov@xfel.eu
 */
public abstract class GuiClient implements Runnable {

    protected static final Logger LOG = Logger.getLogger(GuiClient.class);
    private final static int READ_BUFFER_SIZE = 0x100000;
    private final static int WRITE_BUFFER_SIZE = 0x100000;
    private static final long MAXIMUM_RECONNECT_INTERVAL = 30000;
    private static final long INITIAL_RECONNECT_INTERVAL = 500;

    private Selector selector;
    private SocketChannel channel;
    private SocketAddress address;

    private final AtomicBoolean connected = new AtomicBoolean(false);
    private HashMap<String, Integer> visible = null;

    private final ByteBuffer readBuf = ByteBuffer.allocateDirect(READ_BUFFER_SIZE); // 1Mb
    private final ByteBuffer writeBuf = ByteBuffer.allocateDirect(WRITE_BUFFER_SIZE); // 1Mb

    private final Thread thread = new Thread(this);
    private long reconnectInterval = INITIAL_RECONNECT_INTERVAL;

    private final BinarySerializer<Hash> serializer = BinarySerializerHash.create(BinarySerializerHash.class, "Bin", new Hash());

    /**
     * Start the separate communication thread that will connect to GuiServer device and will call the callbacks
     * implemented by user.
     */
    public void start() {
        visible = new HashMap<>();
        thread.start();
    }

    private void addVisible(String deviceId) {
        synchronized (visible) {
            if (!visible.containsKey(deviceId)) {
                visible.put(deviceId, 1);
            } else {
                visible.put(deviceId, visible.get(deviceId) + 1);
            }
        }
    }

    private void removeVisible(String deviceId) {
        synchronized (visible) {
            if (visible.containsKey(deviceId)) {
                int nvisible = visible.get(deviceId) - 1;
                if (nvisible <= 0) {
                    visible.remove(deviceId);
                } else {
                    visible.put(deviceId, nvisible);
                }
            }
        }
    }

    private boolean isVisible(String deviceId) {
        synchronized (visible) {
            if (visible.containsKey(deviceId)) {
                return true;
            }
        }
        return false;
    }

    private void cleanVisible(String deviceId) {
        try {
            while (isVisible(deviceId)) {
                stopMonitoringDevice(deviceId);
            }
        } catch (IOException | InterruptedException ex) {
            synchronized (visible) {
                visible.remove(deviceId);
            }
        }
    }

    protected void cleanVisibleAll() {
        Set<String> ids = null;
        synchronized (visible) {
            ids = visible.keySet();
        }
        if (ids == null) {
            return;
        }
        for (String id : ids) {
            synchronized (visible) {
                visible.remove(id);
            }
            cleanVisible(id);
        }
    }

    /**
     * Join the communication thread
     *
     * @throws InterruptedException may throw an exception
     */
    public void join() throws InterruptedException {
        if (Thread.currentThread().getId() != thread.getId()) {
            thread.join();
            System.out.println("Communication thread joined!");
        }
    }

    /**
     * Stop communication thread by sending interrupt.
     */
    public void stop() {
        thread.interrupt();
        selector.wakeup();
    }

    /**
     * Check connectivity flag
     *
     * @return true if the GuiServer connection is established
     */
    public boolean isConnected() {
        return connected.get();
    }

    @Override
    public void run() {
        readBuf.order(ByteOrder.LITTLE_ENDIAN);
        writeBuf.order(ByteOrder.LITTLE_ENDIAN);

        try {
            while (!Thread.interrupted()) {
                try {
                    selector = Selector.open();
                    channel = SocketChannel.open();
                    configureChannel(channel);
                    channel.connect(address);
                    channel.register(selector, SelectionKey.OP_CONNECT);

                    while (!Thread.interrupted() && channel.isOpen()) {
                        if (selector.select() > 0) {
                            processSelectedKeys(selector.selectedKeys());
                        }
                    }
                } catch (IOException e) {
                    LOG.error("IO exception", e);
                } finally {
                    connected.set(false);
                    onDisconnect();
                    writeBuf.clear();
                    readBuf.clear();
                    if (channel != null) {
                        channel.close();
                    }
                    if (selector != null) {
                        selector.close();
                    }
                    LOG.info("connection closed");
                }
                try {
                    Thread.sleep(reconnectInterval);
                    if (reconnectInterval < MAXIMUM_RECONNECT_INTERVAL) {
                        reconnectInterval *= 2;
                    }
                    LOG.info("reconnecting to " + address);
                } catch (InterruptedException e) {
                    break;
                }
            }

        } catch (IOException e) {
            LOG.error("unrecoverable error", e);
        } catch (java.nio.channels.CancelledKeyException e) {
        }

        cleanVisibleAll();

        try {
            channel.close();
        } catch (IOException ex) {
            LOG.error(ex);
        }

        try {
            selector.close();
        } catch (IOException ex) {
            LOG.error(ex);
        }
    }

    private void configureChannel(SocketChannel channel) throws IOException {
        channel.configureBlocking(false);
        channel.socket().setSendBufferSize(0x100000); // 1Mb
        channel.socket().setReceiveBufferSize(0x100000); // 1Mb
        channel.socket().setKeepAlive(true);
        channel.socket().setReuseAddress(true);
        channel.socket().setSoLinger(false, 0);
        channel.socket().setSoTimeout(0);
        channel.socket().setTcpNoDelay(true);
    }

    private void processSelectedKeys(Set<SelectionKey> selectedKeys) throws IOException {
        Iterator it = selectedKeys.iterator();
        while (it.hasNext()) {
            SelectionKey key = (SelectionKey) it.next();
            if (key.isConnectable()) {
                processConnect(key);
            }
            if (key.isReadable()) {
                processRead(key);
            }
            if (key.isWritable()) {
                processWrite(key);
            }
            it.remove();
        }
    }

    private void processRead(SelectionKey key) throws IOException {
        ReadableByteChannel ch = (ReadableByteChannel) key.channel();

        int bytesOp = 0, bytesTotal = 0;
        while (readBuf.hasRemaining() && (bytesOp = ch.read(readBuf)) > 0) {
            bytesTotal += bytesOp;
        }

        if (bytesTotal > 0) {
            readBuf.flip();
            onReadByteBuffer(readBuf);
            readBuf.compact();
        } else if (bytesOp == -1) {
            LOG.info("peer closed read channel");
            ch.close();
        }
    }

    private void processWrite(SelectionKey key) throws IOException {
        WritableByteChannel ch = (WritableByteChannel) key.channel();
        synchronized (writeBuf) {
            writeBuf.flip();

            int bytesOp = 0, bytesTotal = 0;
            while (writeBuf.hasRemaining() && (bytesOp = ch.write(writeBuf)) > 0) {
                bytesTotal += bytesOp;
            }

            if (writeBuf.remaining() == 0) {
                key.interestOps(key.interestOps() ^ SelectionKey.OP_WRITE);
            }

            if (bytesTotal > 0) {
                writeBuf.notify();
            } else if (bytesOp == -1) {
                LOG.info("peer closed write channel");
                ch.close();
            }

            writeBuf.compact();
        }
    }

    private void processConnect(SelectionKey key) throws IOException {
        SocketChannel ch = (SocketChannel) key.channel();
        if (ch.finishConnect()) {
            key.interestOps(key.interestOps() ^ SelectionKey.OP_CONNECT);
            key.interestOps(key.interestOps() | SelectionKey.OP_READ);
            reconnectInterval = INITIAL_RECONNECT_INTERVAL;
            connected.set(true);
            onConnect();
        }
    }

    /**
     * Get address of the GuiServer connection
     *
     * @return SocketAddress object
     */
    public SocketAddress getAddress() {
        return address;
    }

    /**
     * Set address to the GuiServer by giving ...
     *
     * @param ipaddr IP address and ...
     * @param port port number
     */
    public void setAddress(String ipaddr, int port) {
        this.address = new InetSocketAddress(ipaddr, port);
    }

    private void onReadByteBuffer(ByteBuffer rbuf) throws IOException {
        // copy to local (JVM heap based) ByteBuffer
        ByteBuffer archive = ByteBuffer.allocate(rbuf.limit());
        archive.order(rbuf.order());
        archive.put(rbuf);
        rbuf.flip();
        archive.flip();
        while (rbuf.remaining() > 0) {
            int size = archive.getInt();
            // if the last logical record is not completed ...
            if (size > rbuf.remaining()) {
                break;
            }
            //System.out.println(String.format("archive.content: 0x%x", new BigInteger(1, archive.array())));
            Hash hash = serializer.load(archive);
            archive.position(archive.position() + size);
            onRead(hash);
            // synchronize 'rbuf' and 'archive' positions.
            rbuf.position(rbuf.position() + size + Integer.SIZE/8);
            assert rbuf.position() == archive.position();
        }
    }

    private void sendByteBuffer(ByteBuffer buffer) throws InterruptedException, IOException {
        if (!connected.get()) {
            throw new IOException("not connected");
        }
        //System.out.println(String.format("archive.content: 0x%x", new BigInteger(1, buffer.array())));
        synchronized (writeBuf) {
            writeBuf.putInt(buffer.limit());
            writeBuf.put(buffer);
            // try direct write of what's in the buffer to free up space
            if (writeBuf.remaining() < buffer.remaining()) {
                writeBuf.flip();
                int bytesOp = 0, bytesTotal = 0;
                while (writeBuf.hasRemaining() && (bytesOp = channel.write(writeBuf)) > 0) {
                    bytesTotal += bytesOp;
                }
                writeBuf.compact();
            }

            // if didn't help, wait till some space appears
            if (Thread.currentThread().getId() != thread.getId()) {
                while (writeBuf.remaining() < buffer.remaining()) {
                    writeBuf.wait();
                }
            } else {
                if (writeBuf.remaining() < buffer.remaining()) {
                    throw new IOException("send buffer full"); // TODO: add reallocation or buffers chain
                }
            }
            writeBuf.put(buffer);

            // try direct write to decrease the latency
            writeBuf.flip();
            int bytesOp = 0, bytesTotal = 0;
            while (writeBuf.hasRemaining() && (bytesOp = channel.write(writeBuf)) > 0) {
                bytesTotal += bytesOp;
            }
            writeBuf.compact();

            if (writeBuf.hasRemaining()) {
                SelectionKey key = channel.keyFor(selector);
                key.interestOps(key.interestOps() | SelectionKey.OP_WRITE);
                selector.wakeup();
            }
        }
    }

    private void send(Hash hash) throws IOException, InterruptedException {
        ByteBuffer archive = serializer.save(hash);
        sendByteBuffer(archive);
    }

    private void onRead(Hash hash) {
        if (!hash.has("type")) {
            LOG.warn("Read unconventional Hash ...\n" + hash);
            return;
        }
        String type = hash.get("type");
        //System.out.println("onRead: type is \"" + type + "\"");
        switch (type) {
            case "brokerInformation":
                onBrokerInfo((String) hash.get("host"), (int) hash.get("port"), (String) hash.get("topic"));
                break;
            case "systemTopology":
                onSystemTopology((Hash) hash.get("systemTopology"));
                break;
            case "deviceConfiguration":
                onDeviceConfiguration((String) hash.get("deviceId"), (Hash) hash.get("configuration"));
                break;
            case "deviceSchema":
                onDeviceSchema((String) hash.get("deviceId"), (Schema) hash.get("schema"));
                break;
            case "classSchema":
                onClassSchema((String) hash.get("serverId"), (String) hash.get("classId"), (Schema) hash.get("schema"));
                break;
            case "propertyHistory":
                onPropertyHistory((String) hash.get("deviceId"), (String) hash.get("property"), (VectorHash) hash.get("data"));
                break;
            case "instanceNew":
                onInstanceNew((Hash) hash.get("topologyEntry"));
                break;
            case "instanceUpdated":
                onInstanceUpdated((Hash) hash.get("topologyEntry"));
                break;
            case "instanceGone":
                onInstanceGone((String) hash.get("instanceId"), (String) hash.get("instanceType"));
                break;
            case "notification":
                onNotification((String) hash.get("deviceId"), (String) hash.get("messageType"),
                        (String) hash.get("shortMsg"), (String) hash.get("detailedMsg"));
                break;
            case "log":
                onLogging((String) hash.get("message"));
                break;
            default:
                LOG.warn("Unsupported type: " + type);
        }
    }

    /**
     * This callback should be implemented by user. It will be called if GuiServer connection is successfully
     * established.
     */
    protected abstract void onConnect();

    /**
     * This callback should be implemented by user. It will be called if GuiServer connection is lost.
     */
    protected abstract void onDisconnect();

    /**
     * This callback should be implemented by user. It will be called if GuiServer sends information about JMS broker
     *
     * @param host broker host name
     * @param port broker host port
     * @param topic the topic used by broker for messaging
     */
    protected abstract void onBrokerInfo(String host, int port, String topic);

    /**
     * It will be called if GuiServer sends system network topology.
     *
     * @param systemTopology is a Hash containing topologies details.
     */
    protected abstract void onSystemTopology(Hash systemTopology);

    /**
     * This callback should be implemented by user. It will be called if GuiServer sends changed configuration of the
     * device.
     *
     * @param deviceId identifying the device
     * @param configuration changes in configuration of the device.
     */
    protected abstract void onDeviceConfiguration(String deviceId, Hash configuration);

    /**
     * This callback should be implemented by user. It will be called if GuiServer sends class schema before device
     * instantiation is happened. This schema will be provided by device server and will be sent to GuiServer.
     *
     * @param serverId device server ID
     * @param classId class ID
     * @param schema static schema definition
     */
    protected abstract void onClassSchema(String serverId, String classId, Schema schema);

    /**
     * This callback should be implemented by user. It will be called if GuiServer sends device schema after the device
     * instantiation is happened. This schema will be provided by device itself and will contain static and, optionally,
     * dynamic parts of full schema describing all the device parameters
     *
     * @param deviceId device ID
     * @param schema full schema for parameters describing such device
     */
    protected abstract void onDeviceSchema(String deviceId, Schema schema);

    /**
     * This callback should be implemented by user. It will be called if GuiServer sends historic data in form of vector
     * of Hashes
     *
     * @param deviceId device ID
     * @param property name of property
     * @param data data organized as vector of Hashes
     */
    protected abstract void onPropertyHistory(String deviceId, String property, VectorHash data);

    /**
     * This callback should be implemented by user. It will be called if GuiServer sends topology entry related to the
     * new, just created, device instance
     *
     * @param topologyEntry Hash object describing instance topology in detail.
     */
    protected abstract void onInstanceNew(Hash topologyEntry);

    /**
     * This callback should be implemented by user. It will be called if GuiServer sends updated topology entry related
     * to existing device instance
     *
     * @param topologyEntry Hash object describing instance topology in detail.
     */
    protected abstract void onInstanceUpdated(Hash topologyEntry);

    /**
     * This callback should be implemented by user. It will be called if GuiServer sends notification that device with
     * given instance ID was destroyed
     *
     * @param instanceId device instance ID
     * @param instanceType device instance type
     */
    protected abstract void onInstanceGone(String instanceId, String instanceType);

    /**
     * This callback should be implemented by user. It will be called if GuiServer sends some notification about device.
     *
     * @param deviceId ID of device
     * @param messageType type of message
     * @param shortMsg short version of problem description
     * @param detailedMsg detailed in depth problem description
     */
    protected abstract void onNotification(String deviceId, String messageType, String shortMsg, String detailedMsg);

    /**
     * This callback should be implemented by user. It will be called if GuiServer sends a message that should be
     * logged.
     *
     * @param message message sent
     */
    protected abstract void onLogging(String message);

    /**
     * Send login information to get an access to the GuiServer
     *
     * @param username user name in the system
     * @param sessionToken received session token
     * @param provider provider type (at the moment, "LOCAL" or "KERBEROS")
     * @throws IOException reported in case of IO problems
     * @throws InterruptedException reported if interrupted by user
     */
    public void login(String username, String sessionToken, String provider) throws IOException, InterruptedException {
        send(new Hash("type", "login", "username", username, "sessionToken", sessionToken, "provider", provider));
    }

    /**
     * Send new device configuration via GuiServer to the device (deviceId)
     *
     * @param deviceId device ID
     * @param configuration new configuration
     * @throws IOException reported in case of IO problems
     * @throws InterruptedException reported if interrupted by user
     */
    public void reconfigure(String deviceId, Hash configuration) throws IOException, InterruptedException {
        send(new Hash("type", "reconfigure", "deviceId", deviceId, "configuration", configuration));
    }

    /**
     * Send a request to execute a command on the device via GuiServer.
     *
     * @param deviceId device ID
     * @param command a command
     * @throws IOException reported in case of IO problems
     * @throws InterruptedException reported if interrupted by user
     */
    public void execute(String deviceId, String command) throws IOException, InterruptedException {
        send(new Hash("type", "execute", "deviceId", deviceId, "command", command));
    }

    /**
     * Send a request to refresh the information about device instance. It will be sent back and caught via
     * "onConfigurationChanged" callback.
     *
     * @param deviceId device instance ID
     * @throws IOException reported in case of IO problems
     * @throws InterruptedException reported if interrupted by user
     */
    public void getDeviceConfiguration(String deviceId) throws IOException, InterruptedException {
        send(new Hash("type", "getDeviceConfiguration", "deviceId", deviceId));
    }

    /**
     * Request the GuiServer to ask the device <b>deviceId</b> to send back the current full device schema.
     *
     * @param deviceId device ID
     * @throws IOException reported in case of IO problems
     * @throws InterruptedException reported if interrupted by user
     */
    public void getDeviceSchema(String deviceId) throws IOException, InterruptedException {
        send(new Hash("type", "getDeviceSchema", "deviceId", deviceId));
    }

    /**
     * Request the GuiServer to ask the device server <b>serverId</b> to send back the schema for class <b>classId</b>
     *
     * @param serverId device server ID.
     * @param classId device class ID.
     * @throws IOException reported in case of IO problems
     * @throws InterruptedException reported if interrupted by user
     */
    public void getClassSchema(String serverId, String classId) throws IOException, InterruptedException {
        send(new Hash("type", "getClassSchema", "serverId", serverId, "classId", classId));
    }

    /**
     * Send a request to device server (serverId) to instantiate a device with provided configuration via GuiServer
     *
     * @param serverId device server ID
     * @param classId  class ID
     * @param deviceId device ID
     * @param configuration device initial configuration
     * @throws IOException reported in case of IO problems
     * @throws InterruptedException reported if interrupted by user
     */
    public void initDevice(String serverId, String classId, String deviceId, Hash configuration) throws IOException, InterruptedException {
        send(new Hash("type", "initDevice", "serverId", serverId, "classId", classId, "deviceId", deviceId, "configuration", configuration));
    }

    /**
     * Send a request to kill the particular device server.
     *
     * @param serverId device server ID
     * @throws IOException reported in case of IO problems
     * @throws InterruptedException reported if interrupted by user
     */
    public void killServer(String serverId) throws IOException, InterruptedException {
        send(new Hash("type", "killServer", "serverId", serverId));
    }

    /**
     * Send a request to kill the particular device instance
     *
     * @param deviceId device instance ID
     * @throws IOException reported in case of IO problems
     * @throws InterruptedException reported if interrupted by user
     */
    public void killDevice(String deviceId) throws IOException, InterruptedException {
        send(new Hash("type", "killDevice", "deviceId", deviceId));
    }

    /**
     * Inform the GuiServer that device properties (parameters) are visible in the GUI and, therefore, GUI is interested
     * in receiving updates.  This works per device: for all device properties.
     *
     * @param deviceId device ID
     * @throws IOException reported in case of IO problems
     * @throws InterruptedException reported if interrupted by user
     */
    public void startMonitoringDevice(String deviceId) throws IOException, InterruptedException {
        if (!isVisible(deviceId)) {
            send(new Hash("type", "startMonitoringDevice", "deviceId", deviceId));
            //System.out.println("********** startMonitoringDevice ==> " + deviceId);
        }
        addVisible(deviceId);
        //System.out.println("********** increase visibility counter for " + deviceId);
    }

    /**
     * Inform GuiServer that device properties will be not visible in GUI and, therefore, GUI is not interested in receiving
     * updates. This is optimization trick that reduces network traffic if used systematically.
     *
     * @param deviceId device ID of the device the information belongs to.
     * @throws IOException reported in case of IO problems
     * @throws InterruptedException reported if interrupted by user
     */
    public void stopMonitoringDevice(String deviceId) throws IOException, InterruptedException {
        removeVisible(deviceId);
        //System.out.println("********** descrease visibility counter for " + deviceId);
        if (!isVisible(deviceId)) {
            send(new Hash("type", "stopMonitoringDevice", "deviceId", deviceId));
            //System.out.println("********** stopMonitoringDevice ==> " + deviceId);
        }
    }

    /**
     * Request the GuiServer to ask the device <b>deviceId</b> to send back the historic data for the time period [t0, t1].
     *
     * @param deviceId device ID
     * @param property property to be interested
     * @param t0 start timestamp of requested time period.
     * @param t1 end timestamp for the requested time period.
     * @param maxNumData maximum data numbers
     * @throws IOException reported in case of IO problems
     * @throws InterruptedException reported if interrupted by user
     */
    public void getPropertyHistory(String deviceId, String property, String t0, String t1, int maxNumData) throws IOException, InterruptedException {
        send(new Hash("type", "getPropertyHistory", "deviceId", deviceId, "property", property, "t0", t0, "t1", t1, "maxNumData", maxNumData));
    }
    
    /**
     * Inform GuiServer about error traceback
     * 
     * @param traceback string representing error traceback
     * @throws IOException reported in case of IO problems
     * @throws InterruptedException reported if interrupted by user
     */
    public void error(String traceback) throws IOException, InterruptedException {
        send(new Hash("type", "error", "traceback", traceback));
    }
}
