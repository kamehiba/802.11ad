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

  WIFI

  Remote Host
   |
   |
   Enb
   *
   -
   -
   - ue

*/

#include "ns3/lte-module.h"
#include "ns3/lte-helper.h"
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
#include "ns3/csma-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("debug");

int main (int argc, char *argv[])
{
	//LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
	//LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
	LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

	double simulationTime				= 30;

	bool tracing						= false;
	bool flowmonitor					= true;

	bool   useUdp						= true;

	bool   useDl						= true;
	bool   useUl						= false;

	double serverStartTime				= 0.01;
	double clientStartTime				= 0.01;

	uint32_t dlPort 					= 12345;
	uint32_t ulPort 					= 9;

	uint32_t mtu						= 1500; 		// p2p Mtu
	uint32_t linkDelay					= 0.010;		// p2p link Delay ms
	std::string dataRate 				= "100Gb/s";

	uint32_t maxPackets					= 100000000; 	// The maximum number of packets the application will send
	uint32_t maxTCPBytes 				= 0;
	double UDPPacketInterval 			= 1;

	uint32_t channelNumber 				= 1;

	double BoxXmin						= 0;
	double BoxXmax						= 10;
	double BoxYmin						= 0;
	double BoxYmax						= 10;

	//Femtocells Vars
	bool useFemtocells					= true;
	uint32_t nFemtocells				= 1;
	uint32_t nFloors					= 1;
	uint32_t nApartmentsX				= 1;

	double ueSpeed						= 1.0; // m/s.

	uint16_t numEnb 					= 1;
	uint16_t numUe 						= 1;

	std::string outFile 				= "debug";

	CommandLine cmd;
	cmd.AddValue("simulationTime", "Simulation Time: ", simulationTime);
	cmd.Parse (argc, argv);
	ConfigStore inputConfig;
	inputConfig.ConfigureDefaults ();
	cmd.Parse (argc, argv);

	// disable fragmentation for frames below 2200 bytes
	Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
	// turn off RTS/CTS for frames below 2200 bytes
	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));

	Ipv4AddressHelper 			ipv4;
	InternetStackHelper 		internet;
	Ipv4StaticRoutingHelper 	ipv4RoutingHelper;
    ApplicationContainer 		serverApps, clientApps;
	Box 						boxArea;

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

	NodeContainer enbNodes;
	enbNodes.Create(numEnb);

	NodeContainer ueNodes;
	ueNodes.Create(numUe);

	internet.Install(remoteHostContainer);
	internet.Install(enbNodes);
	internet.Install(ueNodes);

