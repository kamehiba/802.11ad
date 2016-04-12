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

int main (int argc, char *argv[])
{
	LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

	double simulationTime				= 30;

	bool tracing						= false;
	bool flowmonitor					= true;

	double serverStartTime				= 0.05;

	std::string dataRate 				= "512kb/s"; // 1Gb/s = 1000000kb/s

	uint32_t wifiChannelNumber			= 1;

	double BoxXmin						= 0;
	double BoxXmax						= 50;
	double BoxYmin						= 0;
	double BoxYmax						= 50;

	//Femtocells Vars
	bool useFemtocells					= true;
	uint32_t nFemtocells				= 1;
	uint32_t nFloors					= 1;
	uint32_t nApartmentsX				= 1;

	double ueSpeed						= 1.0; // m/s.

	uint16_t numEnb 					= 1;
	uint16_t numUe 						= 100;

	std::string outFile 				= "debug";


	Ipv4AddressHelper 			ipv4;
	InternetStackHelper 		internet;
	Ipv4StaticRoutingHelper 	ipv4RoutingHelper;
    ApplicationContainer 		serverApps, clientApps;
	Box 						boxArea;

///////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Setting Up Configs");
///////////////////////////////////////////////

	// disable fragmentation for frames below 2200 bytes
	Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
	// turn off RTS/CTS for frames below 2200 bytes
	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));

///////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Setting Up Command Line Parameters");
///////////////////////////////////////////////

	CommandLine cmd;
	cmd.AddValue("simulationTime", "Simulation Time: ", simulationTime);
	cmd.Parse (argc, argv);
	ConfigStore inputConfig;
	inputConfig.ConfigureDefaults ();
	cmd.Parse (argc, argv);

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

	NodeContainer enbNodes;
	enbNodes.Create(numEnb);

	NodeContainer ueNodes;
	ueNodes.Create(numUe);

	internet.Install(enbNodes);
	internet.Install(ueNodes);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing Wifi");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	YansWifiChannelHelper 	channel = YansWifiChannelHelper::Default ();
	YansWifiPhyHelper 	 	wifiPhy	= YansWifiPhyHelper::Default ();
	QosWifiMacHelper		wifiMac = QosWifiMacHelper::Default ();
	WifiHelper 				wifi;

	NetDeviceContainer 		enbApdevice;
	NetDeviceContainer 		ueWifiDevice;

