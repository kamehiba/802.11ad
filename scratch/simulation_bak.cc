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

//#include "ns3/lte-module.h"
//#include "ns3/csma-module.h"

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include <ns3/point-to-point-epc-helper.h>
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/core-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include <ns3/buildings-module.h>
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/config-store-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor.h"
#include "ns3/netanim-module.h"
#include "ns3/cone-antenna.h"
#include "ns3/measured-2d-antenna.h"
#include <ns3/femtocellBlockAllocator.h>
#include "ns3/stats-module.h"
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("debug");

#define color(param) printf("\033[%sm",param)

//======================================================================================
void showConfigs(uint32_t,uint32_t,uint32_t,double,bool,uint32_t,std::string,uint32_t,Box,double);
void flowmonitorOutput(Ptr<FlowMonitor>,FlowMonitorHelper*,Gnuplot2dDataset);
void startAppWifi(NodeContainer,NodeContainer,Ipv4InterfaceContainer,double);
void startAppLTE(NodeContainer,NodeContainer,Ipv4InterfaceContainer,Ptr<LteHelper>,NetDeviceContainer,double);
void calcDistance(NodeContainer,NodeContainer,NodeContainer);
//======================================================================================

double simulationTime				= 40.0;
double appStartTime					= 0.01;

//Others Vars
double staDistanceAp				= 0.0;
double staDistanceEnb				= 0.0;
bool flowmonitor					= true;
bool enableLteEpsBearer				= true;
bool verbose						= false;	// packetSink dataFlow
bool showConf						= true;
bool netAnim						= false;
bool tracing						= false;

//P2P Vars
uint32_t mtu						= 1500; 	// p2p Mtu
uint32_t p2pLinkDelay				= 0.010;		// p2p link Delay

//Area Vars
double BoxXmin						= 0;
double BoxXmax						= 25;
double BoxYmin						= 0;
double BoxYmax						= 25;

//Femtocells Vars
bool useFemtocells					= true;
uint32_t nFemtocells				= 1;
uint32_t nFloors					= 1;
uint32_t nApartmentsX				= 1;

//IPs
Ipv4Address ipRemoteHostWifi		= "1.0.0.0";
Ipv4Address ipRouter				= "2.0.0.0";
Ipv4Address ipWifi					= "3.0.0.0";
Ipv4Address ipRemoteHostLte			= "4.0.0.0";
Ipv4Mask	netMask					= "255.0.0.0";

//WIFI Vars
bool use2DAntenna					= true;
uint32_t wifiChannelNumber			= 1;
uint32_t nTxAntennas 				= 1;
uint32_t nRxAntennas				= 1;
uint32_t maxAmsduSize				= 999999;//262143;

//APP Vars
uint32_t packetSize					= 1472;
uint32_t appPort					= 9;
std::string protocol 				= "ns3::UdpSocketFactory";
std::string dataRate 				= "1Mb/s";

//Nodes Vars
double wifiVerticalPosition			= 0.0;
double lteVerticalPosition			= 20.0;
double staSpeed						= 2.0; 	// m/s.

uint32_t nEnb	 					= 1; 	// Enb
uint32_t nApoints 					= 3; 	// Access Points
uint32_t nStations 					= 1;	// Stations

std::string outFileName				= "debug";

NodeContainer remoteHostContainerWifi, routerContainerWifi, wifiApNode, wifiStaNode, enbNodes;

