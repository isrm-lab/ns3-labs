/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/propagation-module.h"
//#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/config-store-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/gnuplot.h"
#include <cmath>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("WifiPropagationLab");

Ptr<PacketSink> sink; /* Pointer to the packet sink application */
uint64_t lastTotalRx = 0; /* The value of the last total received bytes */

static void setup_flow_udp(int src, int dest, NodeContainer nodes, int packetSize, int UDPrate, double simTime) 
{
  ApplicationContainer cbrApps;
  uint16_t cbrPort = 12345;
   
  Ptr <Node> PtrNode = nodes.Get(dest);
  Ptr<Ipv4> ipv4 = PtrNode->GetObject<Ipv4> ();
  Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1, 0); 
  Ipv4Address ipAddrDest = iaddr.GetLocal ();

  OnOffHelper onOffHelper ("ns3::UdpSocketFactory", InetSocketAddress (ipAddrDest, cbrPort));

  std::cout << "[UDP TRAFFIC] src=" << src << " dst=" 
            << dest << " rate=" << DataRate (UDPrate + dest*100) << std::endl;
  onOffHelper.SetConstantRate (DataRate (UDPrate + dest*101), packetSize);
  onOffHelper.SetAttribute ("StartTime", TimeValue (Seconds (1.0 + dest/10.0)));

  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), cbrPort));
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);

  ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get(dest));
  sink = StaticCast<PacketSink> (sinkApp.Get (0));

  cbrApps.Add (onOffHelper.Install (nodes.Get (src)));

  sinkApp.Start (Seconds (0.5));
  sinkApp.Stop (Seconds (simTime));
} 

static void setup_flow_tcp(int src, int dest, NodeContainer nodes, int packetSize, int UDPrate, double simTime)
{
  ApplicationContainer tcpApps;
  uint16_t tcpPort = 12345;

  /* Configure TCP Options */
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (packetSize));

  Ptr <Node> PtrNode = nodes.Get(dest);
  Ptr<Ipv4> ipv4 = PtrNode->GetObject<Ipv4> ();
  Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1, 0); 
  Ipv4Address ipAddrDest = iaddr.GetLocal ();

  OnOffHelper onOffHelper ("ns3::TcpSocketFactory", InetSocketAddress (ipAddrDest, tcpPort));
  onOffHelper.SetAttribute ("PacketSize", UintegerValue (packetSize));

  onOffHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));

  std::cout << "[TCP] src=" << src << " dst=" << dest << std::endl;
  onOffHelper.SetConstantRate (DataRate (UDPrate + dest*101), packetSize);
  onOffHelper.SetAttribute ("StartTime", TimeValue (Seconds (1.0 + dest/10.0)));

  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), tcpPort));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);

  ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get(dest));
  sink = StaticCast<PacketSink> (sinkApp.Get (0));
  tcpApps.Add (onOffHelper.Install (nodes.Get (src)));

  sinkApp.Start (Seconds (0.5));
  sinkApp.Stop (Seconds (simTime));
}

void AdvancePosition (Ptr<Node> node, int stepsSize, int stepsTime)
{
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
    Vector pos = mobility->GetPosition ();
    NS_LOG_INFO("AdvancePosition crt= " << pos.x << "next: " << pos.x + stepsSize);
    pos.x += stepsSize;
    mobility->SetPosition (pos);
    Simulator::Schedule (Seconds (stepsTime), &AdvancePosition, node, stepsSize, stepsTime);
}

Gnuplot2dDataset m_output;

void CalculateThroughput (Ptr<Node> node)
{
  Time now = Simulator::Now (); /* Return the simulator's virtual time. */
  double curBps = (sink->GetTotalRx () - lastTotalRx);
  double curMbps = curBps * (double) 8 / 1e5;
  /* Convert Application RX Packets to MBits. */

  Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
  Vector pos = mobility->GetPosition ();
    std::cout << now.GetSeconds () << " " << pos.x << " " << curMbps << " " << std::endl;
  if (now.GetSeconds() > 5 && floor(now.GetSeconds()) == ceil(now.GetSeconds())) {
    m_output.Add (pos.x, curBps);
  }
  lastTotalRx = sink->GetTotalRx ();
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput, node);
}


