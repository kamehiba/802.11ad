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

  TX1
   |
   |
   AP
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
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/internet-module.h"
#include "ns3/config-store-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor.h"
#include "ns3/netanim-module.h"

using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("debug");

void PrintLocations (NodeContainer,std::string);
void PrintAddresses(Ipv4InterfaceContainer,std::string);

int main (int argc, char *argv[])
{
	LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

	double simulationTime			= 30;

	bool use802_11ad				= true; // false will use 802.11g

	bool tracing					= false;
	bool flowmonitor				= false;

	double wifiServerStartTime		= 0.01;
	double wifiClientStartTime		= 0.01;

	double distanceXUe				= 10.0;
	double distanceYUe				= 0.0;

	uint32_t dlPort 				= 12345;
	uint32_t ulPort 				= 9;

	uint32_t maxTCPBytes 			= 0;
	double udpPacketInterval 		= 1;

	bool   useUdp					= true;

	bool   useDl					= true;
	bool   useUl					= false;

	bool   useappWIFI				= true;

	uint16_t nTransmitedAntennas 	= 1;
	uint16_t nReceiveAntennas 		= 1;

	std::string outFile ("debug");
	std::string p2pWifiRate ("500Gbps");

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

	Ptr<ListPositionAllocator> 	positionAlloc 	= CreateObject<ListPositionAllocator> ();

	PointToPointHelper 			p2p;
	Ipv4AddressHelper 			ipv4;
	InternetStackHelper 		internet;
	MobilityHelper 				mobility;
	Ipv4StaticRoutingHelper 	ipv4RoutingHelper;

///////////////////////////////////////////////////////////
	NS_LOG_UNCOND ("->Initializing Remote Host WIFI (TX1)...");
///////////////////////////////////////////////////////////

	NS_LOG_UNCOND ("Creating p2pNodes WIFI...");
	NodeContainer remoteHostWIFIContainer;
	remoteHostWIFIContainer.Create (2);

	internet.Install(remoteHostWIFIContainer); //txNodeWifi and wifiApNode

	NS_LOG_UNCOND ("Creating pointToPoint WIFI...");
	p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (p2pWifiRate)));
	p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));
	p2p.SetDeviceAttribute ("Mtu", UintegerValue (1500));

	NS_LOG_UNCOND ("Creating p2pDevices WIFI...");
	NetDeviceContainer p2pDevicesWIFI;
	p2pDevicesWIFI = p2p.Install (remoteHostWIFIContainer);

	Ptr<Node> remoteHostWIFI 	= remoteHostWIFIContainer.Get (0);
	Ptr<Node> wifiApNode 		= remoteHostWIFIContainer.Get (1);