int main (int argc, char *argv[])
{
	Ipv4AddressHelper 			ipv4;
	InternetStackHelper 		internet;
	Ipv4StaticRoutingHelper 	ipv4RoutingHelper;
	NetDeviceContainer 			remoteHostDevice, routerDevice, wifiAPDevice, wifiStaDevice, lteDevice;
	Ipv4InterfaceContainer 		wifiStaInterface;
	Ptr<Ipv4StaticRouting> 		remoteHostStaticRouting;
	Box 						boxArea;

///////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Setting Up Configs");
///////////////////////////////////////////////

	/* Global params: no fragmentation, no RTS/CTS, fixed rate for all packets */
	Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("999999"));
	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));

	Config::SetDefault ("ns3::OnOffApplication::DataRate", 		DataRateValue(DataRate(dataRate)));
    Config::SetDefault ("ns3::OnOffApplication::PacketSize", 	UintegerValue (packetSize));
    Config::SetDefault ("ns3::OnOffApplication::OnTime", 		StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    Config::SetDefault ("ns3::OnOffApplication::OffTime", 		StringValue("ns3::ConstantRandomVariable[Constant=0]"));

	Config::SetDefault ("ns3::StaWifiMac::ProbeRequestTimeout", StringValue ("0.01"));
	Config::SetDefault ("ns3::StaWifiMac::AssocRequestTimeout", StringValue ("0.01"));
	Config::SetDefault ("ns3::StaWifiMac::MaxMissedBeacons", 	UintegerValue (1));

///////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Setting Up Command Line Parameters");
///////////////////////////////////////////////

	 if (verbose)
	 {
		 LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
	 }

	CommandLine cmd;
	cmd.AddValue("simulationTime", "Simulation Time: ", simulationTime);
	cmd.AddValue("nApoints", "How many ENBs: ", nApoints);
	cmd.AddValue("nStations", "How many UEs: ", nStations);
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

	NS_ASSERT (nApoints > 0 && nStations > 0);
	NS_ASSERT (nFemtocells > 0);
	NS_ASSERT (appStartTime < simulationTime);

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

	remoteHostContainerWifi.Create (nApoints);
	routerContainerWifi.Create (nApoints);
	wifiApNode.Create(nApoints);
	wifiStaNode.Create(nStations);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Installing p2p");
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	PointToPointHelper 	p2ph;
	p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
	p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (p2pLinkDelay)));
	p2ph.SetDeviceAttribute ("Mtu", UintegerValue (mtu));
	p2ph.SetQueue("ns3::DropTailQueue","MaxPackets",StringValue("99999999"));

	for (uint32_t i = 0; i < nApoints; ++i)
	{
		remoteHostDevice.Add (p2ph.Install (remoteHostContainerWifi.Get(i), routerContainerWifi.Get (i)));
		routerDevice.Add (p2ph.Install (routerContainerWifi.Get(i), wifiApNode.Get (i)));
	}

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing Wifi");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	YansWifiChannelHelper 	wifiChannel = YansWifiChannelHelper::Default ();
	YansWifiPhyHelper 	 	wifiPhy		= YansWifiPhyHelper::Default ();
	QosWifiMacHelper 		wifiMac 	= QosWifiMacHelper::Default ();
	WifiHelper 				wifi;

	Ssid ssid = Ssid ("ns3-80211ad");

	wifi.SetStandard (WIFI_PHY_STANDARD_80211ad_OFDM);
	wifi.SetRemoteStationManager ("ns3::IdealWifiManager");
	//wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue("OfdmRate7Gbps"));
	//wifi.SetRemoteStationManager ("ns3::IdealWifiManager", "BerThreshold",DoubleValue(1e-6));
	//wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",StringValue ("OfdmRate7Gbps"), "ControlMode",StringValue ("OfdmRate2Gbps"));

	wifiMac.SetType ("ns3::FlywaysWifiMac");

	////video trasmission
	wifiMac.SetBlockAckThresholdForAc(AC_VI, 2);
	wifiMac.SetMsduAggregatorForAc (AC_VI, "ns3::MsduStandardAggregator", "MaxAmsduSize", UintegerValue (maxAmsduSize));

	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (60e3));

	wifiPhy.SetChannel (wifiChannel.Create ());
	wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);
	wifiPhy.SetErrorRateModel ("ns3::SensitivityModel60GHz");
	wifiPhy.Set ("ChannelNumber", UintegerValue(wifiChannelNumber));
	wifiPhy.Set ("TxAntennas", UintegerValue (nTxAntennas));
	wifiPhy.Set ("RxAntennas", UintegerValue (nRxAntennas));
