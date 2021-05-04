 /* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
 /*
  * Copyright (c) 2019 University of Washington
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
  * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
  */
//  This demo explicitly needs ns 3.30 otherwise doesn't work. It is required for spectrum PHY setup
//  The geometry is as follows:
//
//      0,x             d1,x
//     STA1---200m----> AP1
//                      |
//                       x
//                      |
//                      AP2 <----200m----STA2
//      0,0             0,200           0,400 (d1+d2)
//  STA1 and AP1 are in one BSS (with color set to 1), while STA2 and AP2 are in
//  another BSS (with color set to 2). The distances are configurable.
//
//  STA1 is continously transmitting data to AP1, while STA2 is continuously sending data to AP2.
//  Each STA has configurable traffic loads (inter packet interval and packet size).
//  It is also possible to configure TX power per node as well as their CCA-ED tresholds.
//  A simple Friis path loss model is used and a constant PHY rate is considered.
//
//  In general, the program can be configured at run-time by passing command-line arguments.
//  The following command will display all of the available run-time help options:
//    ./waf --run "wifi-spatial-reuse --help"
//
//  By default, the script runs:
//    ./waf --run wifi-spatial-reuse
//    Throughput for BSS 1: 6.6468 Mbit/s
//    Throughput for BSS 2: 6.6672 Mbit/s
 
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/string.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/application-container.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/wifi-net-device.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/he-configuration.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-server.h"

using namespace ns3;

std::vector<uint32_t> bytesReceived (4);

uint32_t ContextToNodeId (std::string context) {
  std::string sub = context.substr (10);
  uint32_t pos = sub.find ("/Device");
  return atoi (sub.substr (0, pos).c_str ());
}

void SocketRx (std::string context, Ptr<const Packet> p, const Address &addr) {
  uint32_t nodeId = ContextToNodeId (context);
  bytesReceived[nodeId] += p->GetSize ();
}

uint32_t MacTxDropCount, PhyTxDropCount, PhyRxDropCount;

void MacTxDrop(std::string context, Ptr<const Packet> p)
{
  MacTxDropCount++;
}

void PhyTxDrop(std::string context, Ptr<const Packet> p)
{
  PhyTxDropCount++;
}

void PhyRxDrop(std::string context, Ptr<const Packet> p, WifiPhyRxfailureReason reason)
{
  PhyRxDropCount++;
}

