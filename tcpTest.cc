#include <fstream>
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/animation-interface.h"
#include "ns3/ssid.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/applications-module.h"
#include <fstream>
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/internet-module.h"
#include "ns3/tcp-header.h"
#include  <string>
#include "ns3/traffic-control-module.h"
#include "ns3/queue.h"
#include "ns3/trace-helper.h"
#include "ns3/queue-disc.h"
#include "ns3/packet-sink.h"
using namespace ns3;



NS_LOG_COMPONENT_DEFINE ("nathan");


  uint32_t g_dropsObserved = 0;       //!< Number of dropped packets
  std::string g_validate = ""; 
 
  uint32_t checkTimes;
  double avgQueueDiscSize;
  std::stringstream filePlotQueueDisc;    //!< Output file name for queue disc size.
  std::stringstream filePlotQueueDiscAvg; //!< Output file name for queue disc average.
  double sink_stop_time;    //!< Sink stop time.
  int tcpSegmentSize = 1000;  
  uint32_t packetSize=0;
  uint32_t mtu_bytes = 1500;
  

std::ofstream fPlotQueue;

void
CheckQueueDiscSize(Ptr<QueueDisc> queue)
{
    uint32_t qSize = queue->GetCurrentSize().GetValue();

    avgQueueDiscSize += qSize;
    checkTimes++;

    // check queue disc size every 1/100 of a second
    Simulator::Schedule(Seconds(0.1), &CheckQueueDiscSize, queue);

    std::ofstream fPlotQueueDisc(filePlotQueueDisc.str(), std::ios::out | std::ios::app);
    fPlotQueueDisc << Simulator::Now().GetSeconds() << " " << qSize << std::endl;
    fPlotQueueDisc.close();

    std::ofstream fPlotQueueDiscAvg(filePlotQueueDiscAvg.str(), std::ios::out | std::ios::app);
    fPlotQueueDiscAvg << Simulator::Now().GetSeconds() << " " << avgQueueDiscSize / checkTimes
                      << std::endl;
    fPlotQueueDiscAvg.close();
}
void
TraceQueueDrop(std::ofstream* ofStream, Ptr<const QueueDiscItem> item)
{
    if (g_validate.empty())
    {
        *ofStream << Simulator::Now().GetSeconds() << " " << std::hex << item->Hash() << std::endl;
    }
    g_dropsObserved++;
}

double totalDelay = 0.0;
uint32_t packetsSent = 0;
uint32_t packetsReceived = 0;


void ReceivePacket(Ptr<Socket> socket) {
    Time now = Simulator::Now();
    Ptr<Packet> packet;
    Address addr;
    while ((packet = socket->RecvFrom(addr))) {
        packetsReceived++;

        // Calculate delay in seconds
        double delay = (now.GetSeconds() - packet->GetUid()) * 0.001; // Convert to milliseconds
        totalDelay += delay;

        // Uncomment the next line to see per-packet delay
        std::cout << "Packet Delay: " << delay << " ms" << std::endl;
    }
    
}

void CalculateMeanDelay() {
    double meanDelay = totalDelay / packetsReceived;
    std::cout << "Total Delay: " << totalDelay << " seconds" << std::endl;
    std::cout << "Mean Delay: " << meanDelay << " seconds" << std::endl;
}
void
TraceFirstCwnd(std::ofstream* ofStream, uint32_t oldCwnd, uint32_t newCwnd)
{
 
    
        *ofStream << Simulator::Now().GetSeconds() << " " << static_cast<double>(newCwnd)/1440
                  << std::endl;
                  
        std::cout << Simulator::Now().GetSeconds() << " " << static_cast<double>(newCwnd)/1440
                  << std::endl;
    }

void
ScheduleFirstTcpCwndTraceConnection(std::ofstream* ofStream)
{
    Config::ConnectWithoutContextFailSafe(
        "/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
        MakeBoundCallback(&TraceFirstCwnd, ofStream));
}



void PacketEnqueued (Ptr<const Packet> packet, uint32_t interface, const Address &srcAddress, const Address &destAddress) {
    // This function is called whenever a packet is enqueued

    // Calculate throughput here based on packet size and simulation time
    double packetSizeBits = packet->GetSize() * 8.0;
    double simulationTime = Simulator::Now().GetSeconds();

    double throughput12 = packetSizeBits / simulationTime;

    // Define your router1 IP address
    Ipv4Address router1Address = Ipv4Address("10.1.7.1"); // Replace with the actual IP address of router1

    // You can identify packets related to router1 using srcAddress or destAddress
    if (srcAddress == router1Address || destAddress == router1Address) {
        NS_LOG_INFO("Router1 throughput: " << throughput12 << " bps");
    }
}