//	wifiPhy.Set ("TxGain", DoubleValue (0));
//	wifiPhy.Set ("RxGain", DoubleValue (0));
	wifiPhy.Set ("TxPowerStart", DoubleValue(10.0));
	wifiPhy.Set ("TxPowerEnd", DoubleValue(10.0));
	wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
	wifiPhy.Set ("RxNoiseFigure", DoubleValue(0));
	wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-79) );
	wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-79+3) );
	wifiPhy.Set ("ShortGuardEnabled", BooleanValue (false));// Set guard interval

	wifiMac.SetType ("ns3::ApWifiMac","Ssid",SsidValue (ssid),"BeaconGeneration", BooleanValue (true),"BeaconInterval", TimeValue (Seconds (2.5)));

	for (uint32_t i = 0; i < nApoints; ++i)
		wifiAPDevice.Add (wifi.Install (wifiPhy, wifiMac, wifiApNode.Get(i)));

	wifiMac.SetType ("ns3::StaWifiMac","Ssid", SsidValue (ssid),"ActiveProbing", BooleanValue (true));

	for (uint32_t i = 0; i < nStations; ++i)
		wifiStaDevice.Add (wifi.Install (wifiPhy, wifiMac, wifiStaNode.Get(i)));

	if(use2DAntenna)
	{
		NS_LOG_UNCOND ("==> Initializing 2D Antenna");

		Ptr<Measured2DAntenna> m2DAntenna = CreateObject<Measured2DAntenna>();
		m2DAntenna->SetMode(10);

		for (uint32_t u = 0; u < nApoints; ++u)
			wifiAPDevice.Get(u)->GetObject<WifiNetDevice>()->GetPhy()->AggregateObject(m2DAntenna);

		for (uint32_t u = 0; u < nStations; ++u)
			wifiStaDevice.Get(u)->GetObject<WifiNetDevice>()->GetPhy()->AggregateObject(m2DAntenna);
	}

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Assigning IP to WIFI Devices");
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	internet.Install(remoteHostContainerWifi);
	internet.Install(routerContainerWifi);
	internet.Install(wifiApNode);
	internet.Install(wifiStaNode);

	ipv4.SetBase (ipRemoteHostWifi, netMask);
	ipv4.Assign (remoteHostDevice);

	ipv4.SetBase (ipRouter, netMask);
	ipv4.Assign (routerDevice);

	ipv4.SetBase (ipWifi, netMask);
	ipv4.Assign (wifiAPDevice);

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
	double rhX=wifiVerticalPosition;
	for(uint32_t i=0; i<wifiApNode.GetN();i++)
	{
		rhPositionAlloc->Add (Vector (rhX, -40.0, 0.0));
		rhX+=-20;
	}
	rhmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	rhmobility.SetPositionAllocator(rhPositionAlloc);
	rhmobility.Install(remoteHostContainerWifi);
	BuildingsHelper::Install (remoteHostContainerWifi);

	NS_LOG_UNCOND ("Installing mobility on Router");
	MobilityHelper routermobility;
	Ptr<ListPositionAllocator> routerPositionAlloc = CreateObject<ListPositionAllocator> ();
	routerPositionAlloc = CreateObject<ListPositionAllocator> ();
	double roX=wifiVerticalPosition;
	for(uint32_t i=0; i<wifiApNode.GetN();i++)
	{
		routerPositionAlloc->Add (Vector (roX, -20.0, 0.0));
		roX+=-20;
	}
	routermobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	routermobility.SetPositionAllocator(routerPositionAlloc);
	routermobility.Install(routerContainerWifi);
	BuildingsHelper::Install (routerContainerWifi);

	NS_LOG_UNCOND ("Installing mobility on wifiApNode");
	MobilityHelper apMobility;
	Ptr<ListPositionAllocator> apPositionAlloc = CreateObject<ListPositionAllocator> ();
	apPositionAlloc = CreateObject<ListPositionAllocator> ();
	double apX=wifiVerticalPosition;
	for(uint32_t i=0; i<wifiApNode.GetN();i++)
	{
		apPositionAlloc->Add (Vector (apX, 5.0, 0.0));
		apX+=-20;
	}
	apMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	apMobility.SetPositionAllocator(apPositionAlloc);
	apMobility.Install(wifiApNode);
	BuildingsHelper::Install (wifiApNode);

    NS_LOG_UNCOND ("Installing mobility on wifiStaNode");
    MobilityHelper stamobility;
    stamobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
    stamobility.Install (wifiStaNode);
    double staX = -10.0;
    for (uint32_t j = 0; j < nApoints; j++)
    {
    	for (uint32_t i = 0; i < nStations; i++)
    	{
    		wifiStaNode.Get (i)->GetObject<MobilityModel> ()->SetPosition (Vector (staX, 10.0, 0.0));
    		wifiStaNode.Get (i)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (staSpeed, 0, 0));
    	}
		staX+=-20.0;
    }
    BuildingsHelper::Install (wifiStaNode);

