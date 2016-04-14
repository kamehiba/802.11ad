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
   - ues

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
	//LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

	double simulationTime				= 10;

	bool tracing						= false;
	bool flowmonitor					= true;

	double serverStartTime				= 0.05;

	std::string dataRate 				= "512kb/s"; // 1Gb/s = 1000000kb/s         512kb/s

	uint32_t wifiChannelNumber			= 1;

	double BoxXmin						= 0;
	double BoxXmax						= 5;
	double BoxYmin						= 0;
	double BoxYmax						= 5;

	//Femtocells Vars
	bool useFemtocells					= true;
	uint32_t nFemtocells				= 1;
	uint32_t nFloors					= 1;
	uint32_t nApartmentsX				= 1;

	double ueSpeed						= 1.0; // m/s.

	uint16_t numEnb 					= 1;
	uint16_t numUe 						= 2;

	std::string outFile 				= "debug";

	Ipv4InterfaceContainer 				wifiApInterface, wifiUeIPInterface;
	Ipv4AddressHelper 					ipv4Enb, ipv4UE;
	InternetStackHelper 				internet;
	Ipv4StaticRoutingHelper 			ipv4RoutingHelper;
    ApplicationContainer 				serverApps, clientApps;
	Box 								boxArea;

///////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Setting Up Configs");
///////////////////////////////////////////////

	// disable fragmentation for frames below 2200 bytes
	Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));

	// turn off RTS/CTS for frames below 2200 bytes
	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));

	// Set to true if this interface should respond to interface events by globallly recomputing routes
	//Config::SetDefault ("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue (true));

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

	ipv4Enb.SetBase ("1.0.0.0", "255.0.0.0");
	ipv4UE.SetBase ("2.0.0.0", "255.0.0.0");

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

	for (uint32_t i = 0; i < enbNodes.GetN (); ++i)
	{
		std::ostringstream oss;
		oss << "ns3-wifi-" << i;
		Ssid ssid = Ssid (oss.str ());
		//Ssid ssid = Ssid ("ns3-wifi");

		wifi.SetStandard (WIFI_PHY_STANDARD_80211ad_OFDM);
		wifi.SetRemoteStationManager ("ns3::IdealWifiManager");

		////video trasmission
		wifiMac.SetBlockAckThresholdForAc(AC_VI, 2);
		wifiMac.SetMsduAggregatorForAc (AC_VI, "ns3::MsduStandardAggregator", "MaxAmsduSize", UintegerValue (262143));

		wifiPhy.SetChannel (channel.Create ());
		wifiPhy.SetErrorRateModel ("ns3::SensitivityModel60GHz");
		wifiPhy.Set ("ChannelNumber", UintegerValue(wifiChannelNumber));

		Ptr<Measured2DAntenna> m2DAntenna = CreateObject<Measured2DAntenna>();
		m2DAntenna->SetMode(10);

		//Ptr<ConeAntenna> coneAntenna = CreateObject<ConeAntenna>();
		//coneAntenna->SetGainDbi(0);
		//coneAntenna->GainDbiToBeamwidth(0);

		wifiMac.SetType ("ns3::ApWifiMac","Ssid",SsidValue (ssid),"BeaconGeneration", BooleanValue (true),"BeaconInterval", TimeValue (Seconds (2.5)));
		enbApdevice = wifi.Install (wifiPhy, wifiMac, enbNodes.Get(i));
		wifiApInterface = ipv4Enb.Assign (enbApdevice);

		wifiMac.SetType ("ns3::StaWifiMac","Ssid", SsidValue (ssid),"ActiveProbing", BooleanValue (false));
		ueWifiDevice = wifi.Install (wifiPhy, wifiMac, ueNodes);
		wifiUeIPInterface = ipv4UE.Assign (ueWifiDevice);

		//////////////////////////////////////////////////////
		//////////////////////////////////////////////////////
			NS_LOG_UNCOND ("==> Assigning Routing Tables on ENB " << i);
		/////////////////////////////////////////////////////
		/////////////////////////////////////////////////////

		Ptr<Node> enb = enbNodes.Get (i);
		Ptr<Ipv4StaticRouting> enbStaticRouting = ipv4RoutingHelper.GetStaticRouting (enb->GetObject<Ipv4> ());
		enbStaticRouting->AddHostRouteTo(wifiUeIPInterface.GetAddress(0),1);
		enbStaticRouting->AddNetworkRouteTo("2.0.0.0","255.0.0.0", 1); //AddNetwork Route To UEnode

		for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
		{
			NS_LOG_UNCOND ("==> Assigning Routing Tables on UE " << u);
			Ptr<Node> ue = ueNodes.Get (u);
			Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());
			ueStaticRouting->AddHostRouteTo(wifiApInterface.GetAddress(0),i+1);
			ueStaticRouting->AddNetworkRouteTo("1.0.0.0","255.0.0.0", i+1); //AddNetwork Route To enbNode

			NS_LOG_UNCOND ("Starting Apps on UE Interface " << u);
			InetSocketAddress dst = InetSocketAddress (wifiUeIPInterface.GetAddress (u), 9);
			OnOffHelper onoff = OnOffHelper ("ns3::UdpSocketFactory", dst);
			onoff.SetConstantRate (DataRate (dataRate));

			NS_LOG_UNCOND ("Starting Apps on ENB " << i);
			ApplicationContainer apps = onoff.Install (enbNodes.Get(i));
			apps.Start (Seconds (serverStartTime));

			PacketSinkHelper sink = PacketSinkHelper ("ns3::UdpSocketFactory", dst);
			apps = sink.Install (ueNodes.Get(u));
			apps.Start (Seconds (serverStartTime));
		}
	}

	//Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

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
	  ////wifiPhy.EnablePcap (outFile, enbApdevice.Get (0));
		wifiPhy.EnablePcapAll (outFile, true);
	}

	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll();

	Simulator::Run ();

	if(flowmonitor)
	{
//		AnimationInterface anim (outFile+"_anim.xml");
//	 	anim.SetConstantPosition (wifiApNode, 0.0, 0.0);
//	 	anim.SetConstantPosition (ueNode, distanceXUe, distanceYUe);

		monitor->CheckForLostPackets ();
		monitor->SerializeToXmlFile (outFile+"_monitor.xml", true, true);

		Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
		FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
		for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
		{
			Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
			std::cout << "Flow " << i->first - 2 << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
			std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
			std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
			std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / 9.0 / 1000 / 1000  << " Mbps\n";
			std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
			std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
			std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / 9.0 / 1000 / 1000  << " Mbps\n";
		}
	}
	std::cout << std::endl << std::endl;
	std::cout << "Simulation time: " << Simulator::Now().GetSeconds () << " seconds <> " << Simulator::Now().GetMinutes() << " minutes \n";
	std::cout << "Real time: " << Simulator::Now().GetSeconds () << " seconds \n\n";

	Simulator::Destroy ();

	return 0;
}

