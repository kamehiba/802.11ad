#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/gnuplot.h"
#include "ns3/constant-velocity-helper.h"
#include "ns3/integer.h"
#include "ns3/mpi-interface.h"
#include "math.h"
#include <iostream>

/**
 *  PARAMETERS
 */
#define StaNb       1
#define Distance    2
#define Duration    10
#define DataRate    90000000
#define PacketSize  1500

#define couleur(param) printf("\033[%sm",param)

using namespace ns3;

class Experiment {
public:
    Experiment();
    void CreateArchi(void);
    void CreateApplis();

private:

    Ptr<ListPositionAllocator> positionAllocAp;
    Ptr<ListPositionAllocator> positionAllocSta;
    Ptr<GridPositionAllocator> positionAllocStaCouloir;
    Ptr<RandomDiscPositionAllocator> positionAllocStaAmphi;

    std::vector<Ptr<ConstantPositionMobilityModel> > constant;

    NodeContainer m_wifiAP, m_wifiQSta;

    NetDeviceContainer m_APDevice;

    NetDeviceContainer m_QStaDevice;

    YansWifiChannelHelper m_channel;
    Ptr<YansWifiChannel> channel;

    YansWifiPhyHelper m_phyLayer_Sta, m_phyLayer_AP;
    WifiHelper m_wifi;

    QosWifiMacHelper m_macSta, m_macAP;

    InternetStackHelper m_stack;
    Ipv4InterfaceContainer m_StaInterface;
    Ipv4InterfaceContainer m_ApInterface;

    Ssid m_ssid;
};

Experiment::Experiment() {

    positionAllocStaCouloir = CreateObject<GridPositionAllocator>();
    positionAllocAp = CreateObject<ListPositionAllocator>();
    positionAllocSta = CreateObject<ListPositionAllocator>();
    positionAllocStaAmphi = CreateObject<RandomDiscPositionAllocator>();

    m_wifi = WifiHelper::Default();

    constant.resize(StaNb + 1);
    for (int i = 0; i < StaNb + 1; i++)
    {
        constant[i] = CreateObject<ConstantPositionMobilityModel>();
    }
}

void Experiment::CreateArchi(void) {

    m_wifiQSta.Create(StaNb);

    m_wifiAP.Create(1);

    m_ssid = Ssid("BSS_circle");

    m_channel = YansWifiChannelHelper::Default();

    channel = m_channel.Create();

    m_wifi.SetStandard(WIFI_PHY_STANDARD_80211a);
    m_wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue("OfdmRate6Mbps"));

    m_phyLayer_Sta = YansWifiPhyHelper::Default();
    m_phyLayer_AP = YansWifiPhyHelper::Default();

    m_phyLayer_Sta.SetChannel(channel);
    m_phyLayer_AP.SetChannel(channel);

    positionAllocAp->Add(Vector3D(0.0, 0.0, 0.0));

    MobilityHelper mobilityAp;
    mobilityAp.SetPositionAllocator(positionAllocAp);
    mobilityAp.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilityAp.Install(m_wifiAP.Get(0));

    constant[0]->SetPosition(Vector3D(0.0, 0.0, 0.0));

    float deltaAngle = 2 * M_PI / StaNb;
    float angle = 0.0;
    double x = 0.0;
    double y = 0.0;

    for (int i = 0; i < StaNb; i++)
    {
        x = cos(angle) * Distance;
        y = sin(angle) * Distance;

        positionAllocSta->Add(Vector3D(x, y, 0.0));

        MobilityHelper mobilitySta;
        mobilitySta.SetPositionAllocator(positionAllocSta);
        mobilitySta.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobilitySta.Install(m_wifiQSta.Get(i));

        constant[i]->SetPosition(Vector3D(x, y, 0.0));

        angle += deltaAngle;
    }

    m_macSta = QosWifiMacHelper::Default();

    m_macSta.SetType("ns3::StaWifiMac", "ActiveProbing", BooleanValue(true), "Ssid", SsidValue(m_ssid));

    m_macAP = QosWifiMacHelper::Default();

    m_macAP.SetType("ns3::ApWifiMac", "Ssid", SsidValue(m_ssid), "BeaconInterval", TimeValue(Time(std::string("100ms"))));

    m_APDevice.Add(m_wifi.Install(m_phyLayer_AP, m_macAP, m_wifiAP));

    for (int i = 0; i < StaNb; i++)
    {
        m_QStaDevice.Add(m_wifi.Install(m_phyLayer_Sta, m_macSta, m_wifiQSta.Get(i)));
    }

    m_stack.Install(m_wifiAP);
    m_stack.Install(m_wifiQSta);

    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");

    m_ApInterface.Add(address.Assign(m_APDevice.Get(0)));

    for (int i = 0; i < StaNb; i++)
    {
        m_StaInterface.Add(address.Assign(m_QStaDevice.Get(i)));
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
}

void Experiment::CreateApplis() {

    ApplicationContainer source;

    OnOffHelper onoff("ns3::UdpSocketFactory", Address());
    //onoff.SetAttribute("DataRate", StringValue("512kb/s"));

    for (int i = 0; i < StaNb; i++) {
        AddressValue remoteAddress(
                InetSocketAddress(m_StaInterface.GetAddress(i), 5010));
        onoff.SetAttribute("Remote", remoteAddress);
        source.Add(onoff.Install(m_wifiAP.Get(0)));
        source.Start(Seconds(3.0));
        source.Stop(Seconds(Duration));
    }

    ApplicationContainer sinks;
    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory",
            Address(InetSocketAddress(Ipv4Address::GetAny(), 5010)));

    for (int i = 0; i < StaNb; i++) {

        sinks.Add(packetSinkHelper.Install(m_wifiQSta.Get(i)));
        sinks.Start(Seconds(3.0));
        sinks.Stop(Seconds(Duration));
    }

}

int main(int argc, char *argv[]) {

    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("2346"));
    Config::SetDefault ("ns3::OnOffApplication::OnTime", StringValue("ns3::ConstantRandomVariable[Constant=2]"));
    Config::SetDefault ("ns3::OnOffApplication::OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    Experiment exp = Experiment();

    exp.CreateArchi();

    exp.CreateApplis();

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(Duration));

    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(
            flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
    int c = 0;
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
            stats.begin(); i != stats.end(); ++i) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        std::cout << "Flux " << i->first << " (" << t.sourceAddress << " -> "
                << t.destinationAddress << ")\n";
        std::cout << "  Tx Bytes :   " << i->second.txBytes << "\n";
        std::cout << "  Rx Bytes :   " << i->second.rxBytes << "\n";

        couleur("33");
        std::cout << "  Bitrate  :   "
                << i->second.rxBytes * 8.0
                        / (i->second.timeLastRxPacket.GetSeconds()
                                - i->second.timeFirstRxPacket.GetSeconds())
                        / 1024 << " Mbps\n\n";
        couleur("0");

        if (i->second.rxBytes > 0)
            c++;
    }

    std::cout << "  Number of receiving nodes  :   " << c << "\n";

    Simulator::Destroy();
}
