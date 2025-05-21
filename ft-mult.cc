#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/traffic-control-module.h"
#include <chrono>
#include <iostream>

#include "FatTreeTopology.h"

using namespace ns3;

int main(int argc, char *argv[]) {
    uint32_t k = 4;
    std::string senderRate = "100kbps", withAnim = "N", xmlOutput = "XML/animation.xml";
    CommandLine cmd;
    cmd.AddValue("k", "Number of ports per switch", k);
    cmd.AddValue("dataRate", "Data Rate of the Senders", senderRate);
    cmd.AddValue("withAnim", "Run simulation with Animation", withAnim);
    cmd.AddValue("animPath", "Path to the output of the netAnim", xmlOutput);
    cmd.Parse(argc, argv);


    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2p.SetChannelAttribute("Delay", StringValue("10us"));
    p2p.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));

    FatTreeTopology topology;
    topology.withK(k).withP2P(p2p);
    topology.Build();

    ///// START TCP APPLICATION /////

    uint16_t basePort = 50000; // Base port for applications
    std::vector<ApplicationContainer> sinkApps;
    std::vector<ApplicationContainer> sourceApps;
    uint32_t numFlows = 2; // Number of flows to simulate

    NodeContainer& hosts = topology.GetHosts();

    for (uint32_t i = 0; i < numFlows; ++i) {
        uint16_t port = basePort + i;

        // Select sender and receiver hosts
        Ptr<Node> sender = hosts.Get(i % hosts.GetN());
        Ptr<Node> receiver = hosts.Get((hosts.GetN() - 1 - i) % hosts.GetN());

        // Install PacketSink on the receiver
        Address sinkAddress(InetSocketAddress(receiver->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), port));
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkAddress);
        ApplicationContainer sinkApp = sinkHelper.Install(receiver);
        sinkApp.Start(Seconds(1.0));
        sinkApp.Stop(Seconds(5.0));
        sinkApps.push_back(sinkApp);


        // Install on off on the sender
        OnOffHelper onOffHelper("ns3::TcpSocketFactory", sinkAddress);
        onOffHelper.SetAttribute("DataRate", StringValue(senderRate)); 	// Sending rate
        onOffHelper.SetAttribute("PacketSize", UintegerValue(1024)); 	// Packet size (bytes)
        ApplicationContainer sourceApp = onOffHelper.Install(sender);
        sourceApp.Start(Seconds(2.0));
        sourceApp.Stop(Seconds(5.0));

    }  

    ///// END TCP APPLICATION /////
    
    // Measure throughput on avarage in the system after 4 seconds of simulation
    Simulator::Schedule(Seconds(4.0), [sinkApps, senderRate]() {
			double sumMbps = 0.0;
	        for (uint32_t i = 0; i < sinkApps.size(); ++i) {
				Ptr<PacketSink> sink = DynamicCast<PacketSink>(sinkApps[i].Get(0));
				sumMbps += sink->GetTotalRx() * 8.0 / (3.0 * 1e6); // Throughput in Mbps
			}
			std::cout << "(DataRate = " << senderRate << ") AVG Throughput = " <<  sumMbps / static_cast<double>(sinkApps.size()) << " Mbps" << std::endl;
        });
        

    Simulator::Stop(Seconds(5.0));
    auto simStart = std::chrono::high_resolution_clock::now();
    Simulator::Run();
    
    auto simEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = simEnd - simStart;
    std::cout << "Simulation runtime: " << elapsed.count() << " seconds" << std::endl << std::endl;
    
    Simulator::Destroy();
    return 0;
}