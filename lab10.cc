/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 IITP RAS
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
 * Authors: Pavel Boyko <boyko@iitp.ru>
 */

/*
 * Multirate
 *
 * Topology:
 *   3
 * 4 0 2
 *   1
 * This example illustrates the use of 
 *  - 1->3; 2->4, 4->0, 0->4 flows are valid
 */
#include "ns3/core-module.h"
#include "ns3/propagation-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/wifi-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-mac-helper.h"
#include "ns3/node-container.h"

using namespace ns3;
using namespace std;

#define MAX_NUM_NODES           5

NS_LOG_COMPONENT_DEFINE ("Lab10-Multirate");


uint32_t MacTxDropCount, PhyTxDropCount, PhyRxDropCount;

void
MacTxDrop(Ptr<const Packet> p)
{
  NS_LOG_INFO("Packet Drop");
  MacTxDropCount++;
}

void
PhyTxDrop(Ptr<const Packet> p)
{
  NS_LOG_INFO("Packet Drop");
  PhyTxDropCount++;
}
void
PhyRxDrop(Ptr<const Packet> p)
{
  NS_LOG_INFO("Packet Drop");
  PhyRxDropCount++;
}

void
PrintDrop()
{
  std::cout << "After " << Simulator::Now().GetSeconds() << " sec: \t";
  std::cout << "MacTxDropCount: " << MacTxDropCount;
  std::cout << ", PhyTxDropCount: "<< PhyTxDropCount << ", PhyRxDropCount: " << PhyRxDropCount << "\n";
  Simulator::Schedule(Seconds(5.0), &PrintDrop);
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

  cout << "[UDP TRAFFIC] src=" << src << " dst=" << dest << " rate=" << DataRate (UDPrate) << "\n";
  onOffHelper.SetConstantRate (DataRate (UDPrate), packetSize);
  onOffHelper.SetAttribute ("StartTime", TimeValue (Seconds (1.0 + dest/10.0)));
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), cbrPort));
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);

  ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get(dest));
  cbrApps.Add (onOffHelper.Install (nodes.Get (src)));
} 

