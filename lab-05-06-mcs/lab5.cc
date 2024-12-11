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
#include <cstdint>
#include <iostream>
#include <iomanip>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("WifiPropagationLab");

Ptr<PacketSink> sink; /* Pointer to the packet sink application */
uint64_t lastTotalRx = 0; /* The value of the last total received bytes */

enum PropagationModel {
  NAKAGAMI_3LOG = 0,    //Nakagami + 3Log Distance
  LOG_DIST_LOSS = 1,    //LogDistancePropagationLossModel
  THREE_LOG_DIST = 2,   //ThreeLogDistancePropagationLossModel
  FRIIS = 3             //FriisPropagationLossModel
};

uint32_t MacTxCount[2], MacRxCount[2];

void
MacTx0(Ptr<CounterCalculator<uint32_t> > datac, std::string path, Ptr<const Packet> packet)
{
    NS_LOG_INFO("MAC Tx 0");
    MacTxCount[0]++;
    datac->Update ();
}

void
MacTx1(Ptr<CounterCalculator<uint32_t> > datac, std::string path, Ptr<const Packet> packet)
{
    NS_LOG_INFO("MAC Tx 1");
    MacTxCount[1]++;
}

void
MacRx0(Ptr<CounterCalculator<uint32_t> > datac, std::string path, Ptr<const Packet> packet)
{
    NS_LOG_INFO("MAC Rx 0");
    MacRxCount[0]++;
    datac->Update ();
}

void
MacRx1(Ptr<CounterCalculator<uint32_t> > datac, std::string path, Ptr<const Packet> packet)
{
    NS_LOG_INFO("MAC Rx 1");
    MacRxCount[1]++;
    datac->Update ();
}

uint32_t PhyTxCount[2], PhyRxCount[2];

void
PhyTx0(std::string path, Ptr<const Packet> packet, double powerW)
{
    (void)powerW;
    NS_LOG_INFO("Phy Tx 0");
    PhyTxCount[0]++;
}

void
PhyTx1(std::string path, Ptr<const Packet> packet, double powerW)
{
    NS_LOG_INFO("Phy Tx 1");
    PhyTxCount[1]++;
    (void)powerW;
}

void
PhyRx0(std::string path, Ptr<const Packet> packet)
{
    NS_LOG_INFO("Phy Rx 0");
    PhyRxCount[0]++;
}

void
PhyRx1(std::string path, Ptr<const Packet> packet)
{
    NS_LOG_INFO("Phy Rx 1");
    PhyRxCount[1]++;
}


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

void AdvancePosition (Ptr<Node> node, int stepsSize, double stepsTime)
{
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
    Vector pos = mobility->GetPosition ();
    NS_LOG_INFO("AdvancePosition crt= " << pos.x << "next: " << pos.x + stepsSize);
    pos.x += stepsSize;
    mobility->SetPosition (pos);
    Simulator::Schedule (Seconds (stepsTime), &AdvancePosition, node, stepsSize, stepsTime);
}

Gnuplot2dDataset m_output;

void CalculateThroughput (Ptr<Node> node, double stepsTime)
{
  Time now = Simulator::Now (); /* Return the simulator's virtual time. */
  double curBytes = (sink->GetTotalRx () - lastTotalRx);
  double curMbps = curBytes * (double) 8 / stepsTime / 1e6;
  /* Convert Application RX Packets to MBits. */

  Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
  Vector pos = mobility->GetPosition ();
  std::cout << std::fixed << std::setprecision(2) << now.GetSeconds ();
  std::cout << " " << pos.x << " " << curMbps << " "; 
  std::cout << MacTxCount[0] << " " << MacRxCount[1] << " ";
  std::cout << PhyTxCount[0] << " " << PhyRxCount[1] << std::endl;

  //  if (now.GetSeconds() > 5 && floor(now.GetSeconds()) == ceil(now.GetSeconds())) {
  if(1){
      m_output.Add (pos.x, curBytes);
      memset(MacTxCount, 0, sizeof(MacTxCount));
      memset(MacRxCount, 0, sizeof(MacRxCount));
      memset(PhyTxCount, 0, sizeof(PhyTxCount));
      memset(PhyRxCount, 0, sizeof(PhyRxCount));
  }
  lastTotalRx = sink->GetTotalRx ();


  Simulator::Schedule (Seconds(stepsTime), &CalculateThroughput, node, stepsTime);
}

