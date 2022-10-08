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
 * Classical hidden terminal problem and its RTS/CTS solution.
 *
 * Topology: [node 0] <-- -50 dB --> [node 1] <-- -50 dB --> [node 2]
 * 
 * This example illustrates the use of 
 *  - Wifi in ad-hoc mode
 *  - Matrix propagation loss model
 *  - Use of OnOffApplication to generate CBR stream 
 *  - IP flow monitor
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

using namespace ns3;
using namespace std;


void setup_flow_udp(int src, int dest, NodeContainer nodes, int packetSize, int UDPrate) 
{
  ApplicationContainer cbrApps;
  uint16_t cbrPort = 12345;
   
  Ptr <Node> PtrNode = nodes.Get(dest);
  Ptr<Ipv4> ipv4 = PtrNode->GetObject<Ipv4> ();
  Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1, 0); 
  Ipv4Address ipAddrDest = iaddr.GetLocal ();

  OnOffHelper onOffHelper ("ns3::UdpSocketFactory", InetSocketAddress (ipAddrDest, cbrPort));
  //  onOffHelper.SetAttribute ("PacketSize", UintegerValue (packetSize));
  // onOffHelper.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));

  /* \internal
   * The slightly different start times and data rates are a workaround
   * for \bugid{388} and \bugid{912}
   */
  cout << "[UDP TRAFFIC] src=" << src << " dst=" << dest << " rate=" << DataRate (UDPrate + dest*100) << "\n";
  onOffHelper.SetConstantRate (DataRate (UDPrate), packetSize);
  onOffHelper.SetAttribute ("StartTime", TimeValue (Seconds (1.0 + dest/10.0)));
 // Create a packet sink to receive these packets
  ///Address sinkLocalAddress(InetSocketAddress (ipAddr, port));
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), cbrPort));
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);

  ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get(dest));
  cbrApps.Add (onOffHelper.Install (nodes.Get (src)));
} 

void setup_flow_tcp(int src, int dest, NodeContainer nodes, int packetSize, int UDPrate)
{
  ApplicationContainer tcpApps;
  uint16_t tcpPort = 12345;
   
  Ptr <Node> PtrNode = nodes.Get(dest);
  Ptr<Ipv4> ipv4 = PtrNode->GetObject<Ipv4> ();
  Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1, 0); 
  Ipv4Address ipAddrDest = iaddr.GetLocal ();

  OnOffHelper onOffHelper ("ns3::TcpSocketFactory", InetSocketAddress (ipAddrDest, tcpPort));
  onOffHelper.SetAttribute ("PacketSize", UintegerValue (packetSize));

  onOffHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));

  /* \internal
   * The slightly different start times and data rates are a workaround
   * for \bugid{388} and \bugid{912}
   */
  cout << "[TCP] src=" << src << " dst=" << dest << endl;
  onOffHelper.SetConstantRate (DataRate (UDPrate + dest*101), packetSize);
  onOffHelper.SetAttribute ("StartTime", TimeValue (Seconds (1.0 + dest/10.0)));
 // Create a packet sink to receive these packets
  ///Address sinkLocalAddress(InetSocketAddress (ipAddr, port));
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), tcpPort));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);

  ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get(dest));
  tcpApps.Add (onOffHelper.Install (nodes.Get (src)));

}

