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


	RH wifi				RH Lte
 	 *					*
	 -					-
	 -					-
	 Router				PGW
	 *					*
	 -					-
	 -					-
   	 APs				ENB
   	 *					*
   	 -					-
   	 -	-			-
   	 	 	 -
   	 	 	 *Sta

*/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/stats-module.h"
#include "ns3/wifi-module.h"
#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include <ns3/point-to-point-epc-helper.h>
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include <ns3/buildings-module.h>
#include "ns3/config-store-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor.h"
#include "ns3/netanim-module.h"
#include "ns3/stats-module.h"
#include <fstream>
#include <iostream>
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("debug");

#define color(param) printf("\033[%sm",param)

class Experiment
{
public:

	Experiment ();
	void showConfigs();
	void flowmonitorOutput(Ptr<FlowMonitor> monitor, FlowMonitorHelper *flowmon);
	Gnuplot2dDataset Run (const WifiHelper &wifi, const YansWifiPhyHelper &wifiPhy,
						  const QosWifiMacHelper &wifiMac, const YansWifiChannelHelper &wifiChannel);

	void setSimulationTime(double var) {m_simulationTime = var;};
	double getSimulationTime() { return m_simulationTime;};

	void setAppStartTime(double var) {m_appStartTime = var;};
	double getAppStartTime() { return m_appStartTime;};

	void setStaSpeed(double var) {m_staSpeed = var;};
	double getStaSpeed() { return m_staSpeed;};

	void setFlowmonitor(bool var) {m_flowmonitor = var;};
	bool getFlowMonitor() { return m_flowmonitor;};

	void setTracing(bool var) {m_tracing = var;};
	bool getTracing() { return m_tracing;};

	void setNetanim(bool var) {m_netAnim = var;};
	bool getNetanim() { return m_netAnim;};

	void setStopApp(bool var) {m_stopApp = var;};
	bool getStopApp() { return m_stopApp;};

	void setShowPacketSink(bool var) {m_showPacketSink = var;};
	bool getShowPacketSink() { return m_showPacketSink;};

	void setShowSimulationTime(bool var) {m_showSimulationTime = var;};
	bool getShowSimulationTime() { return m_showSimulationTime;};

	void setShowConfigs(bool var) {m_showConfigs = var;};
	bool getShowConfigs() { return m_showConfigs;};

	void setEnableLteEpsBearer(bool var) {m_enableLteEpsBearer = var;};
	bool getEnableLteEpsBearer() { return m_enableLteEpsBearer;};

	void setWifiChannelNumber(uint32_t var) {m_wifiChannelNumber = var;};
	uint32_t getWifiChannelNumber() { return m_wifiChannelNumber;};

	void setNTxAntennas(uint32_t var) {m_nTxAntennas = var;};
	uint32_t getNTxAntennas() { return m_nTxAntennas;};

	void setNRxAntennas(uint32_t var) {m_nRxAntennas = var;};
	uint32_t getNRxAntennas() { return m_nRxAntennas;};

	void setMaxAmsduSize(uint32_t var) {m_maxAmsduSize = var;};
	uint32_t getMaxAmsduSize() { return m_maxAmsduSize;};

	void setBytesTotal(uint32_t var) {m_bytesTotal = var;};
	uint32_t getBytesTotal() { return m_bytesTotal;};

	void setNApoints(uint32_t var) {m_nApoints = var;};
	uint32_t getNApoints() { return m_nApoints;};

	void setNStations(uint32_t var) {m_nStations = var;};
	uint32_t getNStations() { return m_nStations;};

	void setPacketSize(uint32_t var) {m_packetSize = var;};
	uint32_t getPacketSize() { return m_packetSize;};

	void setMtu(uint32_t var) {m_mtu = var;};
	uint32_t getMtu() { return m_mtu;};

	void setP2pLinkDelay(uint32_t var) {m_p2pLinkDelay = var;};
	uint32_t getP2pLinkDelay() { return m_p2pLinkDelay;};

