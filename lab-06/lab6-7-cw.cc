#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/mobility-module.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/tcp-westwood.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/propagation-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/wifi-mac-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Lab67-CW");

std::map<std::string, int> DsssToRate = {
    {"DsssRate1Mbps", 1000000}, 
    {"DsssRate2Mbps", 2000000}, 
    {"DsssRate5.5Mbps", 5500000}, 
    {"DsssRate11Mbps", 11000000}
};


uint32_t MacTxDropCount, PhyTxDropCount, PhyRxDropCount;
uint32_t MacTx, MacRx;

void MacTxDrop(Ptr<const Packet> p)
{
  NS_LOG_INFO("Packet Drop");
  MacTxDropCount++;
}

void PhyTxDrop(Ptr<const Packet> p)
{
  NS_LOG_INFO("Packet Drop");
  PhyTxDropCount++;
}

void PhyRxDrop(Ptr<const Packet> p)
{
  NS_LOG_INFO("Packet Drop");
  PhyRxDropCount++;
}

void MacTxDone(Ptr<const Packet> p)
{
  NS_LOG_INFO("Transmitted MAC");
  MacTx++;
}

void MacRxDone(Ptr<const Packet> p)
{
  NS_LOG_INFO("Received MAC");
  MacRx++;
}


void PrintLMACStats()
{
  std::cout << "After " << Simulator::Now().GetSeconds() << " sec: \t";
  std::cout << "MacTx: " << MacTx << ", MacRx: " << MacRx;
  std::cout << ", MacTxDropCount: " << MacTxDropCount;
  std::cout << ", PhyTxDropCount: "<< PhyTxDropCount << ", PhyRxDropCount: " << PhyRxDropCount << std::endl;
  Simulator::Schedule(Seconds(5.0), &PrintLMACStats);
}

void setup_flow_udp(int src, int dest, NodeContainer nodes, int packetSize, int UDPrate) 
{
  ApplicationContainer cbrApps;
  uint16_t cbrPort = 12345;
   
  Ptr <Node> PtrNode = nodes.Get(dest);
  Ptr<Ipv4> ipv4 = PtrNode->GetObject<Ipv4> ();
  Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1, 0); 
  Ipv4Address ipAddrDest = iaddr.GetLocal ();

  OnOffHelper onOffHelper ("ns3::UdpSocketFactory", InetSocketAddress (ipAddrDest, cbrPort));
  onOffHelper.SetAttribute ("PacketSize", UintegerValue (packetSize));

  std::cout << "[UDP TRAFFIC] src=" << src << " dst=" << dest << " rate=" << DataRate (UDPrate) << "\n";
  onOffHelper.SetConstantRate (DataRate (UDPrate));
  onOffHelper.SetAttribute ("PacketSize", UintegerValue (packetSize));

  onOffHelper.SetAttribute ("StartTime", TimeValue (Seconds (1.0 + dest/10.0)));
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), cbrPort));
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);

  ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get(dest));
  cbrApps.Add (onOffHelper.Install (nodes.Get (src)));
} 

