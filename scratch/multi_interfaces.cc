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

void showConfigs(uint32_t,uint32_t,double,bool,uint32_t,std::string,uint32_t,Box,double);
void flowmonitorOutput(Ptr<FlowMonitor>, FlowMonitorHelper&, std::string, double,uint32_t);

int main (int argc, char *argv[])
{
	double simulationTime				= 2;

	bool verbose						= false; // packetSink dataFlow
	bool tracing						= false;
	bool netAnim						= false;
	bool flowmonitor					= true;

	double serverStartTime				= 0.0;

	//Area Vars
	double BoxXmin						= 0;
	double BoxXmax						= 15;
	double BoxYmin						= 0;
	double BoxYmax						= 15;

	//Femtocells Vars
	bool useFemtocells					= true;
	uint32_t nFemtocells				= 1;
	uint32_t nFloors					= 1;
	uint32_t nApartmentsX				= 1;

	//APP Vars
    std::string protocol 				= "ns3::UdpSocketFactory";
	std::string dataRate 				= "300Mbps";
	uint32_t packetSize					= 1024;

	//WIFI Vars
	bool use2DAntenna					= false;
	uint32_t wifiChannelNumber			= 1;
	uint32_t nTxAntennas 				= 1;
	uint32_t nRxAntennas				= 1;
	uint32_t maxAmsduSize				= 999999;//262143;

	//Node Vars
	double ueSpeed						= 1.0; // m/s.
	uint16_t nEnb 						= 2;
	uint16_t nUe 						= 1;

	std::string outFileName				= "debug";

	Ipv4InterfaceContainer 				wifiApInterface, wifiUeIPInterface;
	Ipv4AddressHelper 					ipv4Enb, ipv4UE;
	InternetStackHelper 				internet;
	Ipv4StaticRoutingHelper 			ipv4RoutingHelper;
    ApplicationContainer 				serverApps, clientApps;
	Box 								boxArea;

///////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Setting Up Configs");
///////////////////////////////////////////////

	/* No fragmentation and no RTS/CTS */
	Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("99999"));
	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("99999"));

	Config::SetDefault ("ns3::OnOffApplication::DataRate", DataRateValue(DataRate(dataRate)));
    Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (packetSize));

	// Set to true if this interface should respond to interface events by globallly recomputing routes
	//Config::SetDefault ("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue (true));