//	for (uint32_t i = 0; i < numEnb; ++i)
//	{
//		std::ostringstream oss;
//		oss << "wifi-default-" << i;
//		Ssid ssid = Ssid (oss.str ());
		Ssid ssid = Ssid ("ns3-wifi");

		wifi.SetStandard (WIFI_PHY_STANDARD_80211ad_OFDM);
		wifi.SetRemoteStationManager ("ns3::IdealWifiManager");
		//wifi.SetRemoteStationManager ("ns3::ArfWifiManager");
		//wifiMac.SetType ("ns3::AdhocWifiMac");

		////video trasmission
		wifiMac.SetBlockAckThresholdForAc(AC_VI, 2);
		wifiMac.SetMsduAggregatorForAc (AC_VI, "ns3::MsduStandardAggregator", "MaxAmsduSize", UintegerValue (262143));

		wifiPhy.SetChannel (channel.Create ());
		wifiPhy.SetErrorRateModel ("ns3::SensitivityModel60GHz");
		wifiPhy.Set ("ChannelNumber", UintegerValue(wifiChannelNumber));

		//Ptr<Measured2DAntenna> m2DAntenna = CreateObject<Measured2DAntenna>();
		//m2DAntenna->SetMode(10);

		Ptr<ConeAntenna> coneAntenna = CreateObject<ConeAntenna>();
		coneAntenna->SetGainDbi(0);
		coneAntenna->GainDbiToBeamwidth(0);

		wifiMac.SetType ("ns3::StaWifiMac","Ssid", SsidValue (ssid),"ActiveProbing", BooleanValue (false));
		ueWifiDevice = wifi.Install (wifiPhy, wifiMac, ueNodes);

		//mac.SetType ("ns3::ApWifiMac","Ssid",SsidValue (ssid),"BeaconGeneration", BooleanValue (true),"BeaconInterval", TimeValue (Seconds (2.5)));
		wifiMac.SetType ("ns3::ApWifiMac","Ssid",SsidValue (ssid));
		enbApdevice = wifi.Install (wifiPhy, wifiMac, enbNodes);
	//}

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing WIFI Mobility");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	NS_LOG_UNCOND ("Randomly allocating enbNodes inside the boxArea");
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
	enbMobility.Install (enbNodes);
	BuildingsHelper::Install (enbNodes);

	NS_LOG_UNCOND ("Randomly allocating ueNodes inside the boxArea");
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
	uemobility.Install (ueNodes);
	for (NodeContainer::Iterator it = ueNodes.Begin (); it != ueNodes.End (); ++it)
	  (*it)->Initialize ();
	BuildingsHelper::Install (ueNodes);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Assigning IP to WIFI Devices Enb/Ue");
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	ipv4.SetBase ("10.0.0.0", "255.255.255.0");
	Ipv4InterfaceContainer wifiApInterface 		= ipv4.Assign (enbApdevice);

	ipv4.SetBase ("192.168.1.0", "255.255.255.0");
	Ipv4InterfaceContainer wifiUeIPInterface 	= ipv4.Assign (ueWifiDevice);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Assigning Routing Tables");
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	for (uint32_t u = 0; u < enbNodes.GetN (); ++u)
	{
		Ptr<Node> enb = enbNodes.Get (u);
		Ptr<Ipv4StaticRouting> enbStaticRouting = ipv4RoutingHelper.GetStaticRouting (enb->GetObject<Ipv4> ());
		enbStaticRouting->AddNetworkRouteTo("192.168.1.0","255.255.255.0",1);
	}

	for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
	{
		Ptr<Node> ue = ueNodes.Get (u);
		Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());
		ueStaticRouting->AddNetworkRouteTo("10.0.0.0","255.255.255.0",1);
	}

	//Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing Applications");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	for (uint32_t i = 0; i < enbNodes.GetN (); ++i)
	{
		for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
		{
			InetSocketAddress dst = InetSocketAddress (wifiUeIPInterface.GetAddress (u), 9);
			OnOffHelper onoff = OnOffHelper ("ns3::UdpSocketFactory", dst);
			onoff.SetConstantRate (DataRate (dataRate));

			ApplicationContainer apps = onoff.Install (enbNodes.Get(i));
			apps.Start (Seconds (serverStartTime));

			PacketSinkHelper sink = PacketSinkHelper ("ns3::UdpSocketFactory", dst);
			apps = sink.Install (ueNodes.Get(u));
			apps.Start (Seconds (serverStartTime));
		}
	}

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Starting Simulation...");
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	std::cout << std::endl;
	std::cout << "========================= " << "\n";
	std::cout << "eNB: " << numEnb << "\n";
	std::cout << "UE: " << numUe << "\n";
	std::cout << "UE Speed: " << ueSpeed << "m/s" << " <> " << ueSpeed*3.6 << "km/h\n";
	useFemtocells == true ? std::cout << "Femtocells: " << nFemtocells << "\n" : std::cout << "Femtocells Disabled\n";
	std::cout << "DataRate: " << dataRate << "\n";
	std::cout << "Area: " << (boxArea.xMax - boxArea.xMin) * (boxArea.yMax - boxArea.yMin) << "m²\n";
	std::cout << "========================= " << "\n\n";

	Simulator::Stop (Seconds (simulationTime));

	if (tracing)
	{
	  //p2p.EnablePcapAll (outFile);
	  ////wifiPhy.EnablePcap (outFile, enbApdevice.Get (0));
		wifiPhy.EnablePcapAll (outFile, true);
	}

	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll();

	Simulator::Run ();
	if(flowmonitor)
	{
//		AnimationInterface anim (outFile+"_anim.xml");
//	 	anim.SetConstantPosition (remoteHostWIFI, 0.0, -20.0);
//	 	anim.SetConstantPosition (wifiApNode, 0.0, 0.0);
//	 	anim.SetConstantPosition (ueNode, distanceXUe, distanceYUe);

		monitor->CheckForLostPackets ();
		monitor->SerializeToXmlFile (outFile+"_monitor.xml", true, true);
	}
	Simulator::Destroy ();

	return 0;
}

