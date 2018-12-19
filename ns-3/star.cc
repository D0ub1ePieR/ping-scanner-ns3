#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
using namespace ns3;
 
NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");
 
int
main (int argc, char *argv[])
{
  Time::SetResolution (Time::NS);
 
  bool verbose = true;
  uint32_t nWifi = 5; //设置 无线站点个数
  //bool tracing = false;
 
  CommandLine cmd;
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose); 
  //用命令行修改参数
  cmd.Parse (argc,argv);
 
 
  if (nWifi > 250)
    {
      std::cout << "Too many wifi or csma nodes, no more than 250 each." << std::endl;
      return 1;
    }
 
  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_ALL);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_ALL);
    }
 
 
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);  //创建无线站点STA的节点
  NodeContainer wifiApNode;
  wifiApNode.Create(1);   //创建一个AP节点
   
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();   //使用默认的信道模型
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();  //使用默认的PHY模型
  phy.SetChannel (channel.Create ());  //创建通道对象并把他关联到物理层对象管理器
 
  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");  //设置wifi助手用AARF速率控制算法 
 
  //配置Mac类型以及基础设置SSID
  WifiMacHelper mac; 
  Ssid ssid = Ssid ("ns-3-aqiao");  //设置SSID的名字为ns-3-aqiao
  mac.SetType ("ns3::StaWifiMac",    //指定mac层指定为ns3::StaWifiMac
               "Ssid", SsidValue (ssid),   //设置默认AP是ssid对象
               "ActiveProbing", BooleanValue (false));  //设置不会发出主动探测AP的指令，默认AP是ssid
 
  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);  //在MAC层和PHY层用以前安装方法来创建这些无线设备
 
  mac.SetType ("ns3::ApWifiMac",   //指定为AP
               "Ssid", SsidValue (ssid));   
 
  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);   //一起共享PHY层的属性
 
  // 加入移动模型，让STA可以移动，AP固定
  MobilityHelper mobility; 
 
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));
  //设置初始时，节点的摆放位置
    
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",   //设置移动模式为"ns3::RandomWalk2dMobilityModel"，随机移动
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));   //移动范围
  mobility.Install (wifiStaNodes);
   
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");  //设置AP的不能够移动的，"ns3::ConstantPositionMobilityModel"
  mobility.Install (wifiApNode);
 
  InternetStackHelper stack;
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);   //安装协议栈
 
  Ipv4AddressHelper address;
 
  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer wifiInterfaces;
  wifiInterfaces=address.Assign (staDevices);
  address.Assign (apDevices);                 //地址分配
 
  //安装暗涌程序，使用UDP，与之前的UDP的使用方法一致
  UdpEchoServerHelper echoServer (9);
 
  ApplicationContainer serverApps = echoServer.Install (wifiStaNodes.Get (0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (15.0));
 
  UdpEchoClientHelper echoClient (wifiInterfaces.GetAddress (0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (5000));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.1)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
 
  ApplicationContainer clientApps =
    echoClient.Install (wifiStaNodes.Get (nWifi - 1));
  clientApps.Start (Seconds (1.0));
  clientApps.Stop (Seconds (15.0));
 
  AsciiTraceHelper ascii;
  phy.EnableAsciiAll (ascii.CreateFileStream ("star.tr"));
 
  Simulator::Stop (Seconds (15.0));
 
  phy.EnablePcap ("starwifi", apDevices.Get (0));
 AnimationInterface anim("anim3.xml");
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
