/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Daniel D. Costa <danielcosta@inf.ufg.br>

 Default Network Topology

   Enb
   *
   -
   -
   - ue

*/
//#include "ns3/lte-module.h"
//#include "ns3/lte-helper.h"
#include "ns3/csma-module.h"
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

NS_LOG_COMPONENT_DEFINE ("debug");

void showConfigs(uint32_t,uint32_t,double,bool,uint32_t,std::string,uint32_t,Box,double);
void flowmonitorOutput(Ptr<FlowMonitor>, FlowMonitorHelper&, std::string, double,uint32_t);

int main (int argc, char *argv[])
{
	double simulationTime				= 2;

	bool verbose						= true; // packetSink dataFlow
	bool tracing						= false;
	bool netAnim						= false;
	bool flowmonitor					= true;

	double serverStartTime				= 0.05;

	//P2P Vars
	uint32_t mtu						= 1500; // p2p Mtu
	uint32_t p2pLinkDelay				= 0.010;// p2p link Delay

	//Area Vars
	double BoxXmin						= 0;
	double BoxXmax						= 5;
	double BoxYmin						= 0;
	double BoxYmax						= 5;

	//Femtocells Vars
	bool useFemtocells					= true;
	uint32_t nFemtocells				= 1;
	uint32_t nFloors					= 1;
	uint32_t nApartmentsX				= 1;

	//APP Vars
    std::string protocol 				= "ns3::UdpSocketFactory";
	std::string dataRate 				= "512Kbps";
	uint32_t packetSize					= 1024;
	uint32_t appPort					= 9;

	//IPs
	Ipv4Address ipRemoteHost			= "1.0.0.0";
	Ipv4Address ipRouter				= "2.0.0.0";
	Ipv4Address ipWifiAp				= "3.0.0.0";
	Ipv4Address ipWifiStat				= "4.0.0.0";
	Ipv4Mask	netMask					= "255.0.0.0";

	//WIFI Vars
	bool use2DAntenna					= true;
	uint32_t wifiChannelNumber			= 1;
	uint32_t nTxAntennas 				= 1;
	uint32_t nRxAntennas				= 1;
	uint32_t maxAmsduSize				= 999999;//262143;

	//Node Vars
	double ueSpeed						= 1.0; 	// m/s.
	uint16_t nAcpoints 					= 1; 	// Access Points
	uint16_t nStations 					= 2;	// Stations

	std::string outFileName				= "debug";

	Ipv4AddressHelper 			ipv4Rh, ipv4Router, ipv4WifiStat, ipv4WifiAp;
	NetDeviceContainer 			remoteHostDevice, routerDevice, wifiApDevice, wifiStatDevice;
	Ipv4InterfaceContainer 		remoteHostInterface, routerInterface, wifiApInterface, wifiStatInterface;
	InternetStackHelper 		internet;
	Ipv4StaticRoutingHelper 	ipv4RoutingHelper;
	Box 						boxArea;

///////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Setting Up Configs");
///////////////////////////////////////////////

	/* No fragmentation and no RTS/CTS */
	Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("99999"));
	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("99999"));

	Config::SetDefault ("ns3::OnOffApplication::DataRate", DataRateValue(DataRate(dataRate)));
    Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (packetSize));
    Config::SetDefault ("ns3::OnOffApplication::OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    Config::SetDefault ("ns3::OnOffApplication::OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    //StaWifiMac::StartActiveAssociation

///////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Setting Up Command Line Parameters");
///////////////////////////////////////////////

	 if (verbose)
	 {
		 LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
	 }

	CommandLine cmd;
	cmd.AddValue("simulationTime", "Simulation Time: ", simulationTime);
	cmd.AddValue("nEnb", "How many ENBs: ", nAcpoints);
	cmd.AddValue("nUe", "How many UEs: ", nStations);
	cmd.AddValue("dataRate", "Data Rate: ", dataRate);
	cmd.AddValue("packetSize", "Packet Size: ", packetSize);
	cmd.AddValue("verbose", "Verbose: ", verbose);
	cmd.Parse (argc, argv);
	ConfigStore inputConfig;
	inputConfig.ConfigureDefaults ();
	cmd.Parse (argc, argv);

///////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Checking Variables");
///////////////////////////////////////////////

	NS_ASSERT (nAcpoints > 0 && nStations > 0);
	NS_ASSERT (nFemtocells > 0);
	NS_ASSERT (serverStartTime < simulationTime);

///////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Creating Area");
///////////////////////////////////////////////

	boxArea = Box (BoxXmin, BoxXmax, BoxYmin, BoxYmax, 0, 0);

///////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Creating FemtocellBlock");
///////////////////////////////////////////////

	if(useFemtocells)
	{
		FemtocellBlockAllocator blockAllocator (boxArea, nApartmentsX, nFloors);
		blockAllocator.Create (nFemtocells);
	}

///////////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Creating Nodes");
///////////////////////////////////////////////////////////

	NodeContainer remoteHostContainer;
	remoteHostContainer.Create (1);
	Ptr<Node> remoteHost = remoteHostContainer.Get (0);

	NodeContainer routerContainer;
	routerContainer.Create (1);
	Ptr<Node> router = routerContainer.Get (0);

	NodeContainer wifiApNode;
	wifiApNode.Create(nAcpoints);

	NodeContainer wifiStatNode;
	wifiStatNode.Create(nStations);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Assigning IP to WIFI Devices Enb/Ue");
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	internet.Install(remoteHostContainer);
	internet.Install(routerContainer);
	internet.Install(wifiApNode);
	internet.Install(wifiStatNode);

	ipv4Rh.SetBase (ipRemoteHost, netMask);
	ipv4Router.SetBase (ipRouter, netMask);
	ipv4WifiAp.SetBase (ipWifiAp, netMask);
	ipv4WifiStat.SetBase (ipWifiStat, netMask);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("Installing p2p");
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	PointToPointHelper 	p2ph;
	p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
	p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (p2pLinkDelay)));
	p2ph.SetDeviceAttribute ("Mtu", UintegerValue (mtu));

	remoteHostDevice = p2ph.Install (remoteHost, router);
	remoteHostInterface	= ipv4Rh.Assign (remoteHostDevice);

	for (uint32_t i = 0; i < wifiApNode.GetN (); ++i)
	{
		routerDevice = p2ph.Install (wifiApNode.Get(i), router);
		routerInterface	= ipv4Router.Assign (routerDevice);
	}

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing Wifi");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	YansWifiChannelHelper 	wifiChannel = YansWifiChannelHelper::Default ();
	YansWifiPhyHelper 	 	wifiPhy		= YansWifiPhyHelper::Default ();
	QosWifiMacHelper		wifiMac 	= QosWifiMacHelper::Default ();
	WifiHelper 				wifi;

	for (uint32_t i = 0; i < nAcpoints; ++i)
	{
		//////////////////////////////////////////////////
		NS_LOG_UNCOND ("Innitializing WIFI on ENB-" << i);
		///////////////////////////////////////////////////

		Ssid ssid = Ssid ("ns3-wifi");

		wifi.SetStandard (WIFI_PHY_STANDARD_80211ad_OFDM);
		wifi.SetRemoteStationManager ("ns3::IdealWifiManager", "BerThreshold",DoubleValue(1e-6));
		//wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",StringValue ("OfdmRate7Gbps"), "ControlMode",StringValue ("OfdmRate2Gbps"));

		wifiMac.SetType ("ns3::FlywaysWifiMac");

		////video trasmission
		wifiMac.SetBlockAckThresholdForAc(AC_VI, 2);
		wifiMac.SetMsduAggregatorForAc (AC_VI, "ns3::MsduStandardAggregator", "MaxAmsduSize", UintegerValue (maxAmsduSize));

		//wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
		//wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Lambda", DoubleValue(3e8/60e9));
		//wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5e9));

		wifiPhy.SetChannel (wifiChannel.Create ());
		wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);
		wifiPhy.SetErrorRateModel ("ns3::SensitivityModel60GHz");
		wifiPhy.Set ("ChannelNumber", UintegerValue(wifiChannelNumber));
		wifiPhy.Set ("TxAntennas", UintegerValue (nTxAntennas));
		wifiPhy.Set ("RxAntennas", UintegerValue (nRxAntennas));
