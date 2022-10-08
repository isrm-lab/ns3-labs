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
#define RADIUS 1
#define PI atan(1) * 4
Ptr<PacketSink> sink;                         /* Pointer to the packet sink application */

int main (int argc, char *argv[])
{
    uint32_t tries = 1;                                /* Number of tries */
    bool enableRtsCts = false;                         /* Enable RTS/CTS mechanism */
    uint32_t payloadSize = 1460;                       /* Transport layer payload size in bytes. */
    std::string phyRate = "ErpOfdmRate6Mbps";             /* Physical layer bitrate. */
    double simulationTime = 25.0;                        /* Simulation time in seconds. */
    bool pcapTracing = true;                          /* PCAP Tracing is enabled or not. */
    int nn = 2, i;
    int minCw = 15, maxCw = 1023;
    std::string dataRate = "1Mbps";

    /* Command line argument parser setup. */
    CommandLine cmd;
    cmd.AddValue ("tries", "Number of tries", tries);
    cmd.AddValue ("enableRtsCts", "RTS/CTS enabled", enableRtsCts);
    cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
    cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
    cmd.AddValue ("pcap", "Enable/disable PCAP Tracing", pcapTracing);
    cmd.AddValue ("nn", "number of nodes", nn);
    cmd.AddValue ("minCw", "number of nodes source for transmission", minCw);
    cmd.AddValue ("maxCw", "number of nodes destination for transmission", maxCw);
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
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetErrorRateModel ("ns3::NistErrorRateModel");
    wifiPhy.SetChannel (wifiChannel.Create ());
    wifiPhy.SetErrorRateModel ("ns3::YansErrorRateModel");
    WifiHelper wifiHelper;
    wifiHelper.SetStandard (WIFI_STANDARD_80211g);
    wifiHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                       "DataMode", StringValue (phyRate),
                                       "ControlMode", StringValue ("DsssRate1Mbps"));
    NodeContainer networkNodes;
    networkNodes.Create(nn);
    NetDeviceContainer apDevice, staDevice;
    NodeContainer stationNodes;
    Ptr<Node> apWifiNode = networkNodes.Get (0);
    for (i = 1; i < nn; ++i){
        stationNodes.Add(networkNodes.Get(i));
    }

    /* Configure AP */
    WifiMacHelper wifiMac;
    Ssid ssid = Ssid ("network");
    wifiMac.SetType ("ns3::ApWifiMac",
                    "Ssid", SsidValue (ssid));
    apDevice = wifiHelper.Install (wifiPhy, wifiMac, networkNodes.Get(0));
    /* Configure stations */
    wifiMac.SetType ("ns3::StaWifiMac",
                "Ssid", SsidValue (ssid),
                "ActiveProbing", BooleanValue (false));
    NetDeviceContainer staDevices;
    staDevices = wifiHelper.Install (wifiPhy, wifiMac, stationNodes);


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

    /* Mobility model */
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector (0.0, 0.0, 0.0));

    /* Distribute the nodes (stations) in a circle around the center (access point) */
    double step = 2 * PI / (nn - 1);
    double x, y;
    for (i = 1; i < nn; i++){
        x = RADIUS * cos((i - 1) * step);
        y = RADIUS * sin((i - 1) * step);
        positionAlloc->Add (Vector (x, y, 0.0));
    }

    mobility.SetPositionAllocator (positionAlloc);
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (networkNodes);

    /* Internet stack */
    InternetStackHelper stack;
    stack.Install (networkNodes);

    Ipv4AddressHelper address;
    address.SetBase ("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer apInterface;
    apInterface = address.Assign (apDevice);
    Ipv4InterfaceContainer staInterfaces;
    staInterfaces = address.Assign (staDevices);

    /* Populate routing table */
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    /* Install TCP/UDP Receiver on the access point */
    PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9));
    ApplicationContainer sinkApp = sinkHelper.Install (apWifiNode);
    sink = StaticCast<PacketSink> (sinkApp.Get (0));

    /* Install TCP/UDP Transmitter on the station */
    DataRate dr = DataRate (dataRate);
    OnOffHelper server ("ns3::UdpSocketFactory", (InetSocketAddress (apInterface.GetAddress (0), 9)));
    server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
    server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    server.SetAttribute ("DataRate", DataRateValue (dr));
    ApplicationContainer serverApp = server.Install (stationNodes);

    /* Start Applications */
    sinkApp.Start (Seconds (0.0));
    serverApp.Start (Seconds (1.0));

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
       wifiPhy.EnablePcap ("L8-AccessPoint", apDevice);
       wifiPhy.EnablePcap ("L8-Station", staDevices);
    }
    /* Install FlowMonitor on all nodes */
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

    /* Run simulation for requested num of seconds */
    Simulator::Stop (Seconds (simulationTime));
    Simulator::Run ();

    monitor->CheckForLostPackets ();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
    uint32_t total_rx = 0, total_tx = 0, num_flows = 0;
    double tput = 0, jain_numerator = 0, jain_denominator=0, min_tput = dr.GetBitRate(), max_tput = 0;
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
        double ftime = i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds(); 
        total_rx += i->second.rxPackets;
        total_tx += i->second.txPackets;
        tput = i->second.rxBytes * 8 / ftime / 1000000.0;
        jain_numerator += tput;
        jain_denominator += tput * tput;

        if(tput < min_tput && tput != 0)
            min_tput = tput;
        if(tput > max_tput)
            max_tput = tput;

        num_flows++;

        std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
        std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
        std::cout << "  TxOffered:  " << i->second.txBytes * 8 / ftime / 1000000.0  << " Mbps\n";
        std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
        std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        std::cout << "  Throughput: " << i->second.rxBytes * 8 / ftime / 1000000.0  << " Mbps\n"; // 10 seconds (stop - start)
    }
    /* Compute Jain */
    double jain_idx;
    jain_idx = (jain_numerator * jain_numerator) / ((nn - 1) * jain_denominator);
    std::cout << "rlen jain eps " << std::endl;
    std::cout << (nn - 1) << " " << jain_idx << " " <<  (min_tput/max_tput) << std::endl;

    Simulator::Destroy ();

    return 0;
}
