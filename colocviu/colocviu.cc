 /* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
 /*
  * Copyright (c) 2015, IMDEA Networks Institute
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License version 2 as
  * published by the Free Software Foundation;
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  *
  * Author: Hany Assasa <hany.assasa@gmail.com>
 .*
  * This is a simple example to test TCP over 802.11n (with MPDU aggregation enabled).
  *
  * Network topology:
  *
  *   Ap    STA
  *   *      *
  *   |      |
  *   n1     n2
  *
  * In this example, an HT station sends TCP packets to the access point.
  * We report the total throughput received during a window of 100ms.
  * The user can specify the application data rate and choose the variant
  * of TCP i.e. congestion control algorithm to use.
  */
 
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
 #include "ns3/packet-sink.h"
 #include "ns3/packet-sink-helper.h"
 #include "ns3/tcp-westwood.h"
 #include "ns3/internet-stack-helper.h"
 #include "ns3/ipv4-address-helper.h"
 #include "ns3/ipv4-global-routing-helper.h"
 #include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/udp-header.h"
#include "ns3/llc-snap-header.h"

 
 using namespace ns3;
 
 #define RADIUS 10
 #define PI atan(1) * 4

Ptr<PacketSink> sink;                         /* Pointer to the packet sink application */
std::map<unsigned int, std::vector<int64_t>> timestamps;

// little endian if true
bool endianess;

bool checkEndianess(){
  int x = 1;

  if(*(char *)&x == 1) {
    return true;
  } 

  return false;
}