//		wifiPhy.Set ("TxGain", DoubleValue (0));
//		wifiPhy.Set ("RxGain", DoubleValue (0));
		wifiPhy.Set ("TxPowerStart", DoubleValue(10.0));
		wifiPhy.Set ("TxPowerEnd", DoubleValue(10.0));
		wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
		wifiPhy.Set ("RxNoiseFigure", DoubleValue(0));
		wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-79) );
		wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-79+3) );
		wifiPhy.Set ("ShortGuardEnabled", BooleanValue (false));// Set guard interval

		wifiMac.SetType ("ns3::ApWifiMac","Ssid",SsidValue (ssid),"BeaconGeneration", BooleanValue (true),"BeaconInterval", TimeValue (Seconds (2.5)));
		//wifiApDevice.Add (wifi.Install (wifiPhy, wifiMac, wifiApNode));
		wifiApDevice = wifi.Install (wifiPhy, wifiMac, wifiApNode);
		wifiApInterface = ipv4WifiAp.Assign (wifiApDevice);

		wifiMac.SetType ("ns3::StaWifiMac","Ssid", SsidValue (ssid),"ActiveProbing", BooleanValue (true));
		//wifiStatDevice.Add (wifi.Install (wifiPhy, wifiMac, wifiStatNode));
		wifiStatDevice = wifi.Install (wifiPhy, wifiMac, wifiStatNode);
		wifiStatInterface = ipv4WifiStat.Assign (wifiStatDevice);

		if(use2DAntenna)
		{
			Ptr<Measured2DAntenna> m2DAntenna = CreateObject<Measured2DAntenna>();
			m2DAntenna->SetMode(10);

			wifiApDevice.Get(i)->GetObject<WifiNetDevice>()->GetPhy()->AggregateObject(m2DAntenna);

			for (uint32_t u = 0; u < nStations; ++u)
				wifiStatDevice.Get(u)->GetObject<WifiNetDevice>()->GetPhy()->AggregateObject(m2DAntenna);
		}
	}

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing WIFI Mobility");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	NS_LOG_UNCOND ("Installing mobility on remoteHost");
	MobilityHelper rhmobility;
	Ptr<ListPositionAllocator> rhPositionAlloc = CreateObject<ListPositionAllocator> ();
	rhPositionAlloc = CreateObject<ListPositionAllocator> ();
	rhPositionAlloc->Add (Vector (0.0, -5.0, 0.0));
	rhmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	rhmobility.SetPositionAllocator(rhPositionAlloc);
	rhmobility.Install(remoteHostContainer);
	BuildingsHelper::Install (remoteHostContainer);

	NS_LOG_UNCOND ("Installing mobility on Router");
	MobilityHelper routermobility;
	Ptr<ListPositionAllocator> routerPositionAlloc = CreateObject<ListPositionAllocator> ();
	routerPositionAlloc = CreateObject<ListPositionAllocator> ();
	routerPositionAlloc->Add (Vector (0.0, 0.0, 0.0));
	routermobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	routermobility.SetPositionAllocator(routerPositionAlloc);
	routermobility.Install(routerContainer);
	BuildingsHelper::Install (routerContainer);

	NS_LOG_UNCOND ("Randomly allocating wifiApNode inside the boxArea");
	MobilityHelper enbMobility;
	Ptr<PositionAllocator> enbPositionAlloc;
	enbPositionAlloc = CreateObject<RandomBoxPositionAllocator> ();
	Ptr<UniformRandomVariable> xVal = CreateObject<UniformRandomVariable> ();
	xVal->SetAttribute ("Min", DoubleValue (boxArea.xMin));
	xVal->SetAttribute ("Max", DoubleValue (boxArea.xMax));
	enbPositionAlloc->SetAttribute ("X", PointerValue (xVal));
	Ptr<UniformRandomVariable> yVal = CreateObject<UniformRandomVariable> ();
	yVal->SetAttribute ("Min", DoubleValue (boxArea.yMin));
	yVal->SetAttribute ("Max", DoubleValue (boxArea.yMax));
	enbPositionAlloc->SetAttribute ("Y", PointerValue (yVal));
	Ptr<UniformRandomVariable> zVal = CreateObject<UniformRandomVariable> ();
	zVal->SetAttribute ("Min", DoubleValue (boxArea.zMin));
	zVal->SetAttribute ("Max", DoubleValue (boxArea.zMax));
	enbPositionAlloc->SetAttribute ("Z", PointerValue (zVal));
	enbMobility.SetPositionAllocator (enbPositionAlloc);
	enbMobility.Install (wifiApNode);
	BuildingsHelper::Install (wifiApNode);

	NS_LOG_UNCOND ("Randomly allocating wifiStatNode inside the boxArea");
	MobilityHelper uemobility;
	uemobility.SetMobilityModel ("ns3::SteadyStateRandomWaypointMobilityModel");
	Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinX", 		DoubleValue (boxArea.xMin));
	Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxX", 		DoubleValue (boxArea.xMax));
	Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinY", 		DoubleValue (boxArea.yMin));
	Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxY", 		DoubleValue (boxArea.yMax));
	Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::Z", 			DoubleValue (0.0));
	Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxSpeed", 	DoubleValue (ueSpeed));
	Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinSpeed", 	DoubleValue (ueSpeed));
	Ptr<PositionAllocator> ueRandomPositionAlloc = CreateObject<RandomRoomPositionAllocator> ();
	ueRandomPositionAlloc = CreateObject<RandomBoxPositionAllocator> ();
	uemobility.SetPositionAllocator (ueRandomPositionAlloc);
	uemobility.Install (wifiStatNode);
	for (NodeContainer::Iterator it = wifiStatNode.Begin (); it != wifiStatNode.End (); ++it)
	  (*it)->Initialize ();
	BuildingsHelper::Install (wifiStatNode);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Assigning Routing Tables");
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	NS_LOG_UNCOND ("Assigning Routing Tables on Remote Host");
	Ptr<Ipv4StaticRouting> rhStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
	rhStaticRouting->AddNetworkRouteTo(ipWifiStat, netMask, 1); //AddNetworkRoute To Stat

	NS_LOG_UNCOND ("Assigning Routing Tables on Router");
	Ptr<Ipv4StaticRouting> routerStaticRouting = ipv4RoutingHelper.GetStaticRouting (router->GetObject<Ipv4> ());
	routerStaticRouting->AddNetworkRouteTo(ipWifiStat, netMask, 2); //AddNetworkRoute To Stat

	NS_LOG_UNCOND ("Assigning Routing Tables on Access Points");
	for (uint32_t u = 0; u < nAcpoints; ++u)
	{
		Ptr<Node> ap = wifiApNode.Get (u);
		Ptr<Ipv4StaticRouting> apStaticRouting = ipv4RoutingHelper.GetStaticRouting (ap->GetObject<Ipv4> ());
		apStaticRouting->AddNetworkRouteTo(ipWifiStat, netMask, 2); //AddNetworkRoute To Stat
	}