	void setWifiPort(uint32_t var) {m_wifiPort = var;};
	uint32_t getWifiPort() { return m_wifiPort;};

	void setLtePort(uint32_t var) {m_ltePort = var;};
	uint32_t getLtePort() { return m_ltePort;};

	void setDataRate(std::string var) {m_dataRate = var;};
	std::string getDataRate() { return m_dataRate;};

	void setOutFileName(std::string var) {m_outFileName = var;};
	std::string getOutFileName() { return m_outFileName;};

	void setGnuplotFileName(std::string var) {m_gnuplotFileName = var;};
	std::string getGnuplotFileName() { return m_gnuplotFileName;};

	void setProtocol(std::string var) {m_protocol = var;};
	std::string getProtocol() { return m_protocol;};

	void setBoxArea(Box var) {m_boxArea = var;};
	Box getBoxArea() { return m_boxArea;};

	void setBoxXmin(double var) {m_BoxXmin = var;};
	double getBoxXmin() { return m_BoxXmin;};

	void setBoxXmax(double var) {m_BoxXmax = var;};
	double getBoxXmax() { return m_BoxXmax;};

	void setBoxYmin(double var) {m_BoxYmin = var;};
	double getBoxYmin() { return m_BoxYmin;};

	void setBoxYmax(double var) {m_BoxYmax = var;};
	double getBoxYmax() { return m_BoxYmax;};

	void setIpRemoteHost(Ipv4Address var) {m_ipRemoteHost = var;};
	Ipv4Address getIpRemoteHost() { return m_ipRemoteHost;};

	void setIpRouter(Ipv4Address var) {m_ipRouter = var;};
	Ipv4Address getIpRouter() { return m_ipRouter;};

	void setIpWifi(Ipv4Address var) {m_ipWifi = var;};
	Ipv4Address getIpWifi() { return m_ipWifi;};

	void setNetMask(Ipv4Mask var) {m_netMask = var;};
	Ipv4Mask getNetMask() { return m_netMask;};

private:

	void ReceivePacket (Ptr<Socket> socket);
	void GetRate (Ptr<Node> node);
	Ptr<Socket> SetupPacketReceive (Ptr<Node> node);

	double m_simulationTime;
	double m_appStartTime;

	bool m_flowmonitor;
	bool m_tracing;
	bool m_netAnim;
	bool m_stopApp;
	bool m_showPacketSink;
	bool m_showSimulationTime;
	bool m_showConfigs;
	bool m_enableLteEpsBearer;

	uint32_t m_wifiChannelNumber;
	uint32_t m_nTxAntennas;
	uint32_t m_nRxAntennas;
	uint32_t m_maxAmsduSize;
	uint32_t m_bytesTotal;
	uint32_t m_mtu;
	uint32_t m_p2pLinkDelay;
	uint32_t m_wifiPort;
	uint32_t m_ltePort;

	double m_staSpeed;
	uint32_t m_nApoints;
	uint32_t m_nStations;
	uint32_t m_packetSize;

	std::string m_dataRate;
	std::string m_outFileName;
	std::string m_gnuplotFileName;
	std::string m_protocol;

	//Area Vars
	double m_BoxXmin;
	double m_BoxXmax;
	double m_BoxYmin;
	double m_BoxYmax;
	Box m_boxArea;

	Ipv4Address m_ipRemoteHost;
	Ipv4Address m_ipRouter;
	Ipv4Address m_ipWifi;
	Ipv4Mask	m_netMask;

	Gnuplot2dDataset m_output;
};