// modifidation

void
TraceFirstRx(Ptr<const Packet> packet, const Address& address)
{
    packetSize += packet->GetSize();
   
}

void
TraceFirstThroughput(std::ofstream* ofStream, Time throughputInterval)
{
    double throughput = packetSize * 8 / throughputInterval.GetSeconds();
 
    
        std::cout << Simulator::Now().GetSeconds() << " " << throughput << std::endl;
        *ofStream << Simulator::Now().GetSeconds() << " " << throughput << std::endl;
    
}

void
ScheduleFirstPacketSinkConnection()
{
    Config::ConnectWithoutContextFailSafe("/NodeList/6/ApplicationList/*/$ns3::PacketSink/Rx",
                                          MakeCallback(&TraceFirstRx));
}



int main (int argc, char *argv[])
{

  double duration = 100; //seconds
  //std::string tr_name ("Fiverr1");
  std::string transport_prot;
  bool writeForPlot = true;
  bool errorControl = false;
  double error_rate = 0.000001;
  int protocol;
  std::string  pathOut = ".";
  Time throughputSamplingInterval = Seconds(2);
  uint32_t maxBytes = 0;
 

    //bool printAredStats = true;
  //pathOut = "."; // Current directory
  std::string aredLinkDataRate = "10Mbps";
  std::string aredLinkDelay = "2ms"; 
  std::string queueDisc = "ns3::FifoQueueDisc"; 
  std::string firstTcpThroughputTraceFile = "throughput.dat";
  std::string firstTcpCwndTraceFile = "tcp-validation-first-tcp-cwnd.dat";
  
   std::cout<<"Press 1 for TcpNewReno and Press 2 for TcpCubic"<<std::endl;
   std::cin>>protocol;
   
      if(protocol==1)
  {
    transport_prot = "TcpNewReno";
   }
  else if (protocol==2)
  {
   transport_prot = "TcpCubic";
   }

  else
  {
  std::cout<<"Invalic Choice! Please try again with 1 or 2"<<std::endl;
  }

   std::cout<<"Enter simulation time in seconds"<<std::endl;
   std::cin>>duration;
   
   
   
   // Calculate the ADU size

Header* temp_header = new Ipv4Header ();

uint32_t ip_header = temp_header->GetSerializedSize ();

NS_LOG_UNCOND ("IP Header size is: " << ip_header);

delete temp_header;

 

temp_header = new TcpHeader ();

uint32_t tcp_header = temp_header->GetSerializedSize ();

NS_LOG_UNCOND ("TCP Header size is: " << tcp_header);

delete temp_header;

 

uint32_t tcp_adu_size = mtu_bytes - 20 - (ip_header + tcp_header);

NS_LOG_UNCOND ("TCP ADU size is: " << tcp_adu_size);

    
  
  //uint32_t meanPktSize = 1000;
  transport_prot = std::string ("ns3::") + transport_prot;
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue (transport_prot));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcp_adu_size));
   
  //Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(32768000));//buffer size for 1st flow
  //Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(32768000));
  Config::SetDefault("ns3::FifoQueueDisc::MaxSize", StringValue("200p"));
  Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
    

   CommandLine cmd (__FILE__);
   cmd.AddValue ("duration", "Simulation time in seconds", duration);
   cmd.AddValue ("errorControl", "enable or disable error control", errorControl);
   cmd.Parse (argc, argv);
  LogComponentEnable ("FifoQueueDisc", LOG_LEVEL_INFO);
  // LogComponentEnable("ns3::QueueDisc", LOG_LEVEL_ALL);

 std::ofstream firstTcpCwndOfStream;
 firstTcpCwndOfStream.open(firstTcpCwndTraceFile, std::ofstream::out);



  NS_LOG_INFO ("Create nodes."); 
  NodeContainer c;   
  c.Create (6);   //Create Total Nodes

  NodeContainer r;   
  r.Create (2);   //Create Total router
  
    NS_LOG_INFO("Install internet stack on all nodes.");
    InternetStackHelper internet; 
    internet.Install (c);
    internet.Install (r);
  NodeContainer n0r0 = NodeContainer (c.Get (0), r.Get (0)); 
  NodeContainer n1r0 = NodeContainer (c.Get (1), r.Get (0)); 
  NodeContainer n2r0 = NodeContainer (c.Get (2), r.Get (0)); 
  NodeContainer n3r1 = NodeContainer (c.Get (3), r.Get (1));
  NodeContainer n4r1 = NodeContainer (c.Get (4), r.Get (1));
  NodeContainer n5r1 = NodeContainer (c.Get (5), r.Get (1));
  NodeContainer r0r1 = NodeContainer (r.Get (0), r.Get (1));

  //modification
  TrafficControlHelper tchPfifo;
  //uint16_t handle = tchPfifo.SetRootQueueDisc("ns3::PfifoFastQueueDisc");
  //tchPfifo.AddInternalQueues(handle, 3, "ns3::DropTailQueue", "MaxSize", StringValue("1000p"));
  
  tchPfifo.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "MaxSize", StringValue ("1000p"));

  TrafficControlHelper tchPi;
 tchPi.SetRootQueueDisc("ns3::FifoQueueDisc");


  // We create the channels first without any IP addressing information
  NS_LOG_INFO ("Create channels.");
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer d0 = p2p.Install (n0r0);
  NetDeviceContainer d1 = p2p.Install (n1r0);
  NetDeviceContainer d2 = p2p.Install (n2r0);
  NetDeviceContainer d3 = p2p.Install (n3r1);
  NetDeviceContainer d4 = p2p.Install (n4r1);
  NetDeviceContainer d5 = p2p.Install (n5r1);
  NetDeviceContainer d7 = p2p.Install (r0r1);

  //Router devices
  NetDeviceContainer d6;
  QueueDiscContainer queueDiscs;
    
 // Limit the bandwidth on the wanRouter->lanRouter interface
    //Ptr<PointToPointNetDevice> p = d6.Get(0)->GetObject<PointToPointNetDevice>();
   
    
    p2p.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("200p")));
    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));    
    tchPfifo.Install(d0); 
    tchPfifo.Install(d1);
    tchPfifo.Install(d2); 
    tchPfifo.Install(d3);
    tchPfifo.Install(d4);
    tchPfifo.Install(d5);
    
    p2p.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("200p")));
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    d6 = p2p.Install(r.Get(0), r.Get(1));
    //d6.Get(0)->SetMtu(1500);  // Set MTU for device 0
    // only backbone link has ARED queue disc
    queueDiscs = tchPi.Install(d6);
    
    // Retrieve the channel and its attributes
    Ptr<PointToPointChannel> channel = d6.Get(0)->GetChannel()->GetObject<PointToPointChannel>();
    TimeValue delayValue;
    channel->GetAttribute("Delay", delayValue);
    Time delay1 = delayValue.Get();
    
    
    
    /*
    p2p.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("200p")));
    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms")); 
    d1  = p2p.Install (n1r0);   
    tchPfifo.Install(d1); 
    
    
    p2p.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("200p")));
    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms")); 
    d2  = p2p.Install (n2r0);   
    tchPfifo.Install(d2); 
    
    
    p2p.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("200p")));
    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms")); 
    d3  = p2p.Install (n3r1);   
    tchPfifo.Install(d3); 
    
    
    p2p.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("200p")));
    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms")); 
    d4  = p2p.Install (n4r1);   
    tchPfifo.Install(d4); 
    
    
    p2p.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("200p")));
    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms")); 
    d5  = p2p.Install (n5r1);   
    tchPfifo.Install(d5); 
    
 */