//	NS_LOG_UNCOND ("Assigning Routing Tables on Stations");
//	for (uint32_t u = 0; u < nStations; ++u)
//	{
//		Ptr<Node> stat = wifiStatNode.Get (u);
//		Ptr<Ipv4StaticRouting> statStaticRouting = ipv4RoutingHelper.GetStaticRouting (stat->GetObject<Ipv4> ());
//		statStaticRouting->AddNetworkRouteTo(ipRemoteHost, netMask, 1); //AddNetworkRoute To remotehost
//	}

//	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing Applications");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	for (uint32_t u = 0; u < nStations; ++u)
	{
		InetSocketAddress dst = InetSocketAddress (wifiStatInterface.GetAddress (u), appPort);
		OnOffHelper onoffHelper = OnOffHelper (protocol, dst);

		ApplicationContainer apps 	= onoffHelper.Install (remoteHost);
		PacketSinkHelper sink 		= PacketSinkHelper (protocol, dst);
		apps = sink.Install (wifiStatNode.Get(u));
		apps.Start (Seconds (serverStartTime));
	}

/////////////////////////////////////////////////////
	std::cout << std::endl;
	NS_LOG_UNCOND ("//////////////////////////");
	NS_LOG_UNCOND ("==> Starting Simulation <==");
	NS_LOG_UNCOND ("//////////////////////////");