Experiment::Experiment ()
{
	m_output.SetStyle (Gnuplot2dDataset::LINES);

	m_outFileName 			= "debug";
	m_gnuplotFileName		= "80211g.png";

	m_simulationTime 		= 10;
	m_appStartTime 			= 0.01;

	m_showPacketSink		= false;
	m_showConfigs			= false;
	m_flowmonitor 			= false;
	m_tracing				= false;
	m_netAnim				= false;
	m_showSimulationTime	= false;

	//P2P Vars
	m_mtu					= 1500; // p2p Mtu
	m_p2pLinkDelay			= 0.010;// p2p link Delay

	//Area Vars
	m_BoxXmin				= 0;
	m_BoxXmax				= 25;
	m_BoxYmin				= 0;
	m_BoxYmax				= 10;
	m_boxArea = Box (m_BoxXmin, m_BoxXmax, m_BoxYmin, m_BoxYmax, 0, 0);

	//APP Vars
	m_dataRate 				= "512Kb/s";
	m_packetSize 			= 1472;
	m_wifiPort				= 9;
	m_ltePort				= 10;
	m_bytesTotal 			= 0;
	m_stopApp				= false;
	m_enableLteEpsBearer	= true;
	m_protocol 				= "ns3::UdpSocketFactory"; //= "ns3::PacketSocketFactory";

	//IPs
	m_ipRemoteHost			= "1.0.0.0";
	m_ipRouter				= "2.0.0.0";
	m_ipWifi				= "3.0.0.0";
	m_netMask				= "255.0.0.0";

	//WIFI Vars
	m_wifiChannelNumber 	= 1;
	m_nTxAntennas 			= 1;
	m_nRxAntennas 			= 1;
	m_maxAmsduSize 			= 262143;

	//Nodes Vars
	m_staSpeed				= 1.0; 	// m/s.
	m_nApoints 				= 1;
	m_nStations 			= 1;
}

void
Experiment::GetRate (Ptr<Node> node)
{
	uint32_t timeNow = Simulator::Now().GetSeconds ();
	double mbs = ((m_bytesTotal * 8.0) / 1000000);
	m_bytesTotal = 0;
	m_output.Add (timeNow, mbs);

	if (timeNow == m_simulationTime)
	  return;

	Simulator::Schedule (Seconds (1.0), &Experiment::GetRate, this, node);
}

void
Experiment::ReceivePacket (Ptr<Socket> socket)
{
	Ptr<Packet> packet;

	while ((packet = socket->Recv ()))
	  m_bytesTotal += packet->GetSize ();
}

Ptr<Socket>
Experiment::SetupPacketReceive (Ptr<Node> node)
{
	TypeId tid = TypeId::LookupByName ("ns3::PacketSocketFactory");
	Ptr<Socket> sink = Socket::CreateSocket (node, tid);
	sink->Bind ();
	sink->SetRecvCallback (MakeCallback (&Experiment::ReceivePacket, this));
	return sink;
}

void
Experiment::showConfigs()
{
	std::cout << std::endl;
	std::cout << "==========CONFIGS======== " << "\n";
	std::cout << "Access Points: " << this->getNApoints() << "\n";
	std::cout << "Stations: " << this->getNStations() << "\n";
	std::cout << "UE Speed: " << this->getStaSpeed() << "m/s" << " <> " << this->getStaSpeed()*3.6 << "km/h\n";
	std::cout << "DataRate: " << this->getDataRate() << "\n";
	std::cout << "Area: " << (this->getBoxArea().xMax - this->getBoxArea().xMin) * (this->getBoxArea().yMax - this->getBoxArea().yMin) << "m²\n";
	std::cout << "========================= " << "\n";
}

