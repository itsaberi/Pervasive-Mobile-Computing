
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

void CwndChange(Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd) {
    *stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << newCwnd / 1024.0 << std::endl;
}

int main() {
    Time::SetResolution(Time::NS);
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpNewReno::GetTypeId()));

    NodeContainer leftNodes, rightNodes, bridgeNodes;
    leftNodes.Create(2);    
    rightNodes.Create(2);   
    bridgeNodes.Create(2);  

    InternetStackHelper stack;
    stack.InstallAll();

    PointToPointHelper leafToBridge;
    leafToBridge.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    leafToBridge.SetChannelAttribute("Delay", StringValue("2ms"));

    PointToPointHelper bridgeToBridge;
    bridgeToBridge.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    bridgeToBridge.SetChannelAttribute("Delay", StringValue("5ms"));

    NetDeviceContainer d1 = leafToBridge.Install(leftNodes.Get(0), bridgeNodes.Get(0));
    NetDeviceContainer d2 = leafToBridge.Install(leftNodes.Get(1), bridgeNodes.Get(0));
    NetDeviceContainer d3 = bridgeToBridge.Install(bridgeNodes);
    NetDeviceContainer d4 = leafToBridge.Install(bridgeNodes.Get(1), rightNodes.Get(0));
    NetDeviceContainer d5 = leafToBridge.Install(bridgeNodes.Get(1), rightNodes.Get(1));

    Ipv4AddressHelper address;
    Ipv4InterfaceContainer i1, i2, i3, i4, i5;

    address.SetBase("10.1.1.0", "255.255.255.0"); i1 = address.Assign(d1);
    address.SetBase("10.1.2.0", "255.255.255.0"); i2 = address.Assign(d2);
    address.SetBase("10.1.3.0", "255.255.255.0"); i3 = address.Assign(d3);
    address.SetBase("10.1.4.0", "255.255.255.0"); i4 = address.Assign(d4);
    address.SetBase("10.1.5.0", "255.255.255.0"); i5 = address.Assign(d5);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // TCP Server
    uint16_t tcpPort = 5000;
    Address tcpAddress(InetSocketAddress(i4.GetAddress(1), tcpPort));
    PacketSinkHelper tcpSink("ns3::TcpSocketFactory", tcpAddress);
    ApplicationContainer tcpServerApp = tcpSink.Install(rightNodes.Get(0));
    tcpServerApp.Start(Seconds(0.0));
    tcpServerApp.Stop(Seconds(50.0));

    // TCP Client
    BulkSendHelper tcpClient("ns3::TcpSocketFactory", tcpAddress);
    tcpClient.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer tcpClientApp = tcpClient.Install(leftNodes.Get(0));
    tcpClientApp.Start(Seconds(1.0));
    tcpClientApp.Stop(Seconds(50.0));

    // UDP Server
    uint16_t udpPort = 4000;
    PacketSinkHelper udpSink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), udpPort));
    ApplicationContainer udpServerApp = udpSink.Install(rightNodes.Get(1));
    udpServerApp.Start(Seconds(0.0));
    udpServerApp.Stop(Seconds(50.0));

    // UDP Client
    OnOffHelper udpClient("ns3::UdpSocketFactory", InetSocketAddress(i5.GetAddress(1), udpPort));
    udpClient.SetAttribute("PacketSize", UintegerValue(1024));
    udpClient.SetAttribute("DataRate", StringValue("2.5Mbps"));
    udpClient.SetAttribute("StartTime", TimeValue(Seconds(20.0)));
    udpClient.SetAttribute("StopTime", TimeValue(Seconds(50.0)));
    ApplicationContainer udpApp = udpClient.Install(leftNodes.Get(1));

    // Schedule increase in UDP rate
    Simulator::Schedule(Seconds(30.0), [&udpClient]() {
        udpClient.SetAttribute("DataRate", StringValue("5Mbps"));
    });

    // Setup trace AFTER application installed
    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream("cwnd-trace.txt");
    Ptr<BulkSendApplication> bulkApp = DynamicCast<BulkSendApplication>(tcpClientApp.Get(0));
    bulkApp->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback(&CwndChange, stream));

    Simulator::Stop(Seconds(55.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