/////////////////////////////////////////////////////

	Simulator::Stop (Seconds (simulationTime));

	if (tracing)
	{
		//wifiPhy.EnablePcap (outFileName, wifiDevice.Get (0));
		//wifiPhy.EnablePcapAll (outFileName, true);
		p2ph.EnablePcapAll (outFileName, true);
	}

	if(netAnim)
	{
		AnimationInterface anim (outFileName+"_anim.xml");
		//anim.SetConstantPosition (wifiApNode, 0.0, 0.0);
		//anim.SetConstantPosition (ueNode, 0.0, 0.0);
	}

	if(flowmonitor)
	{
		FlowMonitorHelper flowmon;
		Ptr<FlowMonitor> monitor = flowmon.InstallAll();

		Simulator::Run ();

		showConfigs(nAcpoints, nStations, ueSpeed, useFemtocells, nFemtocells, dataRate, packetSize, boxArea, simulationTime);
		flowmonitorOutput(monitor, flowmon, outFileName, simulationTime, packetSize);
	}
	else
	{
		Simulator::Run ();
		showConfigs(nAcpoints, nStations, ueSpeed, useFemtocells, nFemtocells, dataRate, packetSize, boxArea, simulationTime);
	}

	std::cout << "========================= " << "\n";
	std::cout << "Simulation time: " << Simulator::Now().GetSeconds () << " seconds\n";

	Simulator::Destroy ();

	time_t tempoInicio 	= time(NULL);
	time_t tempoFinal 	= time(NULL);
	double tempoTotal 	= difftime(tempoFinal, tempoInicio);
	std::cout << "Real time: " << tempoTotal << " seconds   ~ " << (uint32_t)tempoTotal / 60 << " min\n";
	std::cout << "========================= " << "\n\n";

	return 0;
}