void
Experiment::flowmonitorOutput(Ptr<FlowMonitor> monitor, FlowMonitorHelper *flowmon)
{
	double difftx=0.0, diffrx=0.0, diffrxtx=0.0, txbitrate_value=0.0, txOffered=0.0, rxbitrate_value=0.0, delay_value=0.0, throughput=0.0;
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon->GetClassifier ());
	FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();

	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
	{
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);

		difftx = i->second.timeLastTxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds();
		diffrx = i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstRxPacket.GetSeconds();
		diffrxtx = i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds();

		txbitrate_value = (double) i->second.txBytes * 8 / difftx / 1024 / 1024;
		txOffered = (double) i->second.txBytes * 8.0 / difftx / 1024 / 1024;

		if (i->second.rxPackets != 0)
		{
			rxbitrate_value = (double) i->second.rxBytes * 8 / diffrx / 1024 / 1024;
			delay_value = (double) i->second.delaySum.GetSeconds() / (double) i->second.rxPackets;
			throughput = (double) i->second.rxBytes * 8.0 / diffrxtx / 1024 / 1024;
		}
		else
		{
			rxbitrate_value = 0;
			delay_value = 0;
			throughput = 0;
		}

		std::cout << "========================= " << "\n";
		color("31");
		std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
		color("0");
		std::cout << "  Tx Packets: " << i->second.txPackets 	<< "\n";
		std::cout << "  Tx Bytes:   " << i->second.txBytes 		<< "\n";
		std::cout << "  Tx bitrate: " << txbitrate_value 		<< " Mbps\n";
		std::cout << "  TxOffered:  " << txOffered 				<< " Mbps\n\n";

		std::cout << "  Rx Packets: " << i->second.rxPackets 	<< "\n";
		std::cout << "  Rx Bytes:   " << i->second.rxBytes 		<< "\n";
		std::cout << "  Rx bitrate: " << rxbitrate_value 		<< " Mbps\n\n";

		std::cout << "  Throughput: " << throughput 			<< " Mbps\n\n";

		std::cout << "  Lost Packets: " 	<< i->second.lostPackets 			<< "\n";
		std::cout << "  Dropped Packets: " 	<< i->second.packetsDropped.size() 	<< "\n";
		std::cout << "  JitterSum: " 		<< i->second.jitterSum 				<< "\n";
		std::cout << "  Average delay: " 	<< delay_value 						<< "s\n";
	}
}

Gnuplot2dDataset
Experiment::Run (const WifiHelper &wifi, const YansWifiPhyHelper &wifiPhy,
                 const QosWifiMacHelper &wifiMac, const YansWifiChannelHelper &wifiChannel)
{
///////////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Creating WIFI Nodes");
///////////////////////////////////////////////////////////

	NodeContainer remoteHostNode;
	remoteHostNode.Create (1);
	Ptr<Node> remoteHost = remoteHostNode.Get (0);

	NodeContainer routerNode;
	routerNode.Create (1);
	Ptr<Node> router = routerNode.Get (0);

	NodeContainer wifiApNode;
	wifiApNode.Create(this->getNApoints());

	NodeContainer wifiStaNode;
	wifiStaNode.Create(this->getNStations());

//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Installing p2p");
/////////////////////////////////////////////////////

	NetDeviceContainer 	remoteHostDevice, routerDevice;
	PointToPointHelper 	p2ph;
	p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (this->getDataRate())));
	p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (this->getP2pLinkDelay())));
	p2ph.SetDeviceAttribute ("Mtu", UintegerValue (this->getMtu()));
	remoteHostDevice = p2ph.Install (remoteHost, router);

	for (uint32_t i = 0; i < this->getNApoints(); ++i)
		routerDevice = p2ph.Install (wifiApNode.Get(i), router);