int main (int argc, char *argv[])
{
    bool enableRtsCts = false;                         /* Enable RTS/CTS mechanism */
    uint32_t payloadSize = 1460;                       /* Transport layer payload size in bytes. */
    std::string phyRate = "ErpOfdmRate6Mbps";             /* Physical layer bitrate. */
    double simulationTime = 25.0;                        /* Simulation time in seconds. */
    bool pcapTracing = true;                          /* PCAP Tracing is enabled or not. */
    int ns = 2, nd = 2, i, j;
    int minCw = 15, maxCw = 1023;
    int udpRate = 1000000;

    /* Command line argument parser setup. */
    CommandLine cmd;
    cmd.AddValue ("enableRtsCts", "RTS/CTS enabled", enableRtsCts);
    cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
    cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
    cmd.AddValue ("pcap", "Enable/disable PCAP Tracing", pcapTracing);
    cmd.AddValue ("ns", "number of nodes source for transmission", ns);
    cmd.AddValue ("nd", "number of nodes destination for transmission", nd);
    cmd.AddValue ("minCw", "minimum contention window size", minCw);
    cmd.AddValue ("maxCw", "maximum contention window size", maxCw);
    cmd.AddValue ("phyRate", "Physical layer data rate (MCS)", phyRate);
    cmd.AddValue ("udpRate", "UDP TX desired delivered bitrate (bps)", udpRate);
    cmd.Parse (argc, argv);

    /* Enable/disable RTS/CTS */
    if (enableRtsCts)
    {
      Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
    }
    /* Set up Legacy Channel */
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (2e4));
    /* Setup Physical Layer */
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
    wifiPhy.SetChannel (wifiChannel.Create ());
    wifiPhy.SetErrorRateModel ("ns3::YansErrorRateModel");
    WifiHelper wifiHelper;
    wifiHelper.SetStandard (WIFI_PHY_STANDARD_80211g);
    wifiHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                       "DataMode", StringValue (phyRate),
                                       "ControlMode", StringValue ("DsssRate1Mbps"));
    NodeContainer networkNodes;
    int nn = ns+nd;
    networkNodes.Create(nn);

    WifiMacHelper wifiMac;
    wifiMac.SetType ("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifiHelper.Install (wifiPhy, wifiMac, networkNodes);

    /* setup contention window */
    for(i = 0; i < nn; i++) {
        Ptr<NetDevice> dev = networkNodes.Get(i)->GetDevice(0); //not sure if getDevice of 0 or other index?!??!
        Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice>(dev);
        Ptr<WifiMac> mac = wifi_dev->GetMac();
        PointerValue ptr;
        mac->GetAttribute("Txop", ptr);
        Ptr<Txop> dca = ptr.Get<Txop>();
        dca->SetMinCw(minCw);
        dca->SetMaxCw(maxCw);
        dca->SetAifsn(3); //https://mrncciew.files.wordpress.com/2014/10/cwap-contention-02.png
        dca->SetTxopLimit(Time(0)); //Value zero corresponds to default DCF
    }

    /* Provide initial (X,Y, Z=0) co-ordinates for mobilenodes */
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    for (i = 0; i < ns; i++) {
        //std::cout << i <<std::endl;
        positionAlloc->Add (Vector (i * 10.0 / ns, 0.0, 0.0));
    }
    j = i;
    for(i = 0; i < nd; i++,j++) {
        //std::cout << i << " ........ j=" << j<<std::endl;
        positionAlloc->Add (Vector (i * 10.0 / nd, 10.0, 0.0));
    }
    mobility.SetPositionAllocator (positionAlloc);
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install(networkNodes);

    /* Install TCP/IP stack & assign IP addresses */
    InternetStackHelper internet;
    internet.Install (networkNodes);
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("192.168.1.0", "255.255.255.0");
    ipv4.Assign (devices);
    /* Populate routing table */
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    for(i = 0; i < ns; i++) {
        setup_flow_udp(i, ns + i % nd, networkNodes, payloadSize, udpRate);
    }

    /* Enable Traces */
    Config::SetDefault ("ns3::ConfigStore::Filename", StringValue ("output-attributes.txt"));
    Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("RawText"));
    Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Save"));
    ConfigStore outputConfig2;
    outputConfig2.ConfigureDefaults ();
    outputConfig2.ConfigureAttributes ();
    if (pcapTracing)
    {
        wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
        wifiPhy.EnablePcap ("L67-CW-comm", devices);
    }
    /* Install FlowMonitor on all nodes */
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

    /* Install Trace for Collisions */
    /* MacTxDrop: A packet has been dropped in the MAC layer before transmission. */
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTxDrop", MakeCallback(&MacTxDrop));
    /* PhyTxDrop: Trace source indicating a packet has been dropped by the device during transmission */
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop", MakeCallback(&PhyRxDrop));
    /* PhyRxDrop: Trace source indicating a packet has been dropped by the device during reception */
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxDrop", MakeCallback(&PhyTxDrop));

    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx", MakeCallback(&MacTxDone));
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx", MakeCallback(&MacRxDone));


    /* Run simulation for requested num of seconds */
    Simulator::Stop (Seconds (simulationTime));
    Simulator::Run ();

    //PrintDrop();

    monitor->CheckForLostPackets ();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
    uint32_t total_rx = 0, total_tx = 0, num_flows = 0;
    double tput = 0;
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
        double ftime = i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds(); 
        total_rx += i->second.rxPackets;
        total_tx += i->second.txPackets;
        tput += i->second.rxBytes * 8 / ftime / 1000000.0;
        num_flows++;

        std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
        std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
        std::cout << "  TxOffered:  " << i->second.txBytes * 8 / ftime / 1000000.0  << " Mbps\n";
        std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
        std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        std::cout << "  Throughput: " << i->second.rxBytes * 8 / ftime / 1000000.0  << " Mbps\n"; // 10 seconds (stop - start)
    }

    PrintLMACStats();
    double udpAverageThroughput = tput / num_flows;

    std::cout << "sent_agt recv_agt col_cbr sent_mac recv_mac avgTput" << std::endl;
    std::cout << total_tx << " " << total_rx << " " << (MacTxDropCount+PhyTxDropCount+PhyRxDropCount) << " ";
    std::cout << MacTx << " " << MacRx << " " << udpAverageThroughput << std::endl;

    Simulator::Destroy ();

    return 0;
}