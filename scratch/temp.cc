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

   APs
   *
   -
   -
   - Sta

*/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/stats-module.h"
#include "ns3/wifi-module.h"
#include <iostream>

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

class Experiment
{
public:

	Experiment ();
	Experiment (std::string title);
	void flowmonitorOutput(Ptr<FlowMonitor> monitor, FlowMonitorHelper *flowmon);
	Gnuplot2dDataset Run (const WifiHelper &wifi, const YansWifiPhyHelper &wifiPhy,
						  const QosWifiMacHelper &wifiMac, const YansWifiChannelHelper &wifiChannel);

	void setSimulationTime(double var) {m_simulationTime = var;};
	double getSimulationTime() { return m_simulationTime;};

	void setAppStartTime(double var) {m_appStartTime = var;};
	double getAppStartTime() { return m_appStartTime;};

	void setFlowmonitor(bool var) {m_flowmonitor = var;};
	bool getFlowMonitor() { return m_flowmonitor;};

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

	void setDataRate(std::string var) {m_dataRate = var;};
	std::string getDataRate() { return m_dataRate;};

	void setOutFileName(std::string var) {m_outFileName = var;};
	std::string getOutFileName() { return m_outFileName;};

private:

	void ReceivePacket (Ptr<Socket> socket);
	void GetRate (Ptr<Node> node);
	Ptr<Socket> SetupPacketReceive (Ptr<Node> node);

	double m_simulationTime;
	double m_appStartTime;

	bool m_flowmonitor;

	uint32_t m_wifiChannelNumber;
	uint32_t m_nTxAntennas;
	uint32_t m_nRxAntennas;
	uint32_t m_maxAmsduSize;
	uint32_t m_bytesTotal;
	uint32_t m_nApoints;
	uint32_t m_nStations;
	uint32_t m_packetSize;

	std::string m_dataRate;
	std::string m_outFileName;

	Gnuplot2dDataset m_output;
};

Experiment::Experiment ()
{
}

Experiment::Experiment (std::string title)
  : m_output (title)
{
	m_simulationTime 	= 10;
	m_appStartTime 		= 0.01;

	m_flowmonitor 		= false;

	m_wifiChannelNumber = 1;
	m_nTxAntennas 		= 1;
	m_nRxAntennas 		= 1;
	m_maxAmsduSize 		= 262143;
	m_bytesTotal 		= 0;
	m_nApoints 			= 1;
	m_nStations 		= 1;
	m_packetSize 		= 1472;

	m_dataRate 			= "512Kb/s";
	m_outFileName 		= "debug";

	m_output.SetStyle (Gnuplot2dDataset::LINES);
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
	NodeContainer wifiApNode;
	wifiApNode.Create(this->getNApoints());

	NodeContainer wifiStaNode;
	wifiStaNode.Create(this->getNStations());

	PacketSocketHelper packetSocket;
	packetSocket.Install (wifiApNode);
	packetSocket.Install (wifiStaNode);

	YansWifiChannelHelper chan = wifiChannel;
	chan.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

	YansWifiPhyHelper phy = wifiPhy;
	phy.SetChannel (wifiChannel.Create ());
	phy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);
	phy.SetErrorRateModel ("ns3::SensitivityModel60GHz");
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


	Ssid ssid = Ssid ("ns3-80211ad");

	mac.SetType ("ns3::ApWifiMac","Ssid",SsidValue (ssid),"BeaconGeneration", BooleanValue (true),"BeaconInterval", TimeValue (Seconds (2.5)));
	NetDeviceContainer wifiAPDevice = wifi.Install (phy, mac, wifiApNode);

	mac.SetType ("ns3::StaWifiMac","Ssid", SsidValue (ssid),"ActiveProbing", BooleanValue (true));
	NetDeviceContainer wifiStaDevice = wifi.Install (phy, mac, wifiStaNode);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing WIFI Mobility");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	NS_LOG_UNCOND ("Installing mobility on wifiApNode");
	MobilityHelper rhmobility;
	Ptr<ListPositionAllocator> rhPositionAlloc = CreateObject<ListPositionAllocator> ();
	rhPositionAlloc = CreateObject<ListPositionAllocator> ();
	rhPositionAlloc->Add (Vector (0.0, -5.0, 0.0));
	rhmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	rhmobility.SetPositionAllocator(rhPositionAlloc);
	rhmobility.Install(wifiApNode);

	NS_LOG_UNCOND ("Installing mobility on wifiStaNode");
	MobilityHelper routermobility;
	Ptr<ListPositionAllocator> routerPositionAlloc = CreateObject<ListPositionAllocator> ();
	routerPositionAlloc = CreateObject<ListPositionAllocator> ();
	routerPositionAlloc->Add (Vector (0.0, 0.0, 0.0));
	routermobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	routermobility.SetPositionAllocator(routerPositionAlloc);
	routermobility.Install(wifiStaNode);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
	NS_LOG_UNCOND ("==> Initializing Applications");//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

	NS_LOG_UNCOND ("Installing PacketSocketAddress");
	PacketSocketAddress socket;
	socket.SetSingleDevice (wifiAPDevice.Get (0)->GetIfIndex ());
	socket.SetPhysicalAddress (wifiStaDevice.Get (0)->GetAddress ());
	socket.SetProtocol (1);

	NS_LOG_UNCOND ("Installing OnOffHelper");
	OnOffHelper onoff ("ns3::PacketSocketFactory", Address (socket));

	NS_LOG_UNCOND ("Installing ApplicationContainer");
	ApplicationContainer apps = onoff.Install (wifiApNode.Get (0));
	apps.Start (Seconds (this->getAppStartTime()));
	apps.Stop (Seconds (this->getSimulationTime()));

	Simulator::Schedule (Seconds (1.0), &Experiment::GetRate, this, wifiStaNode.Get (0));
	Ptr<Socket> recvSink = SetupPacketReceive (wifiStaNode.Get (0));