//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing Wifi");//
/////////////////////////////////////////////////////

	YansWifiChannelHelper chan = wifiChannel;
	chan.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

	YansWifiPhyHelper phy = wifiPhy;
	phy.SetChannel (wifiChannel.Create ());
	phy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);
	phy.Set ("ChannelNumber", UintegerValue(this->getWifiChannelNumber()));
	phy.Set ("TxAntennas", UintegerValue (this->getNTxAntennas()));
	phy.Set ("RxAntennas", UintegerValue (this->getNRxAntennas()));
	//phy.Set ("TxGain", DoubleValue (0));
	//phy.Set ("RxGain", DoubleValue (0));
	phy.Set ("TxPowerStart", DoubleValue(10.0));
	phy.Set ("TxPowerEnd", DoubleValue(10.0));
	phy.Set ("TxPowerLevels", UintegerValue(1));
	phy.Set ("RxNoiseFigure", DoubleValue(0));
	phy.Set ("CcaMode1Threshold", DoubleValue (-79) );
	phy.Set ("EnergyDetectionThreshold", DoubleValue (-79+3) );
	phy.Set ("ShortGuardEnabled", BooleanValue (false));// Set guard interval

	QosWifiMacHelper mac = wifiMac;
	mac.SetBlockAckThresholdForAc(AC_VI, 2);
	mac.SetMsduAggregatorForAc (AC_VI, "ns3::MsduStandardAggregator", "MaxAmsduSize", UintegerValue (m_maxAmsduSize));

	Ssid ssid = Ssid ("ns3-80211g");
	NetDeviceContainer wifiAPDevice, wifiStaDevice;

	mac.SetType ("ns3::ApWifiMac","Ssid",SsidValue (ssid),"BeaconGeneration", BooleanValue (true),"BeaconInterval", TimeValue (Seconds (2.5)));
	wifiAPDevice.Add (wifi.Install (phy, mac, wifiApNode));

	mac.SetType ("ns3::StaWifiMac","Ssid", SsidValue (ssid),"ActiveProbing", BooleanValue (true));
	wifiStaDevice.Add (wifi.Install (phy, mac, wifiStaNode));

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Assigning IP to WIFI Devices");
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	Ipv4AddressHelper 		ipv4;
	InternetStackHelper 	internet;
	Ipv4InterfaceContainer 	remoteHostInterface, routerInterface, wifiAPInterface, wifiStaInterface;

	internet.Install(remoteHostNode);
	internet.Install(routerNode);
	internet.Install(wifiApNode);
	internet.Install(wifiStaNode);

	ipv4.SetBase (this->getIpRemoteHost(), this->getNetMask());
	remoteHostInterface	= ipv4.Assign (remoteHostDevice);

	ipv4.SetBase (this->getIpRouter(), this->getNetMask());
	routerInterface	= ipv4.Assign (routerDevice);

	ipv4.SetBase (this->getIpWifi(), this->getNetMask());
	wifiAPInterface = ipv4.Assign (wifiAPDevice);
	wifiStaInterface = ipv4.Assign (wifiStaDevice);

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

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
	rhmobility.Install(remoteHostNode);
	BuildingsHelper::Install (remoteHostNode);

	NS_LOG_UNCOND ("Installing mobility on Router");
	MobilityHelper routermobility;
	Ptr<ListPositionAllocator> routerPositionAlloc = CreateObject<ListPositionAllocator> ();
	routerPositionAlloc = CreateObject<ListPositionAllocator> ();
	routerPositionAlloc->Add (Vector (0.0, 0.0, 0.0));
	routermobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	routermobility.SetPositionAllocator(routerPositionAlloc);
	routermobility.Install(routerNode);
	BuildingsHelper::Install (routerNode);

	NS_LOG_UNCOND ("Installing mobility on wifiApNode");
	MobilityHelper apMobility;
	Ptr<ListPositionAllocator> apPositionAlloc = CreateObject<ListPositionAllocator> ();
	apPositionAlloc = CreateObject<ListPositionAllocator> ();
	apPositionAlloc->Add (Vector (0.0, 5.0, 0.0));
	apMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	apMobility.SetPositionAllocator(apPositionAlloc);
	apMobility.Install(wifiApNode);
	BuildingsHelper::Install (wifiApNode);

	NS_LOG_UNCOND ("Randomly allocating wifiStaNode inside the boxArea");
	MobilityHelper stamobility;
	stamobility.SetMobilityModel ("ns3::SteadyStateRandomWaypointMobilityModel");
	Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinX", 		DoubleValue (this->getBoxArea().xMin));
	Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxX", 		DoubleValue (this->getBoxArea().xMax));
	Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinY", 		DoubleValue (this->getBoxArea().yMin));
	Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxY", 		DoubleValue (this->getBoxArea().yMax));
	Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::Z", 			DoubleValue (0.0));
	Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxSpeed", 	DoubleValue (this->getStaSpeed()));
	Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinSpeed", 	DoubleValue (this->getStaSpeed()));
	Ptr<PositionAllocator> ueRandomPositionAlloc = CreateObject<RandomRoomPositionAllocator> ();
	ueRandomPositionAlloc = CreateObject<RandomBoxPositionAllocator> ();
	stamobility.SetPositionAllocator (ueRandomPositionAlloc);
	stamobility.Install (wifiStaNode);
	for (NodeContainer::Iterator it = wifiStaNode.Begin (); it != wifiStaNode.End (); ++it)
	  (*it)->Initialize ();
	BuildingsHelper::Install (wifiStaNode);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing Applications");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

    ApplicationContainer appSource, appSink;
	OnOffHelper onOffHelper (this->getProtocol(), Address (InetSocketAddress (wifiStaInterface.GetAddress(0), this->getWifiPort())));
	appSource = onOffHelper.Install (remoteHost);
	appSource.Start (Seconds (this->getAppStartTime()));
	if(this->getStopApp())
		appSource.Stop (Seconds (this->getSimulationTime()));

	PacketSinkHelper sink (this->getProtocol(),Address (InetSocketAddress (Ipv4Address::GetAny (), this->getWifiPort())));
	appSink = sink.Install (wifiStaNode.Get(0));
	appSink.Start (Seconds (this->getAppStartTime()));
	if(this->getStopApp())
		appSink.Stop (Seconds (this->getSimulationTime()));

	Simulator::Schedule (Seconds (1.0), &Experiment::GetRate, this, wifiStaNode.Get (0));
	Ptr<Socket> recvSink = SetupPacketReceive (wifiStaNode.Get (0));

	m_output.SetTitle("wifi");

