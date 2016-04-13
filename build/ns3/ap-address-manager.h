/* 
 * File:   ap-address-manager.h
 * Author: francesco
 *
 * Created on December 29, 2008, 1:07 PM
 */

#ifndef _AP_ADDRESS_MANAGER_H
#define	_AP_ADDRESS_MANAGER_H

#include <map>
#include <queue>

#include "ns3/ptr.h"
#include "ns3/wifi-net-device.h"
#include "ns3/net-device-container.h"

using namespace std;

namespace ns3{
    /*
     * \brief A dhcp-like feature for wifi access points
     * Reacts to association and de-association events, assigning IP addresses to stations
     * according to the AP address base and availability
     */
    class ApAddressManager{
    public:
        typedef struct MulticastRoute{
            bool outgoing;
            Ipv4Address group;
            unsigned int in_interface;
            unsigned int out_interface;
            
            MulticastRoute(bool outgoing,Ipv4Address group,unsigned int in_interface,unsigned int out_interface);
        }MulticastRoute;
        /*
         * \brief Put an access point under the manager's control
         * \param dev The WifiNetDevice of the access point
         * \param addr_base The network part of the address, e.g. "192.168.1."
         * \param first_valid_addr The first address that can be assigned to a station, e.g. "2" if the first station will get the IP 192.168.1.2
         */
        void AddAccessPoint(Ptr<const NetDevice> dev,const string& addr_base,unsigned int first_valid_addr);
        /*
         * \brief Put a set of stations under the manager's control
         * \param dev The WifiNetDevice of the access point
         */
        void AddStations(NetDeviceContainer& devs);
        /*
         * A list of multicast routes to enforce at the access points
         */
        list<MulticastRoute>& MulticastRoutes(void);
    private:
        void assoc(string context,Mac48Address ap);
        void deassoc(string context,Mac48Address ap);
        Ipv4Address getAddr(Mac48Address ap,unsigned int *id=0);
        Mac48Address convert(const Address& addr) const;
        unsigned int getDeviceId(Ptr<const Node> n,TypeId type) const;
        unsigned int getNodeId(const string& path) const;
        
        typedef struct access_point{
            Mac48Address address;
            string addressBase;
            Ptr<Node> node;
            queue<unsigned int> addresses;
            Ipv4Address ip;
        } access_point;
        typedef struct station{
            unsigned int nodeId;
            Mac48Address accessPoint;
            unsigned int n_addr;
            Ipv4Address addr;
            Ipv4Address old_addr;
            Ipv4Mask old_mask;
        } station;
        
        map<Mac48Address,access_point> accessPoints;
        map<unsigned int,station> registeredStations;
        list<MulticastRoute> multicastRoutes;
    };
} //ns3

#endif	/* _AP_ADDRESS_MANAGER_H */