///////////////////////////////////////////////
///////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Innitializing LTE <==");
///////////////////////////////////////////////
//////////////////////////////////////////////

	enbNodes.Create(nEnb);

	Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
	Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
	Ptr<Node> 	pgw;

	//lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::BuildingsObstaclePropagationLossModel"));
	lteHelper->SetEpcHelper (epcHelper);
	pgw = epcHelper->GetPgwNode ();

	////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Create  RemoteHost LTE");
	////////////////////////////////////////////////

	NodeContainer remoteHostContainerLte;
	remoteHostContainerLte.Create (1);
	Ptr<Node> remoteHostLTE = remoteHostContainerLte.Get (0);
	internet.Install (remoteHostContainerLte);

	p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
	p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (p2pLinkDelay)));
	p2ph.SetDeviceAttribute ("Mtu", UintegerValue (mtu));
	lteDevice = p2ph.Install (pgw, remoteHostLTE);

	ipv4.SetBase (ipRemoteHostLte, netMask);
	Ipv4InterfaceContainer internetIpIfaces = ipv4.Assign (lteDevice);
	//Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1); //for upload only

	remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHostLTE->GetObject<Ipv4> ());
	remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask (netMask), 1);

	////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Creating Mobility Models LTE");
	////////////////////////////////////////////////

	NS_LOG_UNCOND ("Installing mobility on remoteHost LTE");
	MobilityHelper rhmobilityLTE;
	Ptr<ListPositionAllocator> rhPositionAllocLTE = CreateObject<ListPositionAllocator> ();
	rhPositionAllocLTE = CreateObject<ListPositionAllocator> ();
	rhPositionAllocLTE->Add (Vector (lteVerticalPosition, -40.0, 0.0));
	rhmobilityLTE.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	rhmobilityLTE.SetPositionAllocator(rhPositionAllocLTE);
	rhmobilityLTE.Install(remoteHostContainerLte);
	BuildingsHelper::Install (remoteHostContainerLte);

	NS_LOG_UNCOND ("Installing mobility on PGW");
	MobilityHelper pgwmobility;
	Ptr<ListPositionAllocator> pgwPositionAlloc = CreateObject<ListPositionAllocator> ();
	pgwPositionAlloc = CreateObject<ListPositionAllocator> ();
	pgwPositionAlloc->Add (Vector (lteVerticalPosition, -20.0, 0.0));
	pgwmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	pgwmobility.SetPositionAllocator(pgwPositionAlloc);
	pgwmobility.Install(pgw);
	BuildingsHelper::Install (pgw);

	NS_LOG_UNCOND ("Installing mobility on enbNodes");
	MobilityHelper enbMobility;
	Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
	enbPositionAlloc = CreateObject<ListPositionAllocator> ();
	double enbX=lteVerticalPosition;
	for(uint32_t i=0;i<enbNodes.GetN();i++)
	{
		enbPositionAlloc->Add (Vector (enbX, -10.0, 0.0));
		enbX+=20.0;
	}
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

	for (uint32_t u = 0; u < nStations; ++u)
	{
		Ptr<Node> staNode = wifiStaNode.Get (u);
		Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (staNode->GetObject<Ipv4> ());
		ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
	}

///////////////////////////////////////////
	NS_LOG_UNCOND ("==> Installing  Devices to the nodes LTE");