/////////////////////////////////////////////////////
	std::cout << std::endl;
	NS_LOG_UNCOND ("//////////////////////////");
	NS_LOG_UNCOND ("==> Starting Simulation <==");
	NS_LOG_UNCOND ("//////////////////////////");
/////////////////////////////////////////////////////

	Simulator::Stop (Seconds (this->getSimulationTime()));

	if(this->getFlowMonitor())
	{
		FlowMonitorHelper flowmon;
		Ptr<FlowMonitor> monitor = flowmon.InstallAll();

		monitor->CheckForLostPackets ();
		monitor->SerializeToXmlFile (this->getOutFileName()+"_flowMonitor.xml", true, true);

		Simulator::Run ();

		//showConfigs(m_nApoints, m_nStations, staSpeed, useFemtocells, nFemtocells, m_dataRate, m_packetSize, boxArea, m_simulationTime);
		flowmonitorOutput(monitor, &flowmon);
	}
	else
	{
		Simulator::Run ();
		//showConfigs(m_nApoints, m_nStations, staSpeed, useFemtocells, nFemtocells, m_dataRate, m_packetSize, boxArea, m_simulationTime);
	}

	Simulator::Destroy ();

	return m_output;
}

int main (int argc, char *argv[])
{
	Experiment experiment = Experiment ("WIFI_PHY_STANDARD_80211ad_OFDM");

//	experiment.setSimulationTime(10);
//	experiment.setFlowmonitor(true);
//	experiment.setBytesTotal(0);
//	experiment.setWifiChannelNumber(1);
//	experiment.setNTxAntennas(1);
//	experiment.setNRxAntennas(1);
//	experiment.setMaxAmsduSize(262143);
//	experiment.setAppStartTime(0.01);
//	experiment.setNApoints(1);
//	experiment.setNStations(1);
//	experiment.setDataRate("512Kb/s");
//	experiment.setPacketSize(1472);
//	experiment.setOutFileName("debug");


	Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));

	Config::SetDefault ("ns3::OnOffApplication::DataRate", DataRateValue(DataRate(experiment.getDataRate())));
	Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (experiment.getPacketSize()));
	Config::SetDefault ("ns3::OnOffApplication::OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
	Config::SetDefault ("ns3::OnOffApplication::OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

	CommandLine cmd;
	cmd.Parse (argc, argv);

	Gnuplot gnuplot = Gnuplot ("rates.png");
	Gnuplot2dDataset dataset;

	YansWifiPhyHelper 		wifiPhy 	= YansWifiPhyHelper::Default ();
	YansWifiChannelHelper 	wifiChannel = YansWifiChannelHelper::Default ();
	QosWifiMacHelper 		wifiMac		= QosWifiMacHelper::Default ();
	WifiHelper 				wifi;

	wifi.SetStandard (WIFI_PHY_STANDARD_80211ad_OFDM);
	wifi.SetRemoteStationManager ("ns3::IdealWifiManager");
	//wifiMac.SetType ("ns3::FlywaysWifiMac");
	wifiMac.SetType ("ns3::AdhocWifiMac");
	dataset = experiment.Run (wifi, wifiPhy, wifiMac, wifiChannel);
	gnuplot.AddDataset (dataset);

	std::cout << std::endl;
	std::cout << "==========GNUPLOT======== " << "\n";
	gnuplot.GenerateOutput (std::cout);
	std::cout << std::endl;

	return 0;
}