///////////////////////////////////////////////
///////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Innitializing LTE <==");
///////////////////////////////////////////////
//////////////////////////////////////////////

	NetDeviceContainer 			internetDevices;
	Ipv4AddressHelper 			ipv4h;
	Ptr<Ipv4StaticRouting> 		remoteHostStaticRouting;
	Ipv4StaticRoutingHelper 	ipv4RoutingHelper;

	NodeContainer enbNodes;
	enbNodes.Create(1);

	Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
	Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
	Ptr<Node> 	pgw;

	lteHelper->SetEpcHelper (epcHelper);
	pgw = epcHelper->GetPgwNode ();

	////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Create  RemoteHost LTE");
	////////////////////////////////////////////////

	NodeContainer remoteHostNodeLte;
	remoteHostNodeLte.Create (1);
	Ptr<Node> remoteHostLTE = remoteHostNodeLte.Get (0);
	internet.Install (remoteHostNodeLte);

	p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (this->getDataRate())));
	p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (this->getP2pLinkDelay())));
	p2ph.SetDeviceAttribute ("Mtu", UintegerValue (this->getMtu()));
	internetDevices = p2ph.Install (pgw, remoteHostLTE);

	ipv4h.SetBase ("10.0.0.0", "255.0.0.0");
	Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
	//Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1); //for upload only

	remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHostLTE->GetObject<Ipv4> ());
	remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

	////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Creating Mobility Models LTE");
	////////////////////////////////////////////////

	NS_LOG_UNCOND ("Installing mobility on remoteHost LTE");
	MobilityHelper rhmobilityLTE;
	Ptr<ListPositionAllocator> rhPositionAllocLTE = CreateObject<ListPositionAllocator> ();
	rhPositionAllocLTE = CreateObject<ListPositionAllocator> ();
	rhPositionAllocLTE->Add (Vector (20.0, -5.0, 0.0));
	rhmobilityLTE.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	rhmobilityLTE.SetPositionAllocator(rhPositionAllocLTE);
	rhmobilityLTE.Install(remoteHostNodeLte);
	BuildingsHelper::Install (remoteHostNodeLte);

	NS_LOG_UNCOND ("Installing mobility on PGW");
	MobilityHelper pgwmobility;
	Ptr<ListPositionAllocator> pgwPositionAlloc = CreateObject<ListPositionAllocator> ();
	pgwPositionAlloc = CreateObject<ListPositionAllocator> ();
	pgwPositionAlloc->Add (Vector (20.0, 0.0, 0.0));
	pgwmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	pgwmobility.SetPositionAllocator(pgwPositionAlloc);
	pgwmobility.Install(pgw);
	BuildingsHelper::Install (pgw);

	NS_LOG_UNCOND ("Installing mobility on PGW");
	MobilityHelper enbMobility;
	Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
	enbPositionAlloc = CreateObject<ListPositionAllocator> ();
	enbPositionAlloc->Add (Vector (20.0, 5.0, 0.0));
	enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	enbMobility.SetPositionAllocator(enbPositionAlloc);
	enbMobility.Install(enbNodes);
	BuildingsHelper::Install (enbNodes);