///////////////////////////////////////////

	lteHelper->AttachToClosestEnb (staDevs, enbDevs);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing Applications");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	double startLteApp=0.0;

	if(nApoints==1)
		startLteApp=8.0;
	else if(nApoints==2)
		startLteApp=16.0;
	else if(nApoints==3)
		startLteApp=31.0;
	else
		NS_ASSERT (simulationTime > startLteApp);

	//to use LTE simulation time has to be higher than startLteApp
	NS_ASSERT (simulationTime > startLteApp);

	lteHelper->AddX2Interface (enbNodes);

	startAppWifi(wifiStaNode,remoteHostContainerWifi, wifiStaInterface, startLteApp);
	startAppLTE(wifiStaNode, remoteHostContainerLte, ueIpIface, lteHelper, staDevs, startLteApp);

	for(uint32_t i=0; i < nEnb; i++)
	{
	  if(i+1 != nEnb)
	  {
		  for(uint32_t j=0; j < nStations; j++)
			  lteHelper->HandoverRequest (Seconds (startLteApp), staDevs.Get (j), enbDevs.Get (i), enbDevs.Get (i+1));
	  }

	  startLteApp+=5.0;
	}

/////////////////////////////////////////////////////
	std::cout << std::endl;
	NS_LOG_UNCOND ("//////////////////////////");
	NS_LOG_UNCOND ("==> Starting Simulation <==");
	NS_LOG_UNCOND ("//////////////////////////");
/////////////////////////////////////////////////////

	time_t tempoInicio 	= time(NULL);
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

	std::string fileNameWithNoExtension = "gnuplot";
	std::string graphicsFileName        = fileNameWithNoExtension + ".png";
	std::string plotFileName            = fileNameWithNoExtension + ".plt";
	std::string plotTitle               = "Flow vs Throughput";
	std::string dataTitle               = "Throughput (Mbps)";

	Gnuplot gnuplot (graphicsFileName);
	gnuplot.SetTitle (plotTitle);

	gnuplot.SetTerminal ("png");
	gnuplot.SetLegend ("Flow (secs)", "Throughput (Mbps)");

	Gnuplot2dDataset dataset;
	dataset.SetTitle (dataTitle);
	dataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);

	FlowMonitorHelper fmHelper;
	Ptr<FlowMonitor> allMon = fmHelper.InstallAll();
	allMon->CheckForLostPackets ();
	allMon->SerializeToXmlFile (outFileName+"_flowMonitor.xml", true, true);
	flowmonitorOutput(allMon, &fmHelper, dataset);

	Simulator::Run ();

	gnuplot.AddDataset (dataset);
	std::ofstream plotFile (plotFileName.c_str());
	gnuplot.GenerateOutput (plotFile);
	plotFile.close ();

	std::cout << std::endl;
	std::cout << "==========TIMERS========= " << "\n";
	std::cout << "Simulation time: " << Simulator::Now().GetSeconds () << " secs\n";
	std::cout << "GNU Plot time: " << simulationTime << " secs\n";

	Simulator::Destroy ();

	time_t tempoFinal 	= time(NULL);
	double tempoTotal 	= difftime(tempoFinal, tempoInicio);
	std::cout << "Real time: " << tempoTotal << " secs ~ " << (uint32_t)tempoTotal / 60 << " min\n";

	if(showConf)
		showConfigs(nEnb, nApoints, nStations, staSpeed, useFemtocells, nFemtocells, dataRate, packetSize, boxArea, simulationTime);

	return 0;
}