int main (int argc, char **argv)
{
    uint32_t packetSize = 1420; // bytes
    string phyRate ("DsssRate11Mbps");
    int rate13 = 7000000;  // - 1->3; 2->4, 4->0, 0->4 flows are valid
    int rate24 = 7000000;  // - 1->3; 2->4, 4->0, 0->4 flows are valid
    int rate40 = 7000000;  // - 1->3; 2->4, 4->0, 0->4 flows are valid
    int rate04 = 7000000;  // - 1->3; 2->4, 4->0, 0->4 flows are valid
    int simTime = 10;      // simulation time, default 10s

    uint8_t isTcp = 0;
    bool useRtsCts = false;

    CommandLine cmd;

    cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
    cmd.AddValue ("phyMode", "Wifi Phy Rate for DATA Packets", phyRate);
    cmd.AddValue ("rate13", "UDP/TCP desired rate[bps] ", rate13);
    cmd.AddValue ("rate24", "UDP/TCP desired rate[bps] ", rate24);
    cmd.AddValue ("rate40", "UDP/TCP desired rate[bps] ", rate40);
    cmd.AddValue ("rate04", "UDP/TCP desired rate[bps] ", rate04);
    cmd.AddValue ("simTime", "Simulation time ", simTime);
    cmd.AddValue ("isTcp", "If 1 is TCP traffic if 0 is UDP", isTcp);
    cmd.AddValue ("useRtsCts", "toggle usage of RTS/CTS", useRtsCts);
    cmd.Parse (argc, argv);

    /* Setup channel in which wave propagates */
    WifiHelper wifiHelper;
    wifiHelper.SetStandard(WIFI_PHY_STANDARD_80211b);
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (2e4));

    /* Setup Physical Layer */
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
    wifiPhy.SetChannel (wifiChannel.Create ());
    wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
    wifiPhy.SetErrorRateModel ("ns3::YansErrorRateModel");
    wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-78.1));  // 550m
    //wifiPhy.Set ("RxSensitivity", DoubleValue (-82));  // typically -82 dbm RSSI for 802.11g/n/a legacy packets
    wifiHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager", 
        "DataMode",StringValue (phyRate),
        "ControlMode",StringValue ("DsssRate1Mbps"));

    /* Setup nodes and install devices */
    NodeContainer networkNodes;
    NetDeviceContainer devices;
    NetDeviceContainer apDevice, staDevice;
    networkNodes.Create (MAX_NUM_NODES);
    /* Configure AP */
    WifiMacHelper wifiMac;
    Ssid ssid = Ssid ("network");
    wifiMac.SetType ("ns3::ApWifiMac",
                    "Ssid", SsidValue (ssid));
    apDevice = wifiHelper.Install (wifiPhy, wifiMac, networkNodes.Get(0));
    devices.Add(apDevice);
    /* Configure stations */
    wifiMac.SetType ("ns3::StaWifiMac",
                "Ssid", SsidValue (ssid),
                "ActiveProbing", BooleanValue (false));
    for ( int i = 1; i < MAX_NUM_NODES; i++){           
        staDevice = wifiHelper.Install (wifiPhy, wifiMac, networkNodes.Get(i));
        devices.Add(staDevice);
    }

    /* Mobility model - set nodes position*/
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector (0.0, 0.0, 0.0));    //node 0 pos
    positionAlloc->Add (Vector (0.0, -5.0, 0.0));   //node 1 pos
    positionAlloc->Add (Vector (5.0, 0.0, 0.0));    //node 2 pos
    positionAlloc->Add (Vector (0.0, 5.0, 0.0));    //node 3 pos
    positionAlloc->Add (Vector (-5.0, 0.0, 0.0));   //node 4 pos

    mobility.SetPositionAllocator (positionAlloc);
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (networkNodes);

    /* Do enable PCAP tracing after device install to be sure you get some PCAP */
    wifiPhy.EnablePcapAll("lab10-multirate");

    /* Install TCP/IP stack & assign IP addresses */
    InternetStackHelper internet;
    internet.Install (networkNodes);
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("192.168.1.0", "255.255.255.0");
    ipv4.Assign (devices);
    /* Populate routing table */
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    /* Install app streams as per lab requirements */
    if (rate13 != 0)
        setup_flow_udp(1, 3, networkNodes, packetSize, rate13);
    if (rate24 != 0)
        setup_flow_udp(2, 4, networkNodes, packetSize, rate24);
    if (rate04 != 0)
        setup_flow_udp(0, 4, networkNodes, packetSize, rate04);
    if (rate40 != 0)
        setup_flow_udp(4, 0, networkNodes, packetSize, rate40);

    /* Install FlowMonitor on all nodes */
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll ();
    Config::SetDefault ("ns3::ConfigStore::Filename", StringValue ("output-attributes.txt"));
    Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("RawText"));
    Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Save"));
    ConfigStore outputConfig2;
    outputConfig2.ConfigureDefaults ();
    outputConfig2.ConfigureAttributes ();

    /* Install Trace for Collisions */
    /* MacTxDrop: A packet has been dropped in the MAC layer before transmission. */
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTxDrop", MakeCallback(&MacTxDrop));
    /* PhyTxDrop: Trace source indicating a packet has been dropped by the device during transmission */
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop", MakeCallback(&PhyRxDrop));
    /* PhyRxDrop: Trace source indicating a packet has been dropped by the device during reception */
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxDrop", MakeCallback(&PhyTxDrop));

    /* Run simulation for requested num of seconds */
    Simulator::Stop (Seconds (simTime));
    Simulator::Run ();

    PrintDrop();

    monitor->CheckForLostPackets ();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
        double ftime = i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds(); 
        std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
        std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
        std::cout << "  TxOffered:  " << i->second.txBytes * 8 / ftime / 1000000.0  << " Mbps\n";
        std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
        std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        std::cout << "  lostPackets:   " << i->second.lostPackets << "\n";
        std::cout << "  Throughput: " << i->second.rxBytes * 8 / ftime / 1000000.0  << " Mbps\n"; // 10 secunde (stop - start)
    }

    Simulator::Destroy ();

    return 0;
}
