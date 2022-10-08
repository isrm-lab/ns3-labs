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

 
 using namespace ns3;
 
 NS_LOG_COMPONENT_DEFINE ("Lab4");


 Ptr<PacketSink> udpSink;                         /* Pointer to the packet sink application that runs over UDP */
 Ptr<PacketSink> tcpSink;                         /* Pointer to the packet sink application that runs over TCP */
 
 uint32_t MacTxCount[4], MacRxCount[4];

void
MacTx0(Ptr<const Packet> p)
{
    NS_LOG_INFO("MAC Tx 0");
    MacTxCount[0]++;
}

void
MacTx1(Ptr<const Packet> p)
{
    NS_LOG_INFO("MAC Tx 1");
    MacTxCount[1]++;
}

void
MacTx2(Ptr<const Packet> p)
{
  NS_LOG_INFO("MAC Tx 2");
  MacTxCount[2]++;
}

void
MacTx3(Ptr<const Packet> p)
{
  NS_LOG_INFO("MAC Tx 3");
  MacTxCount[3]++;
}

void
MacRx0(Ptr<const Packet> p)
{
  NS_LOG_INFO("MAC Rx 0");
  MacRxCount[0]++;
}

void
MacRx1(Ptr<const Packet> p)
{
  NS_LOG_INFO("MAC Rx 1");
  MacRxCount[1]++;
}

void
MacRx2(Ptr<const Packet> p)
{
  NS_LOG_INFO("MAC Rx 2");
  MacRxCount[2]++;
}