int main (int argc, char *argv[])
{
  double duration = 5.0; // seconds
  double d1 = 30.0; // meters
  double d2 = 30.0; // meters
  double x = 10000.0; // meters
  double powSta1 = 10.0; // dBm
  double powSta2 = 10.0; // dBm
  double powAp1 = 21.0; // dBm
  double powAp2 = 21.0; // dBm
  double ccaEdTrSta1 = -62; // dBm
  double ccaEdTrSta2 = -62; // dBm
  double ccaEdTrAp1 = -62; // dBm
  double ccaEdTrAp2 = -62; // dBm
  uint32_t payloadSize = 1472; // bytes
  double interval = 0.0001; // seconds
  double rxSensitivity = -92.2; //dBm
  bool enableRtsCts = false; 

  CommandLine cmd;
  cmd.AddValue ("duration", "Duration of simulation (s)", duration);
  cmd.AddValue ("interval", "Inter packet interval (s)", interval);
  cmd.AddValue ("d1", "Distance between STA1 and AP1 (m)", d1);
  cmd.AddValue ("d2", "Distance between STA2 and AP2 (m)", d2);
  cmd.AddValue ("x", "Distance between AP1 and AP2 (m)", x);
  cmd.AddValue ("powSta1", "Power of STA1 (dBm)", powSta1);
  cmd.AddValue ("powSta2", "Power of STA2 (dBm)", powSta2);
  cmd.AddValue ("powAp1", "Power of AP1 (dBm)", powAp1);
  cmd.AddValue ("powAp2", "Power of AP2 (dBm)", powAp2);
  cmd.AddValue ("ccaEdTrSta1", "CCA-ED Threshold of STA1 (dBm)", ccaEdTrSta1);
  cmd.AddValue ("ccaEdTrSta2", "CCA-ED Threshold of STA2 (dBm)", ccaEdTrSta2);
  cmd.AddValue ("ccaEdTrAp1", "CCA-ED Threshold of AP1 (dBm)", ccaEdTrAp1);
  cmd.AddValue ("ccaEdTrAp2", "CCA-ED Threshold of AP2 (dBm)", ccaEdTrAp2);
  cmd.AddValue ("rxSensitivity", "global rxSensitivity (dBm)", rxSensitivity);
  cmd.AddValue ("enableRtsCts", "RTS/CTS enabled", enableRtsCts);
  cmd.Parse (argc, argv);

  /* Enable/disable RTS/CTS */
  if (enableRtsCts) {
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
  }

  std::cout << "Using distance AP1 -> AP2: " << x << " meters" << std::endl;

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (2);
  NodeContainer wifiApNodes;
  wifiApNodes.Create (2);

  SpectrumWifiPhyHelper spectrumPhy = SpectrumWifiPhyHelper::Default ();
  Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel> ();
  Ptr<LogDistancePropagationLossModel> lossModel = CreateObject<LogDistancePropagationLossModel> ();
  lossModel->SetPathLossExponent(2.6);
  lossModel->SetReference(10, 69.8735);
  spectrumChannel->AddPropagationLossModel (lossModel);
  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
  spectrumChannel->SetPropagationDelayModel (delayModel);

  spectrumPhy.SetChannel (spectrumChannel);
  spectrumPhy.SetErrorRateModel ("ns3::YansErrorRateModel");
  spectrumPhy.Set ("Frequency", UintegerValue (2462)); //channel 11 in 2.4 GHz
  spectrumPhy.SetPreambleDetectionModel ("ns3::ThresholdPreambleDetectionModel");
  //TODO: add parameter to configure CCA-PD

  WifiHelper wifi;
  WifiMacHelper mac;
  wifi.SetStandard (WIFI_STANDARD_80211g);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("ErpOfdmRate12Mbps"),
                                "ControlMode", StringValue ("DsssRate1Mbps"));

  spectrumPhy.Set ("TxPowerStart", DoubleValue (powSta1));
  spectrumPhy.Set ("TxPowerEnd", DoubleValue (powSta1));
  spectrumPhy.Set ("CcaEdThreshold", DoubleValue (ccaEdTrSta1));
  spectrumPhy.Set ("RxSensitivity", DoubleValue (rxSensitivity));

  Ssid ssidA = Ssid ("A");
  mac.SetType ("ns3::StaWifiMac",
              "Ssid", SsidValue (ssidA));
  NetDeviceContainer staDeviceA = wifi.Install (spectrumPhy, mac, wifiStaNodes.Get (0));

  spectrumPhy.Set ("TxPowerStart", DoubleValue (powAp1));
  spectrumPhy.Set ("TxPowerEnd", DoubleValue (powAp1));
  spectrumPhy.Set ("CcaEdThreshold", DoubleValue (ccaEdTrAp1));
  spectrumPhy.Set ("RxSensitivity", DoubleValue (rxSensitivity));

  mac.SetType ("ns3::ApWifiMac",
              "Ssid", SsidValue (ssidA));
  NetDeviceContainer apDeviceA = wifi.Install (spectrumPhy, mac, wifiApNodes.Get (0));

  Ptr<WifiNetDevice> apDevice = apDeviceA.Get (0)->GetObject<WifiNetDevice> ();
  Ptr<ApWifiMac> apWifiMac = apDevice->GetMac ()->GetObject<ApWifiMac> ();

  spectrumPhy.Set ("TxPowerStart", DoubleValue (powSta2));
  spectrumPhy.Set ("TxPowerEnd", DoubleValue (powSta2));
  spectrumPhy.Set ("CcaEdThreshold", DoubleValue (ccaEdTrSta2));
  spectrumPhy.Set ("RxSensitivity", DoubleValue (rxSensitivity));

  Ssid ssidB = Ssid ("B");
  mac.SetType ("ns3::StaWifiMac",
              "Ssid", SsidValue (ssidB));
  NetDeviceContainer staDeviceB = wifi.Install (spectrumPhy, mac, wifiStaNodes.Get (1));

  spectrumPhy.Set ("TxPowerStart", DoubleValue (powAp2));
  spectrumPhy.Set ("TxPowerEnd", DoubleValue (powAp2));
  spectrumPhy.Set ("CcaEdThreshold", DoubleValue (ccaEdTrAp2));
  spectrumPhy.Set ("RxSensitivity", DoubleValue (rxSensitivity));

  mac.SetType ("ns3::ApWifiMac",
              "Ssid", SsidValue (ssidB));
  NetDeviceContainer apDeviceB = wifi.Install (spectrumPhy, mac, wifiApNodes.Get (1));

  Ptr<WifiNetDevice> ap2Device = apDeviceB.Get (0)->GetObject<WifiNetDevice> ();
  apWifiMac = ap2Device->GetMac ()->GetObject<ApWifiMac> ();

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (d1, x, 0.0));           // AP1
  positionAlloc->Add (Vector (d1, 0.0, 0.0));         // AP2
  positionAlloc->Add (Vector (0.0, x, 0.0));          // STA1
  positionAlloc->Add (Vector (0.0, d1 + d2, 0.0));      // STA2
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (wifiApNodes);
  mobility.Install (wifiStaNodes);

  PacketSocketHelper packetSocket;
  packetSocket.Install (wifiApNodes);
  packetSocket.Install (wifiStaNodes);
  ApplicationContainer apps;
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  //BSS 1
  {
    PacketSocketAddress socketAddr;
    socketAddr.SetSingleDevice (staDeviceA.Get (0)->GetIfIndex ());
    socketAddr.SetPhysicalAddress (apDeviceA.Get (0)->GetAddress ());
    socketAddr.SetProtocol (1);
    Ptr<PacketSocketClient> client = CreateObject<PacketSocketClient> ();
    client->SetRemote (socketAddr);
    wifiStaNodes.Get (0)->AddApplication (client);
    client->SetAttribute ("PacketSize", UintegerValue (payloadSize));
    client->SetAttribute ("MaxPackets", UintegerValue (0));
    client->SetAttribute ("Interval", TimeValue (Seconds (interval)));
    Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer> ();
    server->SetLocal (socketAddr);
    wifiApNodes.Get (0)->AddApplication (server);
  }

  // BSS 2
  {
    PacketSocketAddress socketAddr;
    socketAddr.SetSingleDevice (staDeviceB.Get (0)->GetIfIndex ());
    socketAddr.SetPhysicalAddress (apDeviceB.Get (0)->GetAddress ());
    socketAddr.SetProtocol (1);
    Ptr<PacketSocketClient> client = CreateObject<PacketSocketClient> ();
    client->SetRemote (socketAddr);
    wifiStaNodes.Get (1)->AddApplication (client);
    client->SetAttribute ("PacketSize", UintegerValue (payloadSize));
    client->SetAttribute ("MaxPackets", UintegerValue (0));
    client->SetAttribute ("Interval", TimeValue (Seconds (interval)));
    Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer> ();
    server->SetLocal (socketAddr);
    wifiApNodes.Get (1)->AddApplication (server);
  }

  Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::PacketSocketServer/Rx", MakeCallback (&SocketRx));

    /* MacTxDrop: A packet has been dropped in the MAC layer before transmission. */
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTxDrop", MakeCallback(&MacTxDrop));
    /* PhyTxDrop: Trace source indicating a packet has been dropped by the device during transmission */
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop", MakeCallback(&PhyRxDrop));
    /* PhyRxDrop: Trace source indicating a packet has been dropped by the device during reception */
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxDrop", MakeCallback(&PhyTxDrop));

  Simulator::Stop (Seconds (duration));
  Simulator::Run ();
  Simulator::Destroy ();

  for (uint32_t i = 0; i < 2; i++) {
    double throughput = static_cast<double> (bytesReceived[2 + i]) * 8 / 1000 / 1000 / duration;
    std::cout << "Throughput_BSS" << i + 1 << ": " << throughput << " Mbit/s" << std::endl;
  }

  std::cout << "MacTxDropCount: " << MacTxDropCount << std::endl;
  std::cout << "PhyTxDropCount: " << PhyTxDropCount << std::endl; 
  std::cout << "PhyRxDropCount: " << PhyRxDropCount << std::endl;

  return 0;
}