////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Installing  Devices to the nodes LTE");
////////////////////////////////////////////////

	NetDeviceContainer enbDevs = lteHelper->InstallEnbDevice (enbNodes);
	NetDeviceContainer staDevs = lteHelper->InstallUeDevice (wifiStaNode);

///////////////////////////////////////////
	NS_LOG_UNCOND ("==> Installing the IP stack on the UEs LTE");
//////////////////////////////////////////

	//internet.Install (wifiStaNode);
	Ipv4InterfaceContainer ueIpIface;
	ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (staDevs));

	for (uint32_t u = 0; u < this->getNStations(); ++u)
	{
		Ptr<Node> staNode = wifiStaNode.Get (u);
		Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (staNode->GetObject<Ipv4> ());
		ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
	}

///////////////////////////////////////////
	NS_LOG_UNCOND ("==> Installing  Devices to the nodes LTE");
///////////////////////////////////////////

	lteHelper->AttachToClosestEnb (staDevs, enbDevs);

///////////////////////////////////////////
	NS_LOG_UNCOND ("==> Installing  applications LTE");
//////////////////////////////////////////

    ApplicationContainer appSource2, appSink2;
	OnOffHelper onOffHelper2 (this->getProtocol(), Address (InetSocketAddress (ueIpIface.GetAddress(0), this->getLtePort())));
	appSource2 = onOffHelper2.Install (remoteHostLTE);
	appSource2.Start (Seconds (this->getAppStartTime()));
	if(this->getStopApp())
		appSource2.Stop (Seconds (this->getSimulationTime()));

	PacketSinkHelper sink2 (this->getProtocol(),Address (InetSocketAddress (Ipv4Address::GetAny (), this->getLtePort())));
	appSink2 = sink2.Install (wifiStaNode.Get(0));
	appSink2.Start (Seconds (this->getAppStartTime()));
	if(this->getStopApp())
		appSink2.Stop (Seconds (this->getSimulationTime()));

	if(this->getEnableLteEpsBearer())
	{
		EpsBearer bearer (EpsBearer::GBR_CONV_VIDEO);
		Ptr<EpcTft> tft = Create<EpcTft> ();

		NS_LOG_UNCOND ("==> Activating DL Dedicated EpsBearer " );
		EpcTft::PacketFilter dlpf;
		dlpf.localPortStart = this->getLtePort();
		dlpf.localPortEnd = this->getLtePort();
		tft->Add (dlpf);
		lteHelper->ActivateDedicatedEpsBearer (staDevs.Get(0), bearer, tft);
	}

	Simulator::Schedule (Seconds (1.0), &Experiment::GetRate, this, wifiStaNode.Get (0));
	Ptr<Socket> recvSink2 = SetupPacketReceive (wifiStaNode.Get (0));

	m_output.SetTitle("lte");

/////////////////////////////////////////////////////
	std::cout << std::endl;
	NS_LOG_UNCOND ("//////////////////////////");
	NS_LOG_UNCOND ("==> Starting Simulation <==");
	NS_LOG_UNCOND ("//////////////////////////");