int main (int argc, char *argv[])
{
    bool isTcp = false;

    double simTime;
    int steps = 30;
    int stepsSize = 5;
    double stepsTime = 1.0;
    bool useRtsCts = false;
    bool pcapTracing = false;
    int tries = 7;
    double defaultDistNodes = 20.0; //meters

    //these two have to be in sync
    int PacketRateSending = 54000000;
    std::string phyRate ("ErpOfdmRate54Mbps");

    uint16_t propagationModel = 0;
    PropagationModel propModel;

    std::string apManager("ns3::AarfWifiManager");
    //other values: "ns3::MinstrelWifiManager", ""ns3::RraaWifiManager""

    CommandLine cmd;
    cmd.AddValue("phyRate", "Wifi Phy Rate (MCS) for DATA Packets in case of constant wifi", phyRate);
    cmd.AddValue("apManager", "Adaptive algorithm to be used", apManager);
    cmd.AddValue("stepsSize", "How many meters each iteration STA goes away", stepsSize);
    cmd.AddValue("steps", "How many different distances to try", steps);
    cmd.AddValue("stepsTime", "Time on each step", stepsTime);
    cmd.AddValue("isTcp", "If true is TCP traffic if false is UDP", isTcp);
    cmd.AddValue("useRtsCts", "toggle usage of RTS/CTS", useRtsCts);
    cmd.AddValue("propagationModel", "Pick propagation model, refer to enum class PropagationModel for values", propagationModel);
    cmd.AddValue("pcap", "Enable/disable PCAP Tracing", pcapTracing);
    cmd.AddValue("tries", "Max number of attempts to send frame (Short and Long Retry limit for station)", tries);

    cmd.Parse (argc, argv);

    std::cout << "steps=" << steps << " stepSize=" << stepsSize << "m stepTime=" << stepsTime << "s\n";   
    
    ConfigStore config;
    config.ConfigureDefaults ();

    /* Enable/disable RTS/CTS */
    if (useRtsCts)
    {
      Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
    }

    /* STA long retry count - relevant for DATA packets - how many retransmits for DATA */
    Config::SetDefault ("ns3::WifiRemoteStationManager::MaxSlrc", UintegerValue (tries));
    /* STA short retry count - relevant for RTS/CTS - max number of attempts for RTS packets */
    Config::SetDefault ("ns3::WifiRemoteStationManager::MaxSsrc", UintegerValue (tries));

    simTime = (steps + 1) * stepsTime;

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
    wifiPhy.SetErrorRateModel ("ns3::NistErrorRateModel");
    wifiPhy.Set("TxPowerEnd", DoubleValue(10.0)); 
    wifiPhy.Set("TxPowerStart", DoubleValue(10.0));
    // wifiPhy.Set("EnergyDetectionThreshold", DoubleValue(-94.0));
    // wifiPhy.Set("CcaMode1Threshold", DoubleValue(-94.0));
    wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

    propModel = static_cast<PropagationModel>(propagationModel);
    switch (propModel) {
      case PropagationModel::NAKAGAMI_3LOG:
      {
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
        break;
      }
      case PropagationModel::LOG_DIST_LOSS:
      {
        YansWifiChannelHelper wifiChannel;
        wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
        wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel", "Exponent", DoubleValue(2.2)); 
        wifiPhy.SetChannel(wifiChannel.Create());
        break;
      }
      case PropagationModel::THREE_LOG_DIST:
      {
        YansWifiChannelHelper wifiChannel;
        wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
        wifiChannel.AddPropagationLoss ("ns3::ThreeLogDistancePropagationLossModel"); 
        wifiPhy.SetChannel(wifiChannel.Create());
        break;
      }
      case PropagationModel::FRIIS:
      {
        YansWifiChannelHelper wifiChannel;
        wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
        wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel"); 
        wifiPhy.SetChannel(wifiChannel.Create());
        break;
      }
      default:
        std::cerr << "Unsupported propagation model given: " << propagationModel;
        return -EAGAIN;
    }

    /* MAC LAYER setup */
    WifiMacHelper wifiMac;
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211g);
    Ssid ssid = Ssid ("network");
    wifiMac.SetType ("ns3::ApWifiMac",
                    "Ssid", SsidValue (ssid),
                    "BeaconInterval", TimeValue (MilliSeconds (1024)));

    if((apManager.compare("ns3::ConstantRateWifiManager")) == 0) {
        wifi.SetRemoteStationManager (apManager, 
            "ControlMode", StringValue ("ErpOfdmRate6Mbps"),    //Control packets mcs
            "DataMode",    StringValue (phyRate),   //DATA packets mcs
            "NonUnicastMode", StringValue (phyRate));   //RTS/CTS packets mcs
    } else {
        std::cout << "Adaptive algorithm is used, phyRate, if you've entered will be ignored";
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
    positionAlloc->Add (Vector (0.0, defaultDistNodes, 0.0));   //node 1 pos
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

    std::cout << "Timestamp(seconds) Distance(m) Throughput(Mbps) MAC TX Packets MAC RX Packets PHY TX Packets PHY RX Packets" << std::endl; 

    /* Install flow monitor in order to gather stats */ 
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

    /* Move the STA by stepsSize meters every stepsTime seconds */
    Simulator::Schedule (Seconds (stepsTime), 
            &AdvancePosition, 
            nodes.Get (1), stepsSize, stepsTime);
    Simulator::Schedule (Seconds (stepsTime), &CalculateThroughput, nodes.Get (1), stepsTime);

    if (pcapTracing)
    {
      /* Do enable PCAP tracing after device install to be sure you get some PCAP */
      wifiPhy.EnablePcapAll("lab5-propagation-adaptive");
    }

    Ptr<CounterCalculator<uint32_t> > totalTx = CreateObject<CounterCalculator<uint32_t> >();
    Ptr<CounterCalculator<uint32_t> > totalRx = CreateObject<CounterCalculator<uint32_t> >();

////////////////////////////////////////////////////////////////////////////////////////
    totalTx->SetKey ("wifi-tx-frames");
    totalTx->SetContext ("node[0]");
    Config::Connect("/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx", MakeBoundCallback(&MacTx0, totalTx));

    totalRx->SetKey ("wifi-rx-frames");
    totalRx->SetContext ("node[0]");
    Config::Connect("/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx", MakeBoundCallback(&MacRx0, totalRx));
////////////////////////////////////////////////////////////////////////////////////////
    totalTx->SetKey ("wifi-tx-frames");
    totalTx->SetContext ("node[1]");
    Config::Connect("/NodeList/1/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx", MakeBoundCallback(&MacTx1, totalTx));

    totalRx->SetKey ("wifi-rx-frames");
    totalRx->SetContext ("node[1]");
    Config::Connect("/NodeList/1/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx", MakeBoundCallback(&MacRx1, totalRx));
////////////////////////////////////////////////////////////////////////////////////////
    Config::Connect("/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxBegin", MakeCallback(&PhyTx0));
    Config::Connect("/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxEnd", MakeCallback(&PhyRx0));
    Config::Connect("/NodeList/1/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxBegin", MakeCallback(&PhyTx1));
    Config::Connect("/NodeList/1/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxEnd", MakeCallback(&PhyRx1));

    config.ConfigureAttributes ();

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

    monitor->CheckForLostPackets ();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      if (i->first >= 1)
        {
          Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
          std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
          std::cout << "  Lost Packets: " << i->second.lostPackets << "\n";
          std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
          std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
        }
    }

    Simulator::Destroy ();

    return 0;
}
