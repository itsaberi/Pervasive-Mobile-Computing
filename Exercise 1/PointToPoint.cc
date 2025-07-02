#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("UdpSimulationExample");

int main(int argc, char* argv[])
{
    // Enable logging for client and server
    LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
    LogComponentEnable("UdpServer", LOG_LEVEL_INFO);

    // Create two nodes
    NodeContainer nodes;
    nodes.Create(2);

    // Install Internet stack
    InternetStackHelper internet;
    internet.Install(nodes);

    // Setup point-to-point link
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("1ms"));

    NetDeviceContainer devices = p2p.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    // Setup UDP server on node 1, port 4000
    uint16_t port1 = 4000;
    UdpServerHelper server1(port1);
    ApplicationContainer serverApp1 = server1.Install(nodes.Get(1));
    serverApp1.Start(Seconds(1.0));
    serverApp1.Stop(Seconds(10.0));

    // UDP client on node 0 targeting node 1
    UdpClientHelper client1(interfaces.GetAddress(1), port1);
    client1.SetAttribute("MaxPackets", UintegerValue(1000));
    client1.SetAttribute("Interval", TimeValue(Seconds(0.01))); // 100 packets/sec
    client1.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApp1 = client1.Install(nodes.Get(0));
    clientApp1.Start(Seconds(2.0));
    clientApp1.Stop(Seconds(10.0));

    // Optional second UDP stream on port 5000
    uint16_t port2 = 5000;
    UdpServerHelper server2(port2);
    ApplicationContainer serverApp2 = server2.Install(nodes.Get(1));
    serverApp2.Start(Seconds(1.0));
    serverApp2.Stop(Seconds(10.0));

    UdpClientHelper client2(interfaces.GetAddress(1), port2);
    client2.SetAttribute("MaxPackets", UintegerValue(1000));
    client2.SetAttribute("Interval", TimeValue(Seconds(0.01)));
    client2.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApp2 = client2.Install(nodes.Get(0));
    clientApp2.Start(Seconds(2.0));
    clientApp2.Stop(Seconds(10.0));

    // Enable PCAP tracing
    p2p.EnablePcapAll("udp-simulation");

    // Enable NetAnim output
    AnimationInterface anim("udp-simulation.xml");

    // Enable FlowMonitor
    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll();

    // Run simulation
    Simulator::Run();

    // Save FlowMonitor results
    monitor->SerializeToXmlFile("udp-flowmon.xml", true, true);

    Simulator::Destroy();

    std::cout << "\nSimulation completed.\n"
              << "Generated files:\n"
              << "  - udp-simulation-*.pcap   (packet captures)\n"
              << "  - udp-simulation.xml      (NetAnim visualization)\n"
              << "  - udp-flowmon.xml         (flow statistics)\n"
              << "Use Wireshark or NetAnim to visualize results.\n";

    return 0;
}

