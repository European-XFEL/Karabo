
import java.io.IOException;
import java.util.Iterator;
import java.util.Map.Entry;
import karabo.guiclient.GuiClient;
import karabo.util.Hash;
import karabo.util.Node;
import karabo.util.Registrator;
import karabo.util.Schema;
import karabo.util.vectors.VectorHash;
import karabo.util.vectors.VectorString;
import org.apache.log4j.BasicConfigurator;
import org.apache.log4j.ConsoleAppender;
import org.apache.log4j.Level;
import org.apache.log4j.Logger;
import org.apache.log4j.PatternLayout;

/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class GuiClientMain {

    static {
        Registrator.registerAll();
    }

    private static class GuiController extends GuiClient {

        @Override
        protected void onConnect() {
            //System.out.println("connection to GuiServerDevice established.");
        }

        @Override
        protected void onDisconnect() {
            //System.out.println("Connection to GuiServerDevice lost.");
        }

        protected void onInitReply(String deviceId, boolean success, String message) {
            if (success)
                System.out.println("onInitReply(...)  :  deviceId = \"" + deviceId + "\" is OK.");
            else
                System.out.println("onInitReply(...)  :  deviceId = \"" + deviceId + "\" failed : " + message);
        }
        
        @Override
        protected void onSystemVersion(String systemVersion) {
            System.out.println("onSystemVersion() : System version is \"" +  systemVersion + "\"");
        }

        @Override
        protected void onSystemTopology(Hash systemTopology) {
            System.out.println("onSystemTopology ...");
            if (!systemTopology.has("server")) {
                return;
            }
            Hash serverHash = systemTopology.<Hash>get("server");
            Hash deviceHash = null;
            if (systemTopology.has("device")) {
                deviceHash = systemTopology.<Hash>get("device");
            }
            Iterator<Entry<String, Node>> servers = serverHash.entrySet().iterator();
            while (servers.hasNext()) {
                Node serverNode = servers.next().getValue();
                String serverId = serverNode.getAttribute("serverId");
                assert serverNode.getAttribute("type").equals("server");

                VectorString deviceClasses = serverNode.getAttribute("deviceClasses");
                for (String classId : deviceClasses) {
                    if (classId.equals("FileDataLogger") || classId.equals("GuiServerDevice")) {
                        continue;
                    }
                    System.out.println("\t" + serverId + " " + classId);
                    try {
                        getClassSchema(serverId, classId);
                    } catch (IOException ex) {
                        LOG.error(ex);
                    } catch (InterruptedException ex) {
                        LOG.warn(ex);
                    }
                    if (deviceHash == null) {
                        continue;
                    }
                    Iterator<Entry<String, Node>> it = deviceHash.entrySet().iterator();
                    while (it.hasNext()) {
                        Node deviceNode = it.next().getValue();
                        assert deviceNode.getAttribute("type").equals("device");
                        String deviceId = deviceNode.getKey();
                        String sid = deviceNode.getAttribute("serverId");
                        String cid = deviceNode.getAttribute("classId");
                        String status = deviceNode.getAttribute("status");
                        if (!sid.equals(serverId) && !cid.equals(classId)) {
                            continue;
                        }
                        System.out.println("\t\t" + deviceId + "\t" + status);
                        try {
                            getDeviceSchema(deviceId);
//                            refreshInstance(deviceId);
                            startMonitoringDevice(deviceId);
                        } catch (IOException ex) {
                            LOG.error(ex);
                        } catch (InterruptedException ex) {
                            LOG.warn(ex);
                        }
                    }
                }
            }
            //System.out.println("onSystemTopology...\n" + systemTopology);
        }

        @Override
        protected void onDeviceConfiguration(String deviceId, Hash configuration) {
            System.out.println("**** onDeviceConfiguration: deviceId=" + deviceId + "... " + "CONFIGURATION content");
        }

        @Override
        protected void onClassSchema(String serverId, String classId, Schema schema) {
            System.out.println("**** onClassSchema: serverId=" + serverId + ", classId=" + classId + " ... " + "SCHEMA content");
        }

        @Override
        protected void onDeviceSchema(String deviceId, Schema schema, Hash configuration) {
            if (configuration.empty())
                System.out.println("**** onDeviceSchema: deviceId=" + deviceId + " ... " + "SCHEMA content");
            else
                System.out.println("**** onDeviceSchema: deviceId=" + deviceId + " ... " + "SCHEMA content" + " and configuration");
        }

        @Override
        protected void onPropertyHistory(String deviceId, String property, VectorHash data) {
            System.out.println("**** onPropertyHistory: deviceId=" + deviceId + ", property=" + property + "   VectorHash historic data");
        }

        @Override
        protected void onInstanceNew(Hash topologyEntry) {
            System.out.println("**** onInstanceNew: topologyEntry is ...\n" + topologyEntry);
            if (topologyEntry.has("device")) {
                Hash deviceHash = topologyEntry.<Hash>get("device");
                Node node = deviceHash.entrySet().iterator().next().getValue();
                String deviceId = node.getKey();
                String type = node.getAttribute("type");
                String serverId = node.getAttribute("serverId");
                String classId = node.getAttribute("classId");
                String status = node.getAttribute("status");
                System.out.println("**** onInstanceNew: type=\"" + type + "\", deviceId=" + deviceId + ", serverId=" + serverId + ", classId=" + classId + ", status=" + status);
                try {
                    Thread.sleep(50);      // Give some time for device startup
                    getDeviceSchema(deviceId);
                    // register device monitor to get data updates automatically
                    startMonitoringDevice(deviceId);
                } catch (IOException ex) {
                    LOG.error(ex);
                } catch (InterruptedException ex) {
                    LOG.warn(ex);
                }
            } else if (topologyEntry.has("server")) {
                Hash serverHash = topologyEntry.<Hash>get("server");
                Node node = serverHash.entrySet().iterator().next().getValue();
                String serverId = node.getKey();
                String type = node.getAttribute("type");
                String host = node.getAttribute("host");
                String version = node.getAttribute("version");
                System.out.println("**** onInstanceNew: type=\"" + type + "\", serverId=" + serverId + ", host=" + host + ", version=" + version);
            }
        }

        @Override
        protected void onInstanceUpdated(Hash topologyEntry) {
            System.out.println("**** onInstanceUpdated: topologyEntry...\n" + topologyEntry);
            if (topologyEntry.has("device")) {
                Hash deviceHash = topologyEntry.<Hash>get("device");
                Node node = deviceHash.entrySet().iterator().next().getValue();
                String deviceId = node.getKey();
                String serverId = node.getAttribute("serverId");
                String classId = node.getAttribute("classId");
                String status = node.getAttribute("status");
                System.out.println("**** onInstanceUpdated: deviceId=" + deviceId + ", serverId=" + serverId + ", classId=" + classId + ", status=" + status);
                try {
                    Thread.sleep(50);      // Give some time for device startup
                    getDeviceSchema(deviceId);
                    startMonitoringDevice(deviceId);
                } catch (IOException ex) {
                    LOG.error(ex);
                } catch (InterruptedException ex) {
                    LOG.warn(ex);
                }
            } else if (topologyEntry.has("server")) {
                Hash serverHash = topologyEntry.<Hash>get("server");
                Node node = serverHash.entrySet().iterator().next().getValue();
                String serverId = node.getKey();
                String type = node.getAttribute("type");
                String host = node.getAttribute("host");
                String version = node.getAttribute("version");
                System.out.println("**** onInstanceUpdate: type=\"" + type + "\", serverId=" + serverId + ", host=" + host + ", version=" + version);
            }
        }

        @Override
        protected void onInstanceGone(String instanceId, String instanceType) {
            try {
                stopMonitoringDevice(instanceId);
            } catch (IOException ex) {
                LOG.error(ex);
            } catch (InterruptedException ex) {
                LOG.warn(ex);
            }
            System.out.println("onInstanceGone: instanceId=" + instanceId);
        }

        @Override
        protected void onNotification(String deviceId, String messageType, String shortMsg, String detailedMsg) {
            System.out.println("*** Notification: deviceId=" + deviceId + ", messageType=" + messageType + " : " + shortMsg + " -- " + detailedMsg);
        }

        @Override
        protected void onLogging(VectorHash message) {
            
            //System.out.println("onLogging: message is " + message);
        }
        
        @Override
        protected void onAvailableProjects(Hash projects) {
            System.out.println("onAvailableProjects  ...\n" + projects);
        }
        
        @Override
        protected void onNewProject(String projectName, boolean success, char[] data) {
            System.out.println("onNewProject(...)  :  project = \"" + projectName + "\"");
        }
        
        @Override
        protected void onLoadProject(String projectName, Hash metaData, char[] data) {
            System.out.println("onLoadProject(...)  :  project = \"" + projectName + "\" and metaData ...\n" + metaData);
        }
        
        @Override
        protected void onSaveProject(String projectName, boolean success, char[] data) {
            System.out.println("onSaveProject(...)  :  project = \"" + projectName + "\"");
        }
        
        @Override
        protected void onCloseProject(String projectName, boolean success, char[] data) {
            System.out.println("onCloseProject(...)  :  project = \"" + projectName + "\"");
        }
        
        protected void onNetworkData(String name, Hash data) {
            System.out.println("onNetworkData(...)  :  " + name + " ->\n" + data);
        }
    }

    public static void main(String[] args) {
        BasicConfigurator.configure(new ConsoleAppender(new PatternLayout("%d{yyyyMMdd-HH:mm:ss} %-10t %-5p %-20C{1} - %m%n")));
        Logger.getRootLogger().setLevel(Level.INFO);
        final Logger LOG = Logger.getLogger(GuiClientMain.class);

        while (true) {

            final GuiController client = new GuiController();
            client.setAddress("localhost", 44444);

            try {
                // Start communication thread
                client.start();
                // Wait until coonection established
                while (!client.isConnected()) {
                    Thread.sleep(500);
                }
                System.out.println("GUI client successfully connected.\nSend login information: \"operator\", \"\", \"LOCAL\"");
                // Provide login info (for the time being, default one)
                client.login("operator", "karabo", "", "LOCAL");
                // All other communications will happen in thread
                // main thread just wait for joining
                client.join();
            } catch (InterruptedException ex) {
                LOG.info(ex);
            } catch (IOException ex) {
                LOG.fatal(ex);
            }
        }

    }
}
