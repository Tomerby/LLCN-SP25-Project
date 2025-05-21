#pragma once

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

class FatTreeTopology {
public:
    FatTreeTopology();
    FatTreeTopology& withK(uint32_t k);
    FatTreeTopology& withP2P(PointToPointHelper p2p);

    void Build();
    void configureNetAnim(std::string xmlOutput);
    std::vector<NetDeviceContainer> GetAllLinks();

    NodeContainer& GetCore();
    NodeContainer& GetAgg();
    NodeContainer& GetEdge();
    NodeContainer& GetHosts();

private:
    void CreateNodes();
    void ConnectNodes();
    void InstallInternetStack();
    void AssignIPAddresses();

    uint32_t m_k;
    PointToPointHelper m_p2p;

    NodeContainer m_coreSwitches, m_aggSwitches, m_edgeSwitches, m_hosts;
    std::vector<NetDeviceContainer> m_links;
};