///////////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Creating UE/Enb Node...");
///////////////////////////////////////////////////////////

	NodeContainer ueNodeContainer;
	ueNodeContainer.Create (1);

	internet.Install(ueNodeContainer);

	Ptr<Node> ueNode = ueNodeContainer.Get (0);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing Wifi...");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	YansWifiChannelHelper 	channel = YansWifiChannelHelper::Default ();
	YansWifiPhyHelper 	 	phy 	= YansWifiPhyHelper::Default ();
	WifiHelper 				wifi;

	NetDeviceContainer 		ueWifiDevice;
	NetDeviceContainer 		wifiApdevice;

	uint32_t channelNumber = 1;
	Ssid ssid = Ssid ("ns3-wifi");

	if(use802_11ad)
	{
		QosWifiMacHelper mac = QosWifiMacHelper::Default ();

		wifi.SetStandard (WIFI_PHY_STANDARD_80211ad_OFDM);
		wifi.SetRemoteStationManager ("ns3::IdealWifiManager", "BerThreshold",DoubleValue(1e-9));
		//wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate7Gbps"), "RtsCtsThreshold", UintegerValue (0));

		////video trasmission
		//mac.SetType ("ns3::AmpduTag");
		mac.SetType ("ns3::AdhocWifiMac");
		mac.SetBlockAckThresholdForAc(AC_VI, 2);
		mac.SetMsduAggregatorForAc (AC_VI, "ns3::MsduStandardAggregator", "MaxAmsduSize", UintegerValue (262143));

		channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
		//channel.AddPropagationLoss ("ns3::FriisPropagationLossModel","Lambda", DoubleValue(3e8/60.0e9));

		phy.SetChannel (channel.Create ());
		phy.SetErrorRateModel ("ns3::SensitivityModel60GHz");

		phy.Set ("ChannelNumber", UintegerValue(channelNumber));
//		phy.Set ("TxPowerStart", DoubleValue (10.0));
//		phy.Set ("TxPowerEnd", DoubleValue (10.0));
//		phy.Set ("TxPowerLevels", UintegerValue (1));
//		phy.Set ("TxGain", DoubleValue (0));
//		phy.Set ("RxGain", DoubleValue (0));
//		phy.Set ("RxNoiseFigure", DoubleValue (10));
//		phy.Set ("CcaMode1Threshold", DoubleValue (-79));
//		phy.Set ("EnergyDetectionThreshold", DoubleValue (-79 + 3));


		//phy.SetAntenna ("ns3::ConeAntenna", "Beamwidth", DoubleValue(ConeAntenna::GainDbiToBeamwidth(0))); //Antenna_Cone
		//phy.SetAntenna ("ns3::Measured2DAntenna", "Mode", DoubleValue(10)); //Antenna_Measured



		mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false));
		ueWifiDevice = wifi.Install (phy, mac, ueNode);

		mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
		wifiApdevice = wifi.Install (phy, mac, wifiApNode);
	}
	else
	{
		NqosWifiMacHelper mac = NqosWifiMacHelper::Default (); //802.11a,b,g

		wifi.SetStandard (WIFI_PHY_STANDARD_80211g);
		wifi.SetRemoteStationManager ("ns3::IdealWifiManager", "BerThreshold",DoubleValue(1e-9));

		channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

		phy.SetChannel (channel.Create ());
		phy.SetErrorRateModel ("ns3::YansErrorRateModel");
		phy.Set("ChannelNumber", UintegerValue(channelNumber));

		mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false));
		ueWifiDevice = wifi.Install (phy, mac, ueNode);

		mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
		wifiApdevice = wifi.Install (phy, mac, wifiApNode);
	}

	Ptr<NetDevice> ndClient = ueWifiDevice.Get (0);
	Ptr<NetDevice> ndServer = wifiApdevice.Get (0);
	Ptr<WifiNetDevice> wndClient = ndClient->GetObject<WifiNetDevice> ();
	Ptr<WifiNetDevice> wndServer = ndServer->GetObject<WifiNetDevice> ();
	Ptr<WifiPhy> wifiPhyPtrClient = wndClient->GetPhy ();
	Ptr<WifiPhy> wifiPhyPtrServer = wndServer->GetPhy ();
	wifiPhyPtrClient->SetNumberOfTransmitAntennas (nTransmitedAntennas);
	wifiPhyPtrClient->SetNumberOfReceiveAntennas (nReceiveAntennas);
	wifiPhyPtrServer->SetNumberOfTransmitAntennas (nTransmitedAntennas);
	wifiPhyPtrServer->SetNumberOfReceiveAntennas (nReceiveAntennas);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing WIFI Mobility...");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	NS_LOG_UNCOND ("Installing mobility on remoteHostWIFI");
	positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (0.0, -20.0, 0.0));
	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (remoteHostWIFI);

	NS_LOG_UNCOND ("Installing mobility on wifiApNode");
	positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (0.0, 0.0, 0.0));
	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (wifiApNode);

	NS_LOG_UNCOND ("Installing mobility on ueNodes");
    MobilityHelper ueMobility;
    ueMobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
    ueMobility.Install (ueNode);
    ueNode->GetObject<MobilityModel> ()->SetPosition (Vector (distanceXUe, distanceYUe, 0.0));
    ueNode->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (1.0, 0.0, 0.0));

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing WIFI InternetStackHelper...");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	ipv4.SetBase ("60.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer wifiTxIPInterface = ipv4.Assign (p2pDevicesWIFI);

    ipv4.SetBase ("192.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer wifiApInterface 		= ipv4.Assign (wifiApdevice);
    Ipv4InterfaceContainer wifiUeIPInterface 	= ipv4.Assign (ueWifiDevice);

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing Applications...");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

    ApplicationContainer wifiClient;
    ApplicationContainer wifiServer;

    Ipv4Address remoteHostWIFIAddr 	= wifiTxIPInterface.GetAddress (0);
    Ipv4Address ueWIFIAddr 			= wifiUeIPInterface.GetAddress (0);

    if(useUdp)
    {
    	if(useappWIFI)
    	{
			if(useDl)
			{
				NS_LOG_UNCOND ("Installing UDP DL app for UE WIFI ");
				PacketSinkHelper dlPacketSinkHelperWIFI ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
				wifiServer.Add (dlPacketSinkHelperWIFI.Install (ueNode));

				UdpClientHelper dlClientWIFI (ueWIFIAddr, dlPort);
				dlClientWIFI.SetAttribute ("Interval", TimeValue (MilliSeconds(udpPacketInterval)));
				dlClientWIFI.SetAttribute ("MaxPackets", UintegerValue(1000000));
				wifiClient.Add (dlClientWIFI.Install (remoteHostWIFI));
			}
			if(useUl)
			{
				NS_LOG_UNCOND ("Installing UDP UL app for UE WIFI ");
				PacketSinkHelper ulPacketSinkHelperWIFI ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
				wifiServer.Add (ulPacketSinkHelperWIFI.Install (remoteHostWIFI));

				UdpClientHelper ulClientWIFI (remoteHostWIFIAddr, ulPort);
				ulClientWIFI.SetAttribute ("Interval", TimeValue (MilliSeconds(udpPacketInterval)));
				ulClientWIFI.SetAttribute ("MaxPackets", UintegerValue(1000000));
				wifiClient.Add (ulClientWIFI.Install (ueNode));
			}
    	}

    	dlPort++;
    	ulPort++;
    }
    else
    {
    	if(useappWIFI)
    	{
			if(useDl)
			{
			  PacketSinkHelper sinkDl ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
			  wifiServer.Add (sinkDl.Install (ueNode));

			  BulkSendHelper sourceDl ("ns3::TcpSocketFactory", InetSocketAddress (ueWIFIAddr, dlPort));
			  sourceDl.SetAttribute ("MaxBytes", UintegerValue (maxTCPBytes));
			  sourceDl.SetAttribute ("SendSize", UintegerValue (10000));
			  wifiClient.Add (sourceDl.Install (remoteHostWIFI));
			}
			if(useUl)
			{
			  PacketSinkHelper sinkUl ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
			  wifiServer.Add (sinkUl.Install (remoteHostWIFI));

			  BulkSendHelper sourceUl ("ns3::TcpSocketFactory", InetSocketAddress (remoteHostWIFIAddr, ulPort));
			  sourceUl.SetAttribute ("MaxBytes", UintegerValue (maxTCPBytes));
			  sourceUl.SetAttribute ("SendSize", UintegerValue (10000));
			  wifiClient.Add (sourceUl.Install (ueNode));
			}
    	}

    	dlPort++;
    	ulPort++;
    }

	wifiServer.Start (Seconds (wifiServerStartTime));
	wifiClient.Start (Seconds (wifiClientStartTime));

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==>Running Simulation...");
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	Simulator::Stop (Seconds (simulationTime));

	if (tracing)
	{
	  p2p.EnablePcapAll (outFile);
	  phy.EnablePcap (outFile, wifiApdevice.Get (0));
	  phy.EnablePcapAll (outFile, true);
	}

	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll();

	PrintAddresses(wifiTxIPInterface, "IP addresses of WIFI Remote Host (TX1)");
	PrintAddresses(wifiApInterface, "IP address of AP");
	PrintAddresses(wifiUeIPInterface, "IP addresses of wifi base stations(RX1)");
	PrintLocations(remoteHostWIFI, "Location of WIFI Remote Host");
	PrintLocations(wifiApNode, "Location of WIFI AP");
	PrintLocations(ueNode, "Location of StatNodes");

	Simulator::Run ();
	if(flowmonitor)
	{
		AnimationInterface anim (outFile+"_anim.xml");
	 	anim.SetConstantPosition (remoteHostWIFI, 0.0, -20.0);
	 	anim.SetConstantPosition (wifiApNode, 0.0, 0.0);
	 	anim.SetConstantPosition (ueNode, distanceXUe, distanceYUe);

		monitor->CheckForLostPackets ();
		monitor->SerializeToXmlFile (outFile+"_monitor.xml", true, true);
	}
	Simulator::Destroy ();

	return 0;
}

void PrintLocations (NodeContainer nodes, std::string header)
{
	std::cout << header << std::endl;
	for(NodeContainer::Iterator iNode = nodes.Begin (); iNode != nodes.End (); ++iNode)
	{
		Ptr<Node> object = *iNode;
		Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
		NS_ASSERT (position != 0);
		Vector pos = position->GetPosition ();
		std::cout << "(" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
	}

	std::cout << std::endl;
}

void PrintAddresses(Ipv4InterfaceContainer container, std::string header)
{
	std::cout << header << std::endl;
	uint32_t nNodes = container.GetN ();

	for (uint32_t i = 0; i < nNodes; ++i)
		std::cout << container.GetAddress(i, 0) << std::endl;

	std::cout << std::endl;
}
