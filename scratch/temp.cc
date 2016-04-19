//#include "ns3/lte-module.h"
//#include "ns3/lte-helper.h"
#include "ns3/core-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include <ns3/buildings-module.h>
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/internet-module.h"
#include "ns3/config-store-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor.h"
#include "ns3/netanim-module.h"
#include "ns3/cone-antenna.h"
#include "ns3/measured-2d-antenna.h"
#include <ns3/femtocellBlockAllocator.h>

using namespace ns3;

int main(int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  /* Configuration. */

  /* Build nodes. */
  NodeContainer term_1;
  term_1.Create (1);

  NodeContainer router_0;
  router_0.Create (1);

  NodeContainer station_0;
  station_0.Create (1);

  NodeContainer ap_0;
  ap_0.Create (1);

  NodeContainer ap_1;
  ap_1.Create (1);

  /* Build link. */
  YansWifiPhyHelper wifiPhy_ap_0 = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel_ap_0 = YansWifiChannelHelper::Default ();
  wifiPhy_ap_0.SetChannel (wifiChannel_ap_0.Create ());

  YansWifiPhyHelper wifiPhy_ap_1 = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel_ap_1 = YansWifiChannelHelper::Default ();
  wifiPhy_ap_1.SetChannel (wifiChannel_ap_1.Create ());

  PointToPointHelper p2p_p2p_1;
  p2p_p2p_1.SetDeviceAttribute ("DataRate", DataRateValue (100000000));
  p2p_p2p_1.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10000)));

  NodeContainer all_p2p_1;
  all_p2p_1.Add (router_0);
  all_p2p_1.Add (term_1);
  NetDeviceContainer ndc_p2p_1 = p2p_p2p_1.Install (all_p2p_1);



  /* Build link net device container. */
  NodeContainer all_ap_0;
  NetDeviceContainer ndc_ap_0;

  Ssid ssid_ap_0 = Ssid ("wifi-default-0");
  WifiHelper wifi_ap_0;
  NqosWifiMacHelper wifiMac_ap_0 = NqosWifiMacHelper::Default ();
  wifi_ap_0.SetRemoteStationManager ("ns3::ArfWifiManager");

  wifiMac_ap_0.SetType ("ns3::ApWifiMac",  "Ssid", SsidValue (ssid_ap_0),  "BeaconGeneration", BooleanValue (true),"BeaconInterval", TimeValue (Seconds (2.5)));
  ndc_ap_0.Add (wifi_ap_0.Install (wifiPhy_ap_0, wifiMac_ap_0, ap_0));

  wifiMac_ap_0.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid_ap_0), "ActiveProbing", BooleanValue (false));
  ndc_ap_0.Add (wifi_ap_0.Install (wifiPhy_ap_0, wifiMac_ap_0, all_ap_0 ));

  MobilityHelper mobility_ap_0;
  mobility_ap_0.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility_ap_0.Install (ap_0);
  mobility_ap_0.Install(all_ap_0);



  NodeContainer all_ap_1;
  NetDeviceContainer ndc_ap_1;

  Ssid ssid_ap_1 = Ssid ("wifi-default-1");
  WifiHelper wifi_ap_1;// = WifiHelper::Default ();
  NqosWifiMacHelper wifiMac_ap_1 = NqosWifiMacHelper::Default ();
  wifi_ap_1.SetRemoteStationManager ("ns3::ArfWifiManager");

  wifiMac_ap_1.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid_ap_1),  "BeaconGeneration", BooleanValue (true),  "BeaconInterval", TimeValue (Seconds (2.5)));
  ndc_ap_1.Add (wifi_ap_1.Install (wifiPhy_ap_1, wifiMac_ap_1, ap_1));

  wifiMac_ap_1.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid_ap_1),  "ActiveProbing", BooleanValue (false));
  ndc_ap_1.Add (wifi_ap_1.Install (wifiPhy_ap_1, wifiMac_ap_1, all_ap_1 ));

  MobilityHelper mobility_ap_1;
  mobility_ap_1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility_ap_1.Install (ap_1);
  mobility_ap_1.Install(all_ap_1);



  /* Install the IP stack. */
  InternetStackHelper internetStackH;
  internetStackH.Install (router_0);
  internetStackH.Install (station_0);
  internetStackH.Install (ap_0);
  internetStackH.Install (term_1);
  internetStackH.Install (ap_1);

  /* IP assign. */
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer iface_ndc_ap_0 = ipv4.Assign (ndc_ap_0);
  ipv4.SetBase ("10.0.1.0", "255.255.255.0");
  Ipv4InterfaceContainer iface_ndc_p2p_1 = ipv4.Assign (ndc_p2p_1);
  ipv4.SetBase ("10.0.2.0", "255.255.255.0");
  Ipv4InterfaceContainer iface_ndc_ap_1 = ipv4.Assign (ndc_ap_1);

  /* Generate Route. */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

//  /* Generate Application. */
  uint16_t port_tcp_0 = 9;
  Address sinkLocalAddress_tcp_0 (InetSocketAddress (Ipv4Address::GetAny (), port_tcp_0));
  PacketSinkHelper sinkHelper_tcp_0 ("ns3::TcpSocketFactory", sinkLocalAddress_tcp_0);
  ApplicationContainer sinkApp_tcp_0 = sinkHelper_tcp_0.Install (station_0);
  sinkApp_tcp_0.Start (Seconds (1.0));
  sinkApp_tcp_0.Stop (Seconds (2.0));
  OnOffHelper clientHelper_tcp_0 ("ns3::TcpSocketFactory", Address ());
  clientHelper_tcp_0.SetAttribute ("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelper_tcp_0.SetAttribute ("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
  ApplicationContainer clientApps_tcp_0;
  AddressValue remoteAddress_tcp_0 (InetSocketAddress (iface_ndc_p2p_1.GetAddress (0), port_tcp_0));
  clientHelper_tcp_0.SetAttribute ("Remote", remoteAddress_tcp_0);
  clientApps_tcp_0.Add (clientHelper_tcp_0.Install (term_1));
  clientApps_tcp_0.Start (Seconds (1.0));
  clientApps_tcp_0.Stop (Seconds (2.0));

  /* Simulation. */
  /* Pcap output. */
  /* Stop the simulation after x seconds. */
  uint32_t stopTime = 3;
  Simulator::Stop (Seconds (stopTime));
  /* Start and clean simulation. */
  Simulator::Run ();
  Simulator::Destroy ();
}