int main (int argc, char *argv[])
{
    bool isTcp = false;

    double simTime;
    int steps = 30;
    int stepsSize = 5;
    int stepsTime = 1;
    bool useRtsCts = false;

    //these two have to be in sync
    int PacketRateSending = 54000000;
    std::string phyRate ("ErpOfdmRate54Mbps");


    std::string apManager("ns3::AarfWifiManager");
    //other values: "ns3::MinstrelWifiManager", ""ns3::RraaWifiManager""

    //TODO: command line arguments
    CommandLine cmd;
    cmd.AddValue("phyRate", "Wifi Phy Rate (MCS) for DATA Packets in case of constant wifi", phyRate);
    cmd.AddValue("apManager", "Adaptive algorithm to be used", apManager);
    cmd.AddValue("stepsSize", "How many meters each iteration STA goes away", stepsSize);
    cmd.AddValue("steps", "How many different distances to try", steps);
    cmd.AddValue("stepsTime", "Time on each step", stepsTime);
    cmd.AddValue("isTcp", "If true is TCP traffic if false is UDP", isTcp);
    cmd.AddValue("useRtsCts", "toggle usage of RTS/CTS", useRtsCts);
    cmd.Parse (argc, argv);

    /* Enable/disable RTS/CTS */
    if (useRtsCts)
    {
        Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
    }

    simTime = steps * stepsTime;

    NetDeviceContainer devicesList, devices1;
    YansWifiPhyHelper wifiPhy;
    NodeContainer nodes;
    nodes.Create (2);

    Ptr<RateErrorModel> em1 =
    CreateObjectWithAttributes<RateErrorModel> ("RanVar", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1.0]"),
                                                "ErrorRate", DoubleValue (0.01),
                                                "ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET)
                                                );

    /* PHY LAYER setup */
    wifiPhy = YansWifiPhyHelper::Default ();
    wifiPhy.Set("TxPowerEnd", DoubleValue(10.0)); 
    wifiPhy.Set("TxPowerStart", DoubleValue(10.0));
    wifiPhy.Set("EnergyDetectionThreshold", DoubleValue(-94.0));
    wifiPhy.Set("CcaMode1Threshold", DoubleValue(-94.0));
    wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

    Ptr<YansWifiChannel> wifiChannel = CreateObject <YansWifiChannel> ();

    Ptr<ThreeLogDistancePropagationLossModel> log3 = CreateObject<ThreeLogDistancePropagationLossModel> ();
    Ptr<NakagamiPropagationLossModel> nak = CreateObject<NakagamiPropagationLossModel> ();
    nak->SetAttribute("Distance1", DoubleValue (10000.0)); // Let m0 fading be for all intervals
    nak->SetAttribute("Distance2", DoubleValue (10000.0));
    nak->SetAttribute("m0", DoubleValue (3.0));
    log3->SetNext (nak);
    log3->SetAttribute ("Distance1", DoubleValue (40.0));
    log3->SetAttribute ("Distance2", DoubleValue (100.0));
    wifiChannel->SetPropagationLossModel (log3);
    wifiChannel->SetPropagationDelayModel (CreateObject <ConstantSpeedPropagationDelayModel> ());
    wifiPhy.SetChannel(wifiChannel);

    /* MAC LAYER setup */
    WifiMacHelper wifiMac;
    WifiHelper wifi;
    wifi.SetStandard(WIFI_PHY_STANDARD_80211g);
    Ssid ssid = Ssid ("network");
    wifiMac.SetType ("ns3::ApWifiMac",
                    "Ssid", SsidValue (ssid),
                    "BeaconInterval", TimeValue (MilliSeconds (100)));

    if((apManager.compare("ns3::ConstantRateWifiManager")) == 0) {
        wifi.SetRemoteStationManager (apManager, 
            "ControlMode", StringValue (phyRate),
            "DataMode",    StringValue (phyRate),
            "NonUnicastMode", StringValue (phyRate));
    } else {
        wifi.SetRemoteStationManager(apManager);
    }

    devices1 = wifi.Install (wifiPhy, wifiMac, NodeContainer (nodes.Get (0)));
    devicesList.Add(devices1);
    wifiMac.SetType ("ns3::StaWifiMac",
                    "Ssid", SsidValue (ssid),
                    "ActiveProbing", BooleanValue (true));
    devices1 = wifi.Install (wifiPhy, wifiMac, NodeContainer (nodes.Get (1)));
    devicesList.Add(devices1);

    /* Mobility model - set nodes position*/
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector (0.0, 0.0, 0.0));    //node 0 pos
    positionAlloc->Add (Vector (0.0, -20.0, 0.0));   //node 1 pos
    mobility.SetPositionAllocator (positionAlloc);
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (nodes);

    /* Install TCP/IP stack & assign IP addresses */
    InternetStackHelper internet;
    internet.Install (nodes);
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    ipv4.Assign (devicesList);
    /* Populate routing table */
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    if (isTcp)
        setup_flow_tcp(0, 1, nodes, 1460, PacketRateSending, simTime);
    else
        setup_flow_udp(0, 1, nodes, 1460, PacketRateSending, simTime);

    /* Do enable PCAP tracing after device install to be sure you get some PCAP */
    wifiPhy.EnablePcapAll("lab5-propagation-adaptive");

    std::cout << "Timestamp(seconds) Distance(m) Throughput(Mbps)" << std::endl; 

    //Move the STA by stepsSize meters every stepsTime seconds
    Simulator::Schedule (Seconds (0.5 + stepsTime), 
            &AdvancePosition, 
            nodes.Get (1), stepsSize, stepsTime);

    Simulator::Schedule (Seconds (1.1), &CalculateThroughput, nodes.Get (1));

    /* Run simulation for requested num of seconds */
    Simulator::Stop (Seconds (simTime));
    Simulator::Run ();

    std::string outputFileName("theplot");
    std::ofstream outfile (("throughput-" + outputFileName + ".plt").c_str ());
    Gnuplot gnuplot = Gnuplot (("throughput-" + outputFileName + ".eps").c_str (), "Throughput");
    gnuplot.SetTerminal ("post eps color enhanced");
    gnuplot.SetLegend ("STA Pos (meters)", "Throughput (Mb/s)");
    gnuplot.SetTitle ("Throughput (AP to STA) vs time");
    gnuplot.AddDataset (m_output);
    gnuplot.GenerateOutput (outfile);

    Simulator::Destroy ();

    return 0;
}