///////////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing Remote Host WIFI");
///////////////////////////////////////////////////////////

	NS_LOG_UNCOND ("Installing enbNodes on Remote Host");
	NetDeviceContainer remoteHostDevices;
	Ipv4InterfaceContainer remoteHostWifiIpIfaces;

	for (uint32_t u = 0; u < enbNodes.GetN (); ++u)
	{
		PointToPointHelper 	p2p;
		p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
		p2p.SetChannelAttribute ("Delay", TimeValue (Seconds (linkDelay)));
		p2p.SetDeviceAttribute ("Mtu", UintegerValue (mtu));
		remoteHostDevices = p2p.Install (remoteHost, enbNodes.Get(u));

//		CsmaHelper csma;
//		csma.SetChannelAttribute ("DataRate", DataRateValue (dataRate));
//		csma.SetChannelAttribute ("Delay", TimeValue (Seconds (linkDelay)));
//		remoteHostDevices = csma.Install (remoteHostContainer);
	}

	NS_LOG_UNCOND ("Assigning IP to Remote Host");
	ipv4.SetBase ("10.0.0.0", "255.0.0.0");
	remoteHostWifiIpIfaces = ipv4.Assign (remoteHostDevices);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing Wifi");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	YansWifiChannelHelper 	channel = YansWifiChannelHelper::Default ();
	YansWifiPhyHelper 	 	phy 	= YansWifiPhyHelper::Default ();
	QosWifiMacHelper		mac 	= QosWifiMacHelper::Default ();
	WifiHelper 				wifi;

	NetDeviceContainer 		enbApdevice;
	NetDeviceContainer 		ueWifiDevice;

	Ssid ssid = Ssid ("ns3-wifi");

	wifi.SetStandard (WIFI_PHY_STANDARD_80211ad_OFDM);
	wifi.SetRemoteStationManager ("ns3::IdealWifiManager");

	mac.SetType ("ns3::AdhocWifiMac");

	////video trasmission
	mac.SetBlockAckThresholdForAc(AC_VI, 2);
	mac.SetMsduAggregatorForAc (AC_VI, "ns3::MsduStandardAggregator", "MaxAmsduSize", UintegerValue (262143));

	phy.SetChannel (channel.Create ());
	phy.SetErrorRateModel ("ns3::SensitivityModel60GHz");
	phy.Set ("ChannelNumber", UintegerValue(channelNumber));

	Ptr<Measured2DAntenna> m2DAntenna = CreateObject<Measured2DAntenna>();
	m2DAntenna->SetMode(10);

	mac.SetType ("ns3::StaWifiMac","Ssid", SsidValue (ssid),"ActiveProbing", BooleanValue (false));
	ueWifiDevice = wifi.Install (phy, mac, ueNodes);

	mac.SetType ("ns3::ApWifiMac","Ssid",SsidValue (ssid),"BeaconGeneration", BooleanValue (true),"BeaconInterval", TimeValue (Seconds (2.5)));
	enbApdevice = wifi.Install (phy, mac, enbNodes);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing WIFI Mobility");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	NS_LOG_UNCOND ("Installing mobility on remoteHostWIFI");
	MobilityHelper rhmobility;
	Ptr<ListPositionAllocator> rhPositionAlloc = CreateObject<ListPositionAllocator> ();
	rhPositionAlloc = CreateObject<ListPositionAllocator> ();
	rhPositionAlloc->Add (Vector (0.0, -20.0, 0.0));
	rhmobility.SetPositionAllocator (rhPositionAlloc);
	rhmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	rhmobility.Install (remoteHostContainer);
	BuildingsHelper::Install (remoteHostContainer);

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

	ipv4.SetBase ("192.168.1.0", "255.255.255.0");
	Ipv4InterfaceContainer wifiApInterface 		= ipv4.Assign (enbApdevice);
	Ipv4InterfaceContainer wifiUeIPInterface 	= ipv4.Assign (ueWifiDevice);

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing Applications");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

    for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
    	Ipv4Address remoteHostWIFIAddr 	= remoteHostWifiIpIfaces.GetAddress (0);
    	Ipv4Address ueWIFIAddr 			= wifiUeIPInterface.GetAddress (u);

		if(useUdp)
		{
			if(useDl)
			{
				NS_LOG_UNCOND ("==> Installing UDP DL app for UE MMWAVE "  << u);
				PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
				serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));

				UdpClientHelper dlClient (ueWIFIAddr, dlPort);
				dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(UDPPacketInterval)));
				dlClient.SetAttribute ("MaxPackets", UintegerValue(maxPackets));
				clientApps.Add (dlClient.Install (remoteHost));

				UdpServerHelper udpServer = UdpServerHelper (dlPort);
				clientApps = udpServer.Install (ueNodes.Get(u));
			}

			if(useUl)
			{
				NS_LOG_UNCOND ("==> Installing UDP UL app for UE MMWAVE "  << u);
				PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
				serverApps.Add (ulPacketSinkHelper.Install (remoteHost));

				UdpClientHelper ulClient (remoteHostWIFIAddr, ulPort);
				ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(UDPPacketInterval)));
				ulClient.SetAttribute ("MaxPackets", UintegerValue(maxPackets));
				clientApps.Add (ulClient.Install (ueNodes.Get(u)));

				UdpServerHelper udpServer = UdpServerHelper (ulPort);
				clientApps = udpServer.Install (remoteHost);
			}
		}
		else
		{
			if(useDl)
			{
			  NS_LOG_UNCOND ("==> Installing TCP DL app for UE WIFI "  << u);
			  PacketSinkHelper sinkDl ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
			  serverApps.Add (sinkDl.Install (ueNodes.Get(u)));

			  BulkSendHelper sourceDl ("ns3::TcpSocketFactory", InetSocketAddress (ueWIFIAddr, dlPort));
			  sourceDl.SetAttribute ("MaxBytes", UintegerValue (maxTCPBytes));
			  sourceDl.SetAttribute ("SendSize", UintegerValue (10000));
			  clientApps.Add (sourceDl.Install (remoteHost));
			}
			if(useUl)
			{
			  NS_LOG_UNCOND ("==> Installing TCP UL app for UE WIFI "  << u);
			  PacketSinkHelper sinkUl ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
			  serverApps.Add (sinkUl.Install (remoteHost));

			  BulkSendHelper sourceUl ("ns3::TcpSocketFactory", InetSocketAddress (remoteHostWIFIAddr, ulPort));
			  sourceUl.SetAttribute ("MaxBytes", UintegerValue (maxTCPBytes));
			  sourceUl.SetAttribute ("SendSize", UintegerValue (10000));
			  clientApps.Add (sourceUl.Install (ueNodes.Get(u)));
			}
		}
    }

    serverApps.Start (Seconds (serverStartTime));
	clientApps.Start (Seconds (clientStartTime));

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
	std::cout << "Mtu: " << mtu << "\n";
	std::cout << "Area: " << (boxArea.xMax - boxArea.xMin) * (boxArea.yMax - boxArea.yMin) << "m²\n";
	std::cout << "========================= " << "\n\n\n";

	Simulator::Stop (Seconds (simulationTime));

	if (tracing)
	{
	  //p2p.EnablePcapAll (outFile);
	  phy.EnablePcap (outFile, enbApdevice.Get (0));
	  phy.EnablePcapAll (outFile, true);
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