if(errorControl==true)
{
  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (error_rate));
  d4.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
 // d2d5.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
}

  // Mobilty and location of the devices setting here
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator> ();
   allocator->Add (Vector (0,10,0)); 
   allocator->Add (Vector (0,20,0));
   allocator->Add (Vector (0,30,0)); 
   allocator->Add (Vector (30,10,0)); 
   allocator->Add (Vector (30,20,0)); 
   allocator->Add (Vector (30,30,0)); 
   allocator->Add (Vector (10,20,0)); 
   allocator->Add (Vector (20,20,0)); 
 

  mobility.SetPositionAllocator (allocator);     
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c);
  mobility.Install (r);


   NS_LOG_INFO ("assigning ip address");

//   IPv4 addresses.
  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer id0 = ipv4.Assign (d0);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer id1 = ipv4.Assign (d1);

  ipv4.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer id2 = ipv4.Assign (d2);   

 ipv4.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer id3 = ipv4.Assign (d3);

 ipv4.SetBase ("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer id4 = ipv4.Assign (d4);

 ipv4.SetBase ("10.1.6.0", "255.255.255.0");
  Ipv4InterfaceContainer id5 = ipv4.Assign (d5);

 ipv4.SetBase ("10.1.7.0", "255.255.255.0");
  Ipv4InterfaceContainer id6 = ipv4.Assign (d6);
  
  ipv4.SetBase ("10.1.8.0", "255.255.255.0");
  Ipv4InterfaceContainer id7 = ipv4.Assign (d7);
  
  
  // Create a BulkSendApplication and install it on node 0
   //
     uint16_t port = 9;  // well-known echo port number
     
     //NODE 1
     
     BulkSendHelper source1A ("ns3::TcpSocketFactory",Address());
    // Set the amount of data to send in bytes.  Zero is unlimited.
     source1A.SetAttribute ("SendSize", UintegerValue (tcp_adu_size));
     InetSocketAddress firstDestAddress1A(id5.GetAddress(0), port);
     source1A.SetAttribute("Remote", AddressValue(firstDestAddress1A));
     ApplicationContainer sourceApps1A = source1A.Install(c.Get (0));
     sourceApps1A.Start (Seconds (0.0));
     sourceApps1A.Stop (Seconds (100));
  
     
     //BulkSendHelper source1B ("ns3::TcpSocketFactory",Address());
    // Set the amount of data to send in bytes.  Zero is unlimited.
     //source1B.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
     InetSocketAddress firstDestAddress1B(id4.GetAddress(0), port);
     source1A.SetAttribute("Remote", AddressValue(firstDestAddress1B));
     ApplicationContainer sourceApps1B = source1A.Install(c.Get (0));
     sourceApps1B.Start (Seconds (0.0));
     sourceApps1B.Stop (Seconds (100));

     Address firstSinkAddress1S(InetSocketAddress(Ipv4Address::GetAny(), port));
     ApplicationContainer firstSinkApp1S;
     PacketSinkHelper firstSinkHelper1S("ns3::TcpSocketFactory", firstSinkAddress1S);
     firstSinkApp1S = firstSinkHelper1S.Install(c.Get(3));
     firstSinkApp1S.Start (Seconds (0.0));
     firstSinkApp1S.Stop (Seconds (100));

     
     //BulkSendHelper source1C ("ns3::TcpSocketFactory",Address());
    // Set the amount of data to send in bytes.  Zero is unlimited.
     //source1C.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
     InetSocketAddress firstDestAddress1C(id3.GetAddress(0), port);
     source1A.SetAttribute("Remote", AddressValue(firstDestAddress1C));
     ApplicationContainer sourceApps1C = source1A.Install(c.Get (0));
     sourceApps1C.Start (Seconds (0.0));
     sourceApps1C.Stop (Seconds (100));
  
     
     //NODE 2
     
     //BulkSendHelper source ("ns3::TcpSocketFactory",Address());
    // Set the amount of data to send in bytes.  Zero is unlimited.
     //source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
     InetSocketAddress firstDestAddress(id5.GetAddress(0), port);
     source1A.SetAttribute("Remote", AddressValue(firstDestAddress));
     ApplicationContainer sourceApps = source1A.Install(c.Get (1));
     sourceApps.Start (Seconds (0.0));
     sourceApps.Stop (Seconds (100));
  
     
     //BulkSendHelper source3 ("ns3::TcpSocketFactory",Address());
    // Set the amount of data to send in bytes.  Zero is unlimited.
     //source3.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
     InetSocketAddress firstDestAddress3(id4.GetAddress(0), port);
     source1A.SetAttribute("Remote", AddressValue(firstDestAddress3));
     ApplicationContainer sourceApps3 = source1A.Install(c.Get (1));
     sourceApps3.Start (Seconds (0.0));
     sourceApps3.Stop (Seconds (100));

     Address firstSinkAddress4(InetSocketAddress(Ipv4Address::GetAny(), port));
     ApplicationContainer firstSinkApp4;
     PacketSinkHelper firstSinkHelper4("ns3::TcpSocketFactory", firstSinkAddress4);
     firstSinkApp4 = firstSinkHelper4.Install(c.Get(4));
     firstSinkApp4.Start (Seconds (0.0));
     firstSinkApp4.Stop (Seconds (100));
     
     //BulkSendHelper source6 ("ns3::TcpSocketFactory",Address());
    // Set the amount of data to send in bytes.  Zero is unlimited.
     //source6.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
     InetSocketAddress firstDestAddress6(id3.GetAddress(0), port);
     source1A.SetAttribute("Remote", AddressValue(firstDestAddress6));
     ApplicationContainer sourceApps6 = source1A.Install(c.Get (1));
     sourceApps6.Start (Seconds (0.0));
     sourceApps6.Stop (Seconds (100));
     
     
    //NODE 3
     
     //BulkSendHelper source3A ("ns3::TcpSocketFactory",Address());
    // Set the amount of data to send in bytes.  Zero is unlimited.
     //source3A.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
     InetSocketAddress firstDestAddress3A(id5.GetAddress(0), port);
     source1A.SetAttribute("Remote", AddressValue(firstDestAddress3A));
     ApplicationContainer sourceApps3A = source1A.Install(c.Get (2));
     sourceApps3A.Start (Seconds (0.0));
     sourceApps3A.Stop (Seconds (100));
  
     
     //BulkSendHelper source3B ("ns3::TcpSocketFactory",Address());
    // Set the amount of data to send in bytes.  Zero is unlimited.
     //source3B.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
     InetSocketAddress firstDestAddress3B(id4.GetAddress(0), port);
     source1A.SetAttribute("Remote", AddressValue(firstDestAddress3B));
     ApplicationContainer sourceApps3B = source1A.Install(c.Get (2));
     sourceApps3B.Start (Seconds (0.0));
     sourceApps3B.Stop (Seconds (100));

     Address firstSinkAddress3S(InetSocketAddress(Ipv4Address::GetAny(), port));
     ApplicationContainer firstSinkApp3S;
     PacketSinkHelper firstSinkHelper3S("ns3::TcpSocketFactory", firstSinkAddress3S);
     firstSinkApp3S = firstSinkHelper3S.Install(c.Get(5));
     firstSinkApp3S.Start (Seconds (0.0));
     firstSinkApp3S.Stop (Seconds (100));
     
     //BulkSendHelper source3C ("ns3::TcpSocketFactory",Address());
    // Set the amount of data to send in bytes.  Zero is unlimited.
     //source3C.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
     InetSocketAddress firstDestAddress3C(id3.GetAddress(0), port);
     source1A.SetAttribute("Remote", AddressValue(firstDestAddress3C));
     ApplicationContainer sourceApps3C = source1A.Install(c.Get (2));
     sourceApps3C.Start (Seconds (0.0));
     sourceApps3C.Stop (Seconds (100));
     

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();  //Enable Default Routing from source to Destination
  //Ptr<FlowMonitor> monitor;
   FlowMonitorHelper flowmon;
   Ptr<FlowMonitor> monitor = flowmon.InstallAll();
   monitor = flowmon.Install(r.Get(0)); // Monitor flows passing through the router node
   
     
   std::ofstream firstTcpThroughputOfStream;
   firstTcpThroughputOfStream.open(firstTcpThroughputTraceFile, std::ofstream::out);
   
   Simulator::Schedule(Seconds(0.0), &ScheduleFirstPacketSinkConnection);
   
   Simulator::Schedule(throughputSamplingInterval,
                        &TraceFirstThroughput,
                        &firstTcpThroughputOfStream,
                        throughputSamplingInterval);
         
  
  // save queue size
   if (writeForPlot)
    {
        filePlotQueueDisc << pathOut << "/"
                          << "queue.txt";
        filePlotQueueDiscAvg << pathOut << "/"
                             << "AverageQueue.txt";

        remove(filePlotQueueDisc.str().c_str());
        remove(filePlotQueueDiscAvg.str().c_str());
        Ptr<QueueDisc> queue = queueDiscs.Get(0);
        Simulator::ScheduleNow(&CheckQueueDiscSize, queue);
        
        
    }
    


  Simulator::Schedule(Seconds(0.0000001),
                        &ScheduleFirstTcpCwndTraceConnection,
                        &firstTcpCwndOfStream);
 

  NS_LOG_INFO ("Run Simulation.");

   Simulator::Stop (Seconds (duration));
  AnimationInterface anim ("tcpnathan.xml");
  anim.SetMaxPktsPerTraceFile(99999999999999);
  Simulator::Run ();
Ptr < Ipv4FlowClassifier > classifier = DynamicCast < Ipv4FlowClassifier >(flowmon.GetClassifier());
	std::map < FlowId, FlowMonitor::FlowStats > stats = monitor->GetFlowStats();

	double Delaysum = 0;
        //double jitterSum = 0;
	uint64_t txPacketsum = 0;
	uint64_t rxPacketsum = 0;
	uint32_t txPacket = 0;
	uint32_t rxPacket = 0;
        uint32_t PacketLoss = 0;
        uint64_t txBytessum = 0; 
	uint64_t rxBytessum = 0;
	uint64_t DropRatio = 0.0;
	double throughput15;
	Ipv4Address routerIp = id6.GetAddress(0);
        
       double delay;
       //double jitter;
       	double throughput = 0;
	for (std::map < FlowId, FlowMonitor::FlowStats > ::const_iterator iter = stats.begin(); iter != stats.end(); ++iter) {
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);
                NS_LOG_UNCOND("*****************************************");
		NS_LOG_UNCOND("Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
		
                  txPacket = iter->second.txPackets;
                  rxPacket = iter->second.rxPackets;
                  PacketLoss = txPacket - rxPacket;
                  delay = iter->second.delaySum.GetSeconds();
               //   jitter = iter->second.jitterSum.GetMilliSeconds();
         std::cout << "  Tx Packets: " << iter->second.txPackets << "\n";
         std::cout << "  Rx Packets: " << iter->second.rxPackets << "\n";
         std::cout << "  Packet Loss: " << PacketLoss << "\n";
  	 //std::cout << "  Tx Bytes:   " << iter->second.txBytes << "\n";
         // std::cout << "  Rx Bytes:   " << iter->second.rxBytes << "\n";
         std::cout << "  Throughput: " << iter->second.rxBytes * 8.0 / (duration * 1e6) << " Mbps\n";
         //NS_LOG_UNCOND("  Mean Delay: " << delay / txPacket << " ms");
        // NS_LOG_UNCOND("  Per Node Jitter: " << jitter / txPacket << " ms");
        // std::cout << "   PDR for current flow ID : " << ((rxPacket *100) / txPacket) << "%" << "\n";
     
		txPacketsum += iter->second.txPackets;
		rxPacketsum += iter->second.rxPackets;
		txBytessum += iter->second.txBytes;
		rxBytessum += iter->second.rxBytes;
		Delaysum += iter->second.delaySum.GetSeconds();
		
            //    jitterSum += iter->second.jitterSum.GetMilliSeconds();
                 DropRatio = txPacketsum-rxPacketsum;
     }                
             NS_LOG_UNCOND("***********Sum of Results*************");

	throughput = txBytessum * 8 / (duration * 1e6); //Mbit/s  
	NS_LOG_UNCOND("Sent Packets = " << txPacketsum);
	NS_LOG_UNCOND("Received Packets = " << rxPacketsum);
        NS_LOG_UNCOND("Total Packet Loss = " << (txPacketsum-rxPacketsum));
       // NS_LOG_UNCOND("Total Byte Sent = "<<txBytessum);
       // NS_LOG_UNCOND("Total Byte Received = "<<rxBytessum);
       	std::cout << "Channel Delay: " << delay1.GetMilliSeconds() << " ms" << std::endl;
       	//NS_LOG_UNCOND("Total Delay: " << Delaysum << " ms");
      	//NS_LOG_UNCOND("Mean Delay: " << Delaysum / txPacketsum << " s");
        //NS_LOG_UNCOND("Jitter: " << jitterSum / txPacketsum << " ms");
	std::cout << "Average Throughput = "<<throughput << " Mbit/s" << std::endl;
	//std::cout << "Average Throughput router1 = "<<throughput15 << " Mbit/s" << std::endl;
        std::cout << "Packets Delivery Ratio: " << ((rxPacketsum *100.0) / txPacketsum) << "%" << "\n";
        std::cout << "Drop Ratio: " << DropRatio*100.0/ rxPacketsum << "%" << "\n"; 
  
         // set up statistic queue
        QueueDisc::Stats st = queueDiscs.Get(0)->GetStats();
  	std::cout << "Statistics: " << std::endl;
  	st.Print(std::cout);
        monitor->SerializeToXmlFile("ndm.flowmon", true, true);
 
    Simulator::Stop(Seconds(sink_stop_time));
    Simulator::Run();
    Simulator::Destroy ();

    firstTcpThroughputOfStream.close();
    firstTcpCwndOfStream.close();
   
    
   
}