void showConfigs(uint32_t nEnb, uint32_t nUe, double ueSpeed, bool useFemtocells, uint32_t nFemtocells,
				std::string dataRate, uint32_t packetSize, Box boxArea, double simulationTime)
{
	std::cout << std::endl;
	std::cout << "==========CONFIGS======== " << "\n";
	std::cout << "eNB: " << nEnb << "\n";
	std::cout << "UE: " << nUe << "\n";
	std::cout << "UE Speed: " << ueSpeed << "m/s" << " <> " << ueSpeed*3.6 << "km/h\n";
	useFemtocells == true ? std::cout << "Femtocells: " << nFemtocells << "\n" : std::cout << "Femtocells Disabled\n";
	std::cout << "DataRate: " << dataRate << "\n";
	std::cout << "Area: " << (boxArea.xMax - boxArea.xMin) * (boxArea.yMax - boxArea.yMin) << "m²\n";
}

void flowmonitorOutput(Ptr<FlowMonitor> monitor, FlowMonitorHelper &flowmon, std::string outFileName, double simulationTime, uint32_t packetSize)
{
	double difftx=0.0, diffrx=0.0, txbitrate_value=0.0, rxbitrate_value=0.0, delay_value=0.0;
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
	FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();

	monitor->CheckForLostPackets ();
	monitor->SerializeToXmlFile (outFileName+"_flowMonitor.xml", true, true);

	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
	{
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);

		difftx = i->second.timeLastTxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds();
		diffrx = i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstRxPacket.GetSeconds();
		txbitrate_value = (double) i->second.txBytes * 8 / 1024 / difftx;

		if (i->second.rxPackets != 0)
		{
			rxbitrate_value = (double)i->second.rxPackets * packetSize *8 /1024 / diffrx;
			delay_value = (double) i->second.delaySum.GetSeconds() /(double) i->second.rxPackets;
		}
		else
		{
			rxbitrate_value = 0;
			delay_value = 0;
		}

		std::cout << "========================= " << "\n";
		std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
		std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
		std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
		std::cout << "  Tx bitrate: " << txbitrate_value << " kbps\n";
		std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / simulationTime / 1024 / 1024  << " Mbps\n\n";

		std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
		std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
		std::cout << "  Rx bitrate: " << rxbitrate_value << " kbps\n\n";

		std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / simulationTime / 1024 / 1024  << " Mbps\n\n";

		std::cout << "  Lost Packets: " << i->second.lostPackets << "\n";
		std::cout << "  Dropped Packets: " << i->second.packetsDropped.size() << "\n";
		std::cout << "  JitterSum: " << i->second.jitterSum << "\n\n";

		std::cout << "  Average delay: " << delay_value << "s\n";
	}
}