void flowmonitorOutput(Ptr<FlowMonitor> flowMon, FlowMonitorHelper *fmhelper, Gnuplot2dDataset dataSet)
{
	double x=0.0, y=0.0;
	double difftx=0.0, diffrx=0.0, diffrxtx=0.0, txbitrate_value=0.0, txOffered=0.0, rxbitrate_value=0.0, delay_value=0.0, throughput=0.0;
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier ());
	FlowMonitor::FlowStatsContainer stats = flowMon->GetFlowStats ();

	if((double) Simulator::Now().GetSeconds() < simulationTime)
	{
		for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
		{
			calcDistance(wifiApNode,wifiStaNode,enbNodes);

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

			if(flowmonitor)
			{
				if (i->first < 2 && t.sourceAddress == "1.0.0.1" && staDistanceAp <= 12)
				{
					color("31");
					std::cout << "===========WIFI================= " << "\n";
					std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ") " << (double) Simulator::Now().GetSeconds() << " secs\n";
					color("0");
					std::cout << "  Tx Packets: " 		<< i->second.txPackets 				<< "\n";
					std::cout << "  Tx Bytes:   " 		<< i->second.txBytes 				<< "\n";
					std::cout << "  Tx bitrate: " 		<< txbitrate_value 					<< " Mbps\n";
					std::cout << "  TxOffered:  " 		<< txOffered 						<< " Mbps\n\n";

					std::cout << "  Rx Packets: " 		<< i->second.rxPackets 				<< "\n";
					std::cout << "  Rx Bytes:   " 		<< i->second.rxBytes 				<< "\n";
					std::cout << "  Rx bitrate: " 		<< rxbitrate_value 					<< " Mbps\n\n";

					std::cout << "  Lost Packets: " 	<< i->second.lostPackets 			<< "\n";
					std::cout << "  Dropped Packets: " 	<< i->second.packetsDropped.size() 	<< "\n";
					std::cout << "  JitterSum: " 		<< i->second.jitterSum 				<< "\n";
					std::cout << "  Throughput: " 		<< throughput 						<< " Mbps\n\n";

					std::cout << "  Distance from AP: " << staDistanceAp					<< "\n";
					std::cout << "  Distance from Enb: "<< staDistanceEnb 					<< "\n";
					std::cout << "  Average delay: " 	<< delay_value 						<< "s\n";
				}
				else
				{
					color("31");
					std::cout << "===========LTE================= " << "\n";
					std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ") " << (double) Simulator::Now().GetSeconds() << " secs\n";
					color("0");
					std::cout << "  Tx Packets: " 		<< i->second.txPackets 				<< "\n";
					std::cout << "  Tx Bytes:   " 		<< i->second.txBytes 				<< "\n";
					std::cout << "  Tx bitrate: " 		<< txbitrate_value 					<< " Mbps\n";
					std::cout << "  TxOffered:  " 		<< txOffered 						<< " Mbps\n\n";

					std::cout << "  Rx Packets: " 		<< i->second.rxPackets 				<< "\n";
					std::cout << "  Rx Bytes:   " 		<< i->second.rxBytes 				<< "\n";
					std::cout << "  Rx bitrate: " 		<< rxbitrate_value 					<< " Mbps\n\n";

					std::cout << "  Lost Packets: " 	<< i->second.lostPackets 			<< "\n";
					std::cout << "  Dropped Packets: " 	<< i->second.packetsDropped.size() 	<< "\n";
					std::cout << "  JitterSum: " 		<< i->second.jitterSum 				<< "\n";
					std::cout << "  Throughput: " 		<< throughput 						<< " Mbps\n\n";

					std::cout << "  Distance from AP: " << staDistanceAp					<< "\n";
					std::cout << "  Distance from Enb: "<< staDistanceEnb 					<< "\n";
					std::cout << "  Average delay: " 	<< delay_value 						<< "s\n";
				}
			}

			x = (double) Simulator::Now().GetSeconds();

			if(t.sourceAddress == "1.0.0.1" && staDistanceAp <= 13)
				y = (double) txOffered;
			else
				y = throughput;

			dataSet.Add(x,y);
		}
	}

	Simulator::Schedule(Seconds(1), &flowmonitorOutput, flowMon, fmhelper, dataSet);
}