void
MacRx3(Ptr<const Packet> p)
{
  NS_LOG_INFO("MAC Rx 3");
  MacRxCount[3]++;
}

 int
 main (int argc, char *argv[])
 {
    uint32_t dist = 50;                                /* Distance between the station and the AP */
    uint32_t tries = 1;                                /* Number of tries */
    bool enableRtsCts = false;                         /* Enable RTS/CTS mechanism */
    uint32_t payloadSize = 1472;                       /* Transport layer payload size in bytes. */
    std::string dataRate = "2Mbps";                    /* Application layer datarate. */
    std::string tcpVariant = "TcpNewReno";             /* TCP variant type. */
    std::string phyRate = "DsssRate2Mbps";             /* Physical layer bitrate. */
    double simulationTime = 10;                        /* Simulation time in seconds. */
    bool pcapTracing = false;                          /* PCAP Tracing is enabled or not. */
    AsciiTraceHelper ascii;

    /* Command line argument parser setup. */
    CommandLine cmd;
    cmd.AddValue("dist", "Distance between the station and the AP", dist);
    cmd.AddValue("tries", "Number of tries", tries);
    cmd.AddValue("enableRtsCts", "RTS/CTS enabled", enableRtsCts);
    cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
    cmd.AddValue ("dataRate", "Application data rate", dataRate);
    cmd.AddValue ("phyRate", "Physical layer bitrate", phyRate);
    cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
    cmd.AddValue ("pcap", "Enable/disable PCAP Tracing", pcapTracing);
    cmd.Parse (argc, argv);

    /* Enable/disable RTS/CTS */
    UintegerValue ctsThr = (enableRtsCts ? UintegerValue (10) : UintegerValue (2200));
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", ctsThr);

    /* Configure TCP variant */
    tcpVariant = std::string ("ns3::") + tcpVariant;
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (tcpVariant)));

    /* Configure TCP Options */
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize)); 

    WifiMacHelper wifiMac;
    WifiHelper wifiHelper;
    wifiHelper.SetStandard (WIFI_STANDARD_80211b);

    /* Set up Legacy Channel */
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    // TODO - add three models
    wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5e9)); 
    // wifiChannel.AddPropagationLoss ("ns3::TwoRayGroundPropagationLossModel", "Frequency", DoubleValue (2.4e9), "MinDistance", DoubleValue (10.0), "HeightAboveZ", DoubleValue (0.5));
    // wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel", "ReferenceDistance", DoubleValue (1.0), "Exponent", DoubleValue(1.6), "ReferenceLoss", DoubleValue(46.7));

    /* Setup Physical Layer */
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetErrorRateModel ("ns3::NistErrorRateModel");
    wifiPhy.SetChannel (wifiChannel.Create ());
    wifiPhy.SetErrorRateModel ("ns3::YansErrorRateModel");
    wifiHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                       "DataMode", StringValue (phyRate),
                                       "ControlMode", StringValue ("DsssRate1Mbps"),
                                       "MaxSsrc", UintegerValue (tries),
                                       "MaxSlrc", UintegerValue (tries));
    // wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-78.1));  // 550m
    // wifiPhy.Set ("TxGain", DoubleValue (0.281838));
    // wifiPhy.Set ("RxGain", DoubleValue (3.65262e-10));

    NodeContainer networkNodes;
    networkNodes.Create(4);
    NodeContainer apNodes;
    NodeContainer stationNodes;
    for (uint32_t i = 0; i < 4; i++){
        if (i % 2 == 0){
            apNodes.Add(networkNodes.Get(i));
        }
        else
            stationNodes.Add(networkNodes.Get(i));
    }

    /* Configure AP */
    Ssid ssid = Ssid ("network");
    wifiMac.SetType ("ns3::ApWifiMac",
                    "Ssid", SsidValue (ssid));
    NetDeviceContainer apDevices;
    apDevices = wifiHelper.Install (wifiPhy, wifiMac, apNodes);

    /* Configure STA */
    wifiMac.SetType ("ns3::StaWifiMac",
                    "Ssid", SsidValue (ssid));

    NetDeviceContainer staDevices;
    staDevices = wifiHelper.Install (wifiPhy, wifiMac, stationNodes);

    /* Mobility model */
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector (0.0, 0.0, 0.0));
    positionAlloc->Add (Vector (10000.0, 0.0, 0.0));
    positionAlloc->Add (Vector (0.0 + dist, 0.0, 0.0));
    positionAlloc->Add (Vector (10000.0 + dist, 0.0, 0.0));

    
    mobility.SetPositionAllocator (positionAlloc);
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (stationNodes);
    mobility.Install (apNodes);

    wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("lab4.tr"));    

    Packet::EnablePrinting ();
    /* Internet stack */
    InternetStackHelper stack;
    stack.Install (networkNodes);

    Ipv4AddressHelper address;
    address.SetBase ("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer apInterfaces;
    apInterfaces = address.Assign (apDevices);
    Ipv4InterfaceContainer staInterfaces;
    staInterfaces = address.Assign (staDevices);

    /* Populate routing table */
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    /* Install UDP/TCP Receivers on the access points */
    PacketSinkHelper udpSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9));
    ApplicationContainer udpSinkApps = udpSinkHelper.Install(apNodes.Get(0));
    udpSink = StaticCast<PacketSink> (udpSinkApps.Get (0));
    PacketSinkHelper tcpSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9));
    ApplicationContainer tcpSinkApps = tcpSinkHelper.Install(apNodes.Get(1));
    tcpSink = StaticCast<PacketSink> (tcpSinkApps.Get (0));

    /* Install UDP Transmitter on the first station */
    OnOffHelper udpServer ("ns3::UdpSocketFactory", (InetSocketAddress (apInterfaces.GetAddress (0), 9)));
    udpServer.SetAttribute ("PacketSize", UintegerValue (payloadSize));
    udpServer.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    udpServer.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    udpServer.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
    ApplicationContainer udpServerApp = udpServer.Install (stationNodes.Get(0));

    /* Install TCP Transmitter on the second station */
    OnOffHelper tcpServer ("ns3::TcpSocketFactory", (InetSocketAddress (apInterfaces.GetAddress (1), 9)));
    tcpServer.SetAttribute ("PacketSize", UintegerValue (payloadSize));
    tcpServer.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    tcpServer.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    tcpServer.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
    ApplicationContainer tcpServerApp = tcpServer.Install (stationNodes.Get(1));

    /* Start Applications */
    udpSinkApps.Start (Seconds (0.0));
    tcpSinkApps.Start (Seconds (0.1));
    udpServerApp.Start (Seconds (1.0));
    tcpServerApp.Start (Seconds (1.1));

    /* Enable Traces */
    if (pcapTracing)
     {
       wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
       wifiPhy.EnablePcap ("AccessPoint", apDevices);
       wifiPhy.EnablePcap ("Station", staDevices);
     }

    /* Install flow monitor in order to gather stats */ 
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

    Config::ConnectWithoutContext("/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx", MakeCallback(&MacTx0));
    Config::ConnectWithoutContext("/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx", MakeCallback(&MacRx0));
    Config::ConnectWithoutContext("/NodeList/1/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx", MakeCallback(&MacTx1));
    Config::ConnectWithoutContext("/NodeList/1/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx", MakeCallback(&MacRx1));
    Config::ConnectWithoutContext("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx", MakeCallback(&MacTx2));
    Config::ConnectWithoutContext("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx", MakeCallback(&MacRx2));
    Config::ConnectWithoutContext("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx", MakeCallback(&MacTx3));
    Config::ConnectWithoutContext("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx", MakeCallback(&MacRx3));

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
          std::cout << "  Lost Packets: " << i->second.lostPackets << "\n";
          std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
          std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
          std::cout << "  TxOffered:  " << i->second.txBytes * 8 / ftime / 1000000.0  << " Mbps\n";
          std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
          std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
          std::cout << "  Throughput: " << i->second.rxBytes * 8 / ftime / 1000000.0  << " Mbps\n"; // 10 seconds (stop - start)
        }
    }

    double udpAverageThroughput = ((udpSink->GetTotalRx () * 8) / (1e6 * simulationTime));
    double tcpAverageThroughput = ((tcpSink->GetTotalRx () * 8) / (1e6 * simulationTime));

    Simulator::Destroy ();

    std::cout << "\nUDP Average throughput: " << udpAverageThroughput << " Mbit/s" << std::endl;
    std::cout << "\nTCP Average throughput: " << tcpAverageThroughput << " Mbit/s" << std::endl;
    std::cout << "MAC TX0=" << MacTxCount[0] << ", MAC RX0=" << MacRxCount[0] << std::endl;
    std::cout << "MAC TX1=" << MacTxCount[1] << ", MAC RX1=" << MacRxCount[1] << std::endl;
    std::cout << "MAC TX2=" << MacTxCount[2] << ", MAC RX2=" << MacRxCount[2] << std::endl;
    std::cout << "MAC TX3=" << MacTxCount[3] << ", MAC RX3=" << MacRxCount[3] << std::endl;

    return 0;
 }
