#include "FatTreeTopology.h"

FatTreeTopology::FatTreeTopology() : m_k(4) {
    m_p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    m_p2p.SetChannelAttribute("Delay", StringValue("10us"));
    m_p2p.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p")); // Queue holding 60 packets
}
FatTreeTopology& FatTreeTopology::withK(uint32_t k) {
    m_k = k;
    return *this;
}
FatTreeTopology& FatTreeTopology::withP2P(PointToPointHelper p2p) {
    m_p2p = p2p;
    return *this;
}

void FatTreeTopology::Build() {
    CreateNodes();
    InstallInternetStack();
    ConnectNodes();
    AssignIPAddresses();
}

void FatTreeTopology::CreateNodes() {
    uint32_t numCore = (m_k / 2) * (m_k / 2);
    uint32_t numAgg = (m_k * m_k) / 2;
    uint32_t numEdge = numAgg;
    uint32_t numHosts = m_k * m_k * m_k / 4;

    m_coreSwitches.Create(numCore);
    m_aggSwitches.Create(numAgg);
    m_edgeSwitches.Create(numEdge);
    m_hosts.Create(numHosts);
}

void FatTreeTopology::InstallInternetStack() {
    InternetStackHelper internet;
    internet.Install(m_coreSwitches);
    internet.Install(m_aggSwitches);
    internet.Install(m_edgeSwitches);
    internet.Install(m_hosts);
}

void FatTreeTopology::ConnectNodes() {

    // Core to Aggregation Links
    for (uint32_t pod = 0; pod < m_k; ++pod) {
        for (uint32_t i = 0; i < m_k / 2; ++i) {
            for (uint32_t j = 0; j < m_k / 2; ++j) {
                uint32_t coreIndex = i * (m_k / 2) + j;
                uint32_t aggIndex = pod * (m_k / 2) + i;
                m_links.push_back(m_p2p.Install(m_coreSwitches.Get(coreIndex), m_aggSwitches.Get(aggIndex)));
            }
        }
    }

    // Aggregation to Edge Links
    for (uint32_t pod = 0; pod < m_k; ++pod) {
        for (uint32_t i = 0; i < m_k / 2; ++i) {
            for (uint32_t j = 0; j < m_k / 2; ++j) {
                uint32_t aggIndex = pod * (m_k / 2) + i;
                uint32_t edgeIndex = pod * (m_k / 2) + j;
                m_links.push_back(m_p2p.Install(m_aggSwitches.Get(aggIndex), m_edgeSwitches.Get(edgeIndex)));
            }
        }
    }

    // Edge to Host Links
    for (uint32_t pod = 0; pod < m_k; ++pod) {
        for (uint32_t edgeIndex = pod * (m_k / 2); edgeIndex < (pod + 1) * (m_k / 2); ++edgeIndex) {
            for (uint32_t i = 0; i < m_k / 2; ++i) {
                uint32_t hostIndex = edgeIndex * (m_k / 2) + i;
                m_links.push_back(m_p2p.Install(m_edgeSwitches.Get(edgeIndex), m_hosts.Get(hostIndex)));
            }
        }
    }
}


void FatTreeTopology::AssignIPAddresses() {
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.0.0", "255.255.255.0");

    for (auto& link : m_links) {
        Ipv4InterfaceContainer interface = ipv4.Assign(link);
        ipv4.NewNetwork();
    }
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
}

std::vector<NetDeviceContainer> FatTreeTopology::GetAllLinks() {
    return m_links;
}

NodeContainer& FatTreeTopology::GetCore() { return m_coreSwitches; }
NodeContainer& FatTreeTopology::GetAgg() { return m_aggSwitches; }
NodeContainer& FatTreeTopology::GetEdge() { return m_edgeSwitches; }
NodeContainer& FatTreeTopology::GetHosts() { return m_hosts; }

 void SetPosition(Ptr<Node> node, double x, double y) {
     Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
     mob->SetPosition(Vector(x, y, 0));
 }

void FatTreeTopology::configureNetAnim(std::string xmlOutput) {

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(m_coreSwitches);
    mobility.Install(m_aggSwitches);
    mobility.Install(m_edgeSwitches);
    mobility.Install(m_hosts);

	AnimationInterface anim (xmlOutput);
	double xSpacing = 40.0; // Spacing between nodes
	double ySpacing = 150.0; // Spacing between layers
	double xoffset  = 150.0;   // offset each layer begins with
	
	// Core switches (Layer 1)
	for (uint32_t i = 0; i < m_coreSwitches.GetN(); ++i) {
	    SetPosition(m_coreSwitches.Get(i), i * xSpacing + xoffset*1.5, ySpacing * 0);
	    uint32_t switchId = m_coreSwitches.Get(i)->GetId();
	    anim.UpdateNodeColor(switchId, 255, 255, 0);  // RGB for yellow
	    anim.UpdateNodeSize(switchId, 8.0, 8.0);      // Medium size
	   
	}
	
	// Aggregation switches (Layer 2)
	for (uint32_t i = 0; i < m_aggSwitches.GetN(); ++i) {
	    SetPosition(m_aggSwitches.Get(i), i * xSpacing + xoffset, ySpacing * 1);
	    uint32_t switchId = m_aggSwitches.Get(i)->GetId();
	    anim.UpdateNodeColor(switchId, 0, 0, 255);  // RGB for blue
	    anim.UpdateNodeSize(switchId, 8.0, 8.0);    // Medium size
	}
	
	// Edge switches (Layer 3)
	for (uint32_t i = 0; i < m_edgeSwitches.GetN(); ++i) {
	    SetPosition(m_edgeSwitches.Get(i), i * xSpacing + xoffset , ySpacing*2);
	    uint32_t switchId = m_edgeSwitches.Get(i)->GetId();
	    anim.UpdateNodeColor(switchId, 0, 255, 0);  // RGB for green
	    anim.UpdateNodeSize(switchId, 8.0, 8.0);    // Medium size
	}
	
	// Hosts (Layer 4)
	for (uint32_t i = 0; i < m_hosts.GetN(); ++i) {
	    SetPosition(m_hosts.Get(i), i * xSpacing, ySpacing * 3);
	    uint32_t hostId = m_hosts.Get(i)->GetId();
	    anim.UpdateNodeColor(hostId, 255, 0, 0);  // RGB for red
	    anim.UpdateNodeSize(hostId, 8.0, 8.0);    // Large size
	}
}