void startAppWifi(NodeContainer wifiStaNode, NodeContainer remoteHostContainerWifi, Ipv4InterfaceContainer wifiStaInterface, double startLteApp)
{
	ApplicationContainer appSourceWifi, appSinkWifi;

	for (uint32_t i = 0; i < nStations; i++)
	{
		NS_LOG_UNCOND ("Initializing apps WIFI on node " << i);
		OnOffHelper onOffHelperWifi (protocol, Address (InetSocketAddress (wifiStaInterface.GetAddress(i), appPort)));
		appSourceWifi = onOffHelperWifi.Install (remoteHostContainerWifi.Get (i));
		PacketSinkHelper sink (protocol,Address (InetSocketAddress (Ipv4Address::GetAny (), appPort)));
		appSinkWifi = sink.Install (wifiStaNode.Get(i));

		//Ipv4GlobalRoutingHelper::RecomputeRoutingTables();
	}

	appSourceWifi.Start (Seconds (appStartTime));
	appSinkWifi.Start (Seconds (appStartTime));

	appSourceWifi.Stop (Seconds (startLteApp));
	appSinkWifi.Stop (Seconds (startLteApp));
}

void startAppLTE(NodeContainer wifiStaNode, NodeContainer remoteHostContainerLte, Ipv4InterfaceContainer ueIpIface,
				Ptr<LteHelper> lteHelper, NetDeviceContainer staDevs, double startLteApp)
{
	ApplicationContainer appSourceLTE, appSinkLTE;

	for (uint32_t i = 0; i < nStations; i++)
	{
		NS_LOG_UNCOND ("Initializing apps LTE on node " << i);
		OnOffHelper onOffHelperLTE (protocol, Address (InetSocketAddress (ueIpIface.GetAddress(i), appPort)));
		appSourceLTE = onOffHelperLTE.Install (remoteHostContainerLte.Get(i));
		PacketSinkHelper sinkLTE (protocol, Address (InetSocketAddress (Ipv4Address::GetAny (), appPort)));
		appSinkLTE = sinkLTE.Install (wifiStaNode.Get(i));

		if(enableLteEpsBearer)
		{
			EpsBearer bearer (EpsBearer::GBR_CONV_VIDEO);
			Ptr<EpcTft> tft = Create<EpcTft> ();
			NS_LOG_UNCOND ("==> Activating DL Dedicated EpsBearer " );
			EpcTft::PacketFilter dlpf;
			dlpf.localPortStart = appPort;
			dlpf.localPortEnd = appPort;
			tft->Add (dlpf);
			lteHelper->ActivateDedicatedEpsBearer (staDevs.Get(i), bearer, tft);
		}

		//Ipv4GlobalRoutingHelper::RecomputeRoutingTables();
	}

	appSourceLTE.Start(Seconds (startLteApp));
	appSinkLTE.Start(Seconds (startLteApp));

	appSourceLTE.Stop(Seconds (simulationTime));
	appSinkLTE.Stop(Seconds (simulationTime));
}

void calcDistance(NodeContainer wifiApNode, NodeContainer wifiStaNode, NodeContainer enbNodes)
{
	Ptr<MobilityModel> enbNode = enbNodes.Get(0)->GetObject<MobilityModel>();
	Ptr<MobilityModel> apNode = wifiApNode.Get(0)->GetObject<MobilityModel>();
	Ptr<MobilityModel> staNode = wifiStaNode.Get(0)->GetObject<MobilityModel>();

	staDistanceEnb = staNode->GetDistanceFrom (enbNode);
	staDistanceAp = staNode->GetDistanceFrom (apNode);
}

void showConfigs(uint32_t nEnb,uint32_t nApoints, uint32_t nStations, double staSpeed, bool useFemtocells, uint32_t nFemtocells,
				std::string dataRate, uint32_t packetSize, Box boxArea, double simulationTime)
{
	std::cout << "==========CONFIGS======== " << "\n";
	std::cout << "Access points: " << nApoints << "\n";
	std::cout << "Stations: " << nStations << "\n";
	std::cout << "Enb: " << nEnb << "\n";
	std::cout << "Station Speed: " << staSpeed << "m/s" << " <> " << staSpeed*3.6 << "km/h\n";
	useFemtocells == true ? std::cout << "Femtocells: " << nFemtocells << "\n" : std::cout << "Femtocells Disabled\n";
	std::cout << "DataRate: " << dataRate << "\n";
	std::cout << "Area: " << (boxArea.xMax - boxArea.xMin) * (boxArea.yMax - boxArea.yMin) << "m²\n";
	std::cout << "========================= " << "\n";
}