/////////////////////////////////////////////////////

	time_t tempoInicio 	= time(NULL);
	Simulator::Stop (Seconds (this->getSimulationTime()));

	if(this->getTracing())
	{
		phy.EnablePcapAll (this->getOutFileName(), true);
		p2ph.EnablePcapAll (this->getOutFileName(), true);

		lteHelper->EnableTraces ();
		p2ph.EnablePcapAll(this->getOutFileName());
		std::string tr_file_name = "tr_"+this->getOutFileName();
		std::ofstream ascii;
		Ptr<OutputStreamWrapper> ascii_wrap;
		ascii.open (tr_file_name.c_str ());
		ascii_wrap = new OutputStreamWrapper (tr_file_name.c_str (), std::ios::out);
		internet.EnableAsciiIpv4All (ascii_wrap);
	}

	if(this->getNetanim())
	{
		AnimationInterface anim (this->getOutFileName()+"_anim.xml");
	}

	if(this->getShowConfigs())
		showConfigs();

	if(this->getFlowMonitor())
	{
		FlowMonitorHelper flowmon;
		Ptr<FlowMonitor> monitor = flowmon.InstallAll();

		monitor->CheckForLostPackets ();
		monitor->SerializeToXmlFile (this->getOutFileName()+"_flowMonitor.xml", true, true);

		Simulator::Run ();
		flowmonitorOutput(monitor, &flowmon);
	}
	else
		Simulator::Run ();

	if(this->getShowSimulationTime())
	{
		std::cout << "========================= " << "\n";
		std::cout << "Simulation time: " << Simulator::Now().GetSeconds () << " secs\n";
		Simulator::Destroy ();

		time_t tempoFinal 	= time(NULL);
		double tempoTotal 	= difftime(tempoFinal, tempoInicio);
		std::cout << "Real time: " << tempoTotal << " secs  ~ " << (uint32_t)tempoTotal / 60 << " min\n";
		std::cout << "========================= " << "\n";
	}
	else
		Simulator::Destroy ();

	return m_output;
}

int main (int argc, char *argv[])
{
	Experiment experiment = Experiment ();

///////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Setting Up Configs");
///////////////////////////////////////////////

	Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
	Config::SetDefault ("ns3::OnOffApplication::DataRate", DataRateValue(DataRate(experiment.getDataRate())));
	Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (experiment.getPacketSize()));
	Config::SetDefault ("ns3::OnOffApplication::OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
	Config::SetDefault ("ns3::OnOffApplication::OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

	CommandLine cmd;
	cmd.Parse (argc, argv);
	ConfigStore inputConfig;
	inputConfig.ConfigureDefaults ();
	cmd.Parse (argc, argv);

	 if (experiment.getShowPacketSink())
		 LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

///////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Checking Variables");
///////////////////////////////////////////////

	NS_ASSERT (experiment.getNApoints() > 0 && experiment.getNApoints() > 0);
	NS_ASSERT (experiment.getAppStartTime() < experiment.getSimulationTime());

	YansWifiPhyHelper 		wifiPhy 	= YansWifiPhyHelper::Default ();
	YansWifiChannelHelper 	wifiChannel = YansWifiChannelHelper::Default ();
	QosWifiMacHelper 		wifiMac		= QosWifiMacHelper::Default ();
	WifiHelper 				wifi;

	wifi.SetStandard (WIFI_PHY_STANDARD_80211g);
	wifi.SetRemoteStationManager ("ns3::IdealWifiManager");
	wifiMac.SetType ("ns3::AdhocWifiMac");

	Gnuplot gnuplot = Gnuplot (experiment.getGnuplotFileName());
	Gnuplot2dDataset dataset;
	gnuplot.SetTitle ("802.11g");
	gnuplot.SetLegend ("Time (secs)", "Flow Mb/s");

	dataset = experiment.Run (wifi, wifiPhy, wifiMac, wifiChannel);
	gnuplot.AddDataset (dataset);

	std::cout << "==========GNUPLOT======== " << "\n";
	gnuplot.GenerateOutput (std::cout);
	std::cout << std::endl;

	return 0;
}