///////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Setting Up Command Line Parameters");
///////////////////////////////////////////////

	 if (verbose)
	 {
		 LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
	 }

	CommandLine cmd;
	cmd.AddValue("simulationTime", "Simulation Time: ", simulationTime);
	cmd.AddValue("nEnb", "How many ENBs: ", nEnb);
	cmd.AddValue("nUe", "How many UEs: ", nUe);
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

	NS_ASSERT (nEnb > 0 && nUe > 0);
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

	NodeContainer enbNodes;
	enbNodes.Create(nEnb);

	NodeContainer ueNodes;
	ueNodes.Create(nUe);

	internet.Install(enbNodes);
	internet.Install(ueNodes);

	ipv4Enb.SetBase ("1.0.0.0", "255.0.0.0");
	ipv4UE.SetBase ("2.0.0.0", "255.0.0.0");

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing Wifi");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	YansWifiChannelHelper 	wifiChannel = YansWifiChannelHelper::Default ();
	YansWifiPhyHelper 	 	wifiPhy		= YansWifiPhyHelper::Default ();
	QosWifiMacHelper		wifiMac 	= QosWifiMacHelper::Default ();
	WifiHelper 				wifi;

	NetDeviceContainer 		enbApdevice;
	NetDeviceContainer 		ueWifiDevice;

	for (uint32_t i = 0; i < enbNodes.GetN (); ++i)
	{
//		std::ostringstream oss;
//		oss << "ns3-wifi-" << i;
//		Ssid ssid = Ssid (oss.str ());
		Ssid ssid = Ssid ("ns3-wifi");

		wifi.SetStandard (WIFI_PHY_STANDARD_80211ad_OFDM);
		wifi.SetRemoteStationManager ("ns3::IdealWifiManager", "BerThreshold",DoubleValue(1e-6));
		//wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",StringValue ("OfdmRate7Gbps"), "ControlMode",StringValue ("OfdmRate2Gbps"));

		wifiMac.SetType ("ns3::FlywaysWifiMac");

		////video trasmission
		wifiMac.SetBlockAckThresholdForAc(AC_VI, 2);
		wifiMac.SetMsduAggregatorForAc (AC_VI, "ns3::MsduStandardAggregator", "MaxAmsduSize", UintegerValue (maxAmsduSize));

		wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
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
		enbApdevice = wifi.Install (wifiPhy, wifiMac, enbNodes.Get(i));
		wifiApInterface = ipv4Enb.Assign (enbApdevice);

		wifiMac.SetType ("ns3::StaWifiMac","Ssid", SsidValue (ssid),"ActiveProbing", BooleanValue (false));
		ueWifiDevice = wifi.Install (wifiPhy, wifiMac, ueNodes);
		wifiUeIPInterface = ipv4UE.Assign (ueWifiDevice);

		if(use2DAntenna)
		{
			Ptr<Measured2DAntenna> m2DAntenna = CreateObject<Measured2DAntenna>();
			m2DAntenna->SetMode(10);

			enbApdevice.Get(i)->GetObject<WifiNetDevice>()->GetPhy()->AggregateObject(m2DAntenna);

			for (uint32_t u = 0; u < nUe; ++u)
				ueWifiDevice.Get(u)->GetObject<WifiNetDevice>()->GetPhy()->AggregateObject(m2DAntenna);
		}

		//////////////////////////////////////////////////////
		//////////////////////////////////////////////////////
			NS_LOG_UNCOND ("==> Assigning Routing Tables on ENB " << i);
		/////////////////////////////////////////////////////
		/////////////////////////////////////////////////////

		Ptr<Node> enb = enbNodes.Get (i);
		Ptr<Ipv4StaticRouting> enbStaticRouting = ipv4RoutingHelper.GetStaticRouting (enb->GetObject<Ipv4> ());
		enbStaticRouting->AddHostRouteTo(wifiUeIPInterface.GetAddress(0),1);
		enbStaticRouting->AddNetworkRouteTo("2.0.0.0","255.0.0.0", 1); //AddNetwork Route To UEnode

		for (uint32_t u = 0; u < nUe; ++u)
		{
			NS_LOG_UNCOND ("==> Assigning Routing Tables on UE " << u);
			Ptr<Node> ue = ueNodes.Get (u);
			Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());
			ueStaticRouting->AddHostRouteTo(wifiApInterface.GetAddress(0),i+1);
			ueStaticRouting->AddNetworkRouteTo("1.0.0.0","255.0.0.0", i+1); //AddNetwork Route To enbNode

			NS_LOG_UNCOND ("Starting Apps on UE Interface " << u);
			InetSocketAddress dst = InetSocketAddress (wifiUeIPInterface.GetAddress (u), 9);
			OnOffHelper onOffHelper = OnOffHelper (protocol, dst);
			//onOffHelper.SetConstantRate (DataRate (dataRate));
		    onOffHelper.SetAttribute ("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
		    onOffHelper.SetAttribute ("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

			NS_LOG_UNCOND ("Starting Apps on ENB " << i);
			ApplicationContainer apps = onOffHelper.Install (enbNodes.Get(i));
			apps.Start (Seconds (serverStartTime));

			PacketSinkHelper sink = PacketSinkHelper (protocol, dst);
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
	std::cout << std::endl;
	NS_LOG_UNCOND ("//////////////////////////");
	NS_LOG_UNCOND ("==> Starting Simulation <==");
	NS_LOG_UNCOND ("//////////////////////////");
/////////////////////////////////////////////////////

	Simulator::Stop (Seconds (simulationTime));

	if (tracing)
	{
	  ////wifiPhy.EnablePcap (outFile, enbApdevice.Get (0));
		wifiPhy.EnablePcapAll (outFileName, true);
	}

	if(netAnim)
	{
		AnimationInterface anim (outFileName+"_anim.xml");
	 	//anim.SetConstantPosition (wifiApNode, 0.0, 0.0);
	 	//anim.SetConstantPosition (ueNode, distanceXUe, distanceYUe);
	}

	if(flowmonitor)
	{
		FlowMonitorHelper flowmon;
		Ptr<FlowMonitor> monitor = flowmon.InstallAll();

		Simulator::Run ();

		showConfigs(nEnb, nUe, ueSpeed, useFemtocells, nFemtocells, dataRate, packetSize, boxArea, simulationTime);
		flowmonitorOutput(monitor, flowmon, outFileName, simulationTime, packetSize);
	}
	else
	{
		Simulator::Run ();
		showConfigs(nEnb, nUe, ueSpeed, useFemtocells, nFemtocells, dataRate, packetSize, boxArea, simulationTime);
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
	monitor->SerializeToXmlFile (outFileName+"_monitor.xml", true, true);

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