void MacRx0(Ptr<const Packet> p)
{
  Ptr<Packet> packet = p->Copy ();
  LlcSnapHeader header;
  packet->RemoveHeader(header);

  // IPv4 header
  if (header.GetType() == 0x0800){
    Ipv4Header ipHeader;
    packet->PeekHeader(ipHeader);
    Ipv4Address src = ipHeader.GetSource();

    uint32_t ip = src.Get();
    unsigned int lastByte;

    // little endian
    if (endianess == 1){
      lastByte = ip & 0xFF;
    } else {
      // big endian
      lastByte = (ip >> 24) & 0xFF;
    }

    int64_t now = Simulator::Now().GetMilliSeconds();
    if (now >= 1000){
      timestamps[lastByte].push_back(now);
    }
  }
}
 
 int
 main (int argc, char *argv[])
 {
    uint32_t numberOfNodes = 2;                        /* Number of network nodes (includes stations and AP) */
    bool runTcp = false;                               /* Run TCP/UDP traffic */
    bool enableRtsCts = false;                         /* Enable RTS/CTS mechanism */
    uint32_t payloadSize = 1472;                       /* Transport layer payload size in bytes. */
    std::string offeredRate = "12Mbps";                  /* Application layer offeredRate. */
    std::string tcpVariant = "TcpNewReno";             /* TCP variant type. */
    std::string phyRate = "ErpOfdmRate12Mbps";            /* Physical layer bitrate. */
    double simulationTime = 10;                        /* Simulation time in seconds. */
    bool pcapTracing = false;                          /* PCAP Tracing is enabled or not. */

    endianess = checkEndianess();

    /* Command line argument parser setup. */
    CommandLine cmd;
    cmd.AddValue ("numberOfNodes", "Number of nodes", numberOfNodes);
    cmd.AddValue ("runTcp", "Run either TCP or UDP", runTcp);
    cmd.AddValue ("enableRtsCts", "RTS/CTS enabled", enableRtsCts);
    cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
    cmd.AddValue ("offeredRate", "Offered TX data rate (format XMbps) for application", offeredRate);
    cmd.AddValue ("phyRate", "Physical layer bitrate", phyRate);
    cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
    cmd.AddValue ("tracing", "Enable/disable PCAP Tracing", pcapTracing);
    cmd.Parse (argc, argv);

    if (numberOfNodes < 2) {
      std::cout << "The number of nodes should be at least 2." << std::endl;
      return -1;
    }

    /* Enable/disable RTS/CTS */
    UintegerValue ctsThr = (enableRtsCts ? UintegerValue (10) : UintegerValue (2200));
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", ctsThr);

    std::string socketFactory = "ns3::UdpSocketFactory";
    if (runTcp) {
        socketFactory = "ns3::TcpSocketFactory";
          /* Configure TCP variant */
        tcpVariant = std::string ("ns3::") + tcpVariant;
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (tcpVariant)));

     
        /* Configure TCP Options */
        Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize)); 
    }


    WifiMacHelper wifiMac;
    WifiHelper wifiHelper;
    wifiHelper.SetStandard (WIFI_PHY_STANDARD_80211g);

    /* Set up Legacy Channel */
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5e9));
    /* Setup Physical Layer */
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
    wifiPhy.SetChannel (wifiChannel.Create ());
    wifiPhy.SetErrorRateModel ("ns3::YansErrorRateModel");
    wifiHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                       "DataMode", StringValue (phyRate),
                                       "ControlMode", StringValue ("ErpOfdmRate6Mbps"));
    NodeContainer networkNodes;
    networkNodes.Create (numberOfNodes);
    NodeContainer stationNodes;
    for (uint32_t i = 1; i < numberOfNodes; ++i){
        stationNodes.Add(networkNodes.Get(i));
    }
    Ptr<Node> apWifiNode = networkNodes.Get (0);

    /* Configure AP */
    Ssid ssid = Ssid ("network");
    wifiMac.SetType ("ns3::ApWifiMac",
                    "Ssid", SsidValue (ssid));

    NetDeviceContainer apDevice;
    apDevice = wifiHelper.Install (wifiPhy, wifiMac, apWifiNode);

    /* Configure STA */
    wifiMac.SetType ("ns3::StaWifiMac",
                    "Ssid", SsidValue (ssid));

    NetDeviceContainer staDevices;
    staDevices = wifiHelper.Install (wifiPhy, wifiMac, stationNodes);

    /* Mobility model */
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector (0.0, 0.0, 0.0));
    
    /* Distribute the nodes (stations) in a circle around the center (access point) */
    double step = 2 * PI / (numberOfNodes - 1);
    double x, y;
    for (uint32_t i = 1; i < numberOfNodes; i++){
      x = RADIUS * cos((i - 1) * step);
      y = RADIUS * sin((i - 1) * step);
      positionAlloc->Add (Vector (x, y, 0.0));
    }

    mobility.SetPositionAllocator (positionAlloc);
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (apWifiNode);
    mobility.Install (stationNodes);

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
    PacketSinkHelper sinkHelper (socketFactory, InetSocketAddress (Ipv4Address::GetAny (), 9));
    ApplicationContainer sinkApp = sinkHelper.Install (apWifiNode);
    sink = StaticCast<PacketSink> (sinkApp.Get (0));

    /* Install TCP/UDP Transmitter on the station */
    OnOffHelper server (socketFactory, (InetSocketAddress (apInterface.GetAddress (0), 9)));
    server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
    server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    server.SetAttribute ("DataRate", DataRateValue (DataRate (offeredRate)));
    ApplicationContainer serverApp = server.Install (stationNodes);

    /* Start Applications */
    sinkApp.Start (Seconds (0.0));
    serverApp.Start (Seconds (1.0));

    /* Enable Traces */
    if (pcapTracing)
     {
       AsciiTraceHelper ascii;
       wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("wifi-lab3.tr"));
       wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
       wifiPhy.EnablePcap ("AccessPoint", apDevice);
       wifiPhy.EnablePcap ("Station", staDevices);
     }


    Config::ConnectWithoutContext("/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx", MakeCallback(&MacRx0));

    /* Install flow monitor in order to gather stats */ 
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

    /* Start Simulation */
    Simulator::Stop (Seconds (simulationTime + 1));
    Simulator::Run ();

    monitor->CheckForLostPackets ();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      if (i->first >= 1)
        {
          Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
          double ftime = i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds(); 
          std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
          std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
          std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
          std::cout << "  TxOffered:  " << i->second.txBytes * 8 / ftime / 1000000.0  << " Mbps\n";
          std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
          std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
          std::cout << "  Throughput: " << i->second.rxBytes * 8 / ftime / 1000000.0  << " Mbps\n"; // 10 seconds (stop - start)
        }
    }

    double averageThroughput = ((sink->GetTotalRx () * 8) / (1e6 * simulationTime));

    Simulator::Destroy ();

    std::cout << "\nAverage throughput (Mbit/s):\n" << averageThroughput << "\n" << std::endl;

    std::map<unsigned int, std::vector<int64_t>>::iterator it;
    std::map<unsigned int, double> interarivalTimeAverages;
    double interarrivalTimeTotalAverage = 0;

    std::cout << "IP Address: Interarrival time average (ms)\n";

    for (it = timestamps.begin(); it != timestamps.end(); it++)
    {
      double interrarivalTotal = 0;

      for (uint32_t i = 1; i < it->second.size(); i++){
        interrarivalTotal += it->second[i] - it->second[i - 1];
      }

      interarivalTimeAverages[it->first] = interrarivalTotal / (it->second.size() - 1);
      interarrivalTimeTotalAverage += interarivalTimeAverages[it->first];

      std::cout << "10.0.0." << it->first << ": " << interarivalTimeAverages[it->first] << std::endl;
    }

    interarrivalTimeTotalAverage /= timestamps.size();
    std::cout << "\nInterarrival time total average (ms):\n" << interarrivalTimeTotalAverage << std::endl;

    return 0;
 }