/// Run single 10 seconds experiment with enabled or disabled RTS/CTS mechanism
void experiment (bool enableCtsRts, uint32_t packetSize, 
                 string phyMode, int UDPrate, int ns, int nd, int tcp = 0)
{
  int nn = ns + nd; //number of nodes

  // 0. Enable or disable CTS/RTS
  UintegerValue ctsThr = (enableCtsRts ? UintegerValue (10) : UintegerValue (2200));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", ctsThr);
 
  // 1. Create 2 nodes 
  NodeContainer nodes;
  nodes.Create (nn);

  // 2. Place nodes somehow, this is required by every wireless simulation
  for (int i = 0; i < nn; ++i)
  {
    nodes.Get (i)->AggregateObject (CreateObject<ConstantPositionMobilityModel> ());
  }

  // 3. Create propagation loss matrix
  Ptr<MatrixPropagationLossModel> lossModel = CreateObject<MatrixPropagationLossModel> ();
  lossModel->SetDefaultLoss (200); // set default loss to 200 dB (no link)
  for ( int i = 0; i < nn; i++)
    for( int j =0; j < nn; j ++) {
      if( i != j)
        lossModel->SetLoss (nodes.Get (i)->GetObject<MobilityModel>(), 
                            nodes.Get (j)->GetObject<MobilityModel>(), 50);
  }

  // 4. Create & setup wifi channel
  Ptr<YansWifiChannel> wifiChannel = CreateObject <YansWifiChannel> ();
  wifiChannel->SetPropagationLossModel (lossModel);
  wifiChannel->SetPropagationDelayModel (CreateObject <ConstantSpeedPropagationDelayModel> ());

  // 5. Install wireless devices
  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", 
                                "DataMode",StringValue (phyMode), 
                                "ControlMode",StringValue ("DsssRate1Mbps"));
  YansWifiPhyHelper wifiPhy;
  wifiPhy.SetErrorRateModel ("ns3::NistErrorRateModel");
  wifiPhy.SetChannel (wifiChannel);
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);  
  WifiMacHelper wifiMac; // = WifiMacHelper::Default ();
  //wifiMac.SetType ("ns3::AdhocWifiMac"); // use ad-hoc MAC
  // Setup the rest of the upper mac
  Ssid ssid = Ssid ("wifi-default");
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid),
                   "ActiveProbing", BooleanValue (false));
       
  NetDeviceContainer devices;
  for ( int i = 1; i < nn; i++){           
         NetDeviceContainer staDevice = wifi.Install (wifiPhy, wifiMac, nodes.Get (i));
  //NetDeviceContainer staDevice2 = wifi.Install (wifiPhy, wifiMac, nodes.Get (2));
         devices.Add(staDevice);
   }
  //devices.Add( staDevice2);
  // setup ap.
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid));
  NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, nodes.Get (0));
  devices.Add (apDevice);
  if (tcp == 0)               
    wifiPhy.EnablePcapAll ("lab3_UDP");
  else
    wifiPhy.EnablePcapAll ("lab3_TCP");

  // 6. Install TCP/IP stack & assign IP addresses
  InternetStackHelper internet;
  internet.Install (nodes);
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("192.168.1.0", "255.255.255.0");
  ipv4.Assign (devices);

  // 7. Install applications: two CBR streams each saturating the channel
  for(int i = 0; i < nn; i++) {
      if (i % ns == i % nd)
        continue;

      if (tcp == 0)
        setup_flow_udp(i % ns, i % nd, nodes, packetSize, UDPrate);
      else
        setup_flow_tcp(i % ns, i % nd, nodes, packetSize, UDPrate);

  }
 
  // 8. Install FlowMonitor on all nodes
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  Config::SetDefault ("ns3::ConfigStore::Filename", StringValue ("output-attributes.txt"));
  Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("RawText"));
  Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Save"));
  ConfigStore outputConfig2;
  outputConfig2.ConfigureDefaults ();
  outputConfig2.ConfigureAttributes ();

  // 9. Run simulation for 2 seconds
  Simulator::Stop (Seconds (2));
  Simulator::Run ();

  // 10. Print per flow statistics
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
          std::cout << "  Throughput: " << i->second.rxBytes * 8 / ftime / 1000000.0  << " Mbps\n"; // 10 secunde (stop - start)
        }
    }

  // 11. Cleanup
  Simulator::Destroy ();
}

int main (int argc, char **argv)
{
  
  uint32_t packetSize = 1460; // bytes
  uint16_t mtuSize = 1472;
  string phyMode ("DsssRate11Mbps");
  int ns = 1, nd = 1;
  int UDPrate = 7000000;
  uint8_t isTcp = 0;
  bool useRtsCts = false;
  
  CommandLine cmd;
 
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("ns", "number of nodes source for transmission", ns);
  cmd.AddValue ("nd", "number of nodes destination for transmission", nd);
  cmd.AddValue ("UDPrate", "UDP/TCP offered rate[bps] ", UDPrate);
  cmd.AddValue ("isTcp", "If 1 is TCP traffic if 0 is UDP", isTcp);
  cmd.AddValue ("useRtsCts", "toggle usage of RTS/CTS", useRtsCts);

  //Some global configuration
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", 
  TypeIdValue (TypeId::LookupByName ("ns3::TcpNewReno")));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (mtuSize));

  cmd.Parse (argc, argv);
  //std::cout<<"packet size"<<packetSize; 
   
  cout << "Hidden station experiment:\n" << std::flush;
  experiment (useRtsCts, packetSize, phyMode, UDPrate, ns, nd, isTcp);

  return 0;
}
