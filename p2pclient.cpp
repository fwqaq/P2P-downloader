#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>
#include <string>
#include <sys/types.h>
#include <ifaddrs.h>
#include "httplib.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <thread>
#include <boost/thread/thread.hpp>
using namespace httplib;
namespace bf = boost::filesystem;
const std::string  DOWNPATH =  "./download";

const int  MAXFILE = 100*1024*1024;

class P2pclient{
  private:
    //主机列表
    std::vector<std::string> _hostpath;
    //在线列表
    std::vector<std::string> _onlinepath;
    //用户选择的用户
    int _onlineindex;

    //文件的列表
    std::vector<std::string> _file_list;

    //得到全部主机序列
    void GetHostpath(){
      //通过getifaddr()来获取主机的ip和地址
      struct ifaddrs* addrs = NULL;
      getifaddrs(&addrs);
      while(addrs){
        if(std::string(addrs->ifa_name) == "lo"){
          addrs = addrs->ifa_next;
          continue;
        }
        //获取当前的ip地址和掩码
        struct sockaddr_in* addr = (struct sockaddr_in*)addrs->ifa_addr;
        struct sockaddr_in* mask = (struct sockaddr_in*)addrs->ifa_netmask;
        //如果不是IPV4的话就直接略过
        if(addr->sin_family != AF_INET){
          addrs = addrs->ifa_next;
          continue;
        }
        if(addr->sin_addr.s_addr == inet_addr("127.0.0.1")){
          addrs = addrs->ifa_next;
          continue;
        }
        //此时获取出网络号和掩码并且转化为本机字节序
        uint32_t ipaddr = ntohl(addr->sin_addr.s_addr & \
            mask->sin_addr.s_addr);
        //最大主机数
        uint32_t hostnum = ntohl(~(mask->sin_addr.s_addr));
        //将数据添加到hostpath中去
        int i;
        for(i = 2;i < hostnum-1;i++){
            struct in_addr ip;
            ip.s_addr = htonl(ipaddr+i);
            //将字节序转换为字符串
            std::cerr <<inet_ntoa(ip)<<"在范围中\n";
            _hostpath.push_back(inet_ntoa(ip));
        }
        addrs = addrs->ifa_next;
      }
      return;
    }


    void Thr_Online(int i){

        Client client(_hostpath[i].c_str(),9000);
        auto res = client.Get("/");
        if(res && res->status == 200){
          //服务端返回200状态码此时成功
          std::cerr << _hostpath[i] << ":匹配成功\n";
          _onlinepath.push_back(_hostpath[i]);
        }
        else{
          printf("%s:匹配失败\n",_hostpath[i].c_str());
        }
    }
  
  //匹配局域网中在线的主机序列
    void GetOnlinepath(){
      //向局域网中每一个地址都发送一个"/"，在服务端收到的时候就会返回一个200，此时就表示成功了
      std::vector<std::thread> pth;//用来存储线程，方便以后一起进行join
      pth.resize(_hostpath.size()-3);
      int i = 0,j = 0;
      for(i = 2;i < (int)_hostpath.size()-1; i++){
        //对每一个主机都建立一个线程去发送请求
        pth[j++] = std::thread(&P2pclient::Thr_Online, this, i);
        //每一个都发送一个请求
      }
      for(i = 0;i < pth.size(); i++){
        pth[i].join();
      }
      //打印在线的主机序列
      std::cout << "在线的主机序列如下：\n";
        for(i = 0; i < _onlinepath.size(); i++){
          std::cout<<"[" << i  <<"]" <<_onlinepath[i]<<std::endl;
        }
      //选择特定的ip地址向服务端发起请求，得到共享文件目录下面的文件夹
      std::cout<<"请输入你想连接的ip地址序号:";
      std::flush(std::cout);
      std::cin >> _onlineindex;  
      return;
    }
    void GetFileList(){
      //ip地址
      std::string ip_addr = _onlinepath[_onlineindex];
      //现在将向服务端发起请求，得到我们的数据
      Client client(ip_addr.c_str(),9000);
      //存储我们获得的共享目录下的文件，
      std::vector<std::string> file_list;
      auto res = client.Get("/list");
      //从res中得到数据
      if(res && res->status == 200){
        //此时得到数据成功，将数据使用boost库中库sqlit来切割
        std::string body = res->body;
        //使用boost库中的split(file_list,sec_string,is_any_of(""))
        boost::split(file_list,body,boost::is_any_of("\n"));

      }else{
        std::cerr << "获取共享目录文件失败\n";
        return;
      }
      std::cout<< ip_addr <<"下的共享目录下的文件：\n";
      int i;
      for(int i = 0;i < file_list.size(); i++){
        if(file_list[i].empty()){
          continue;
        }
        std::cout<<"[" << i <<"]" << file_list[i] << std::endl;
        _file_list.push_back(file_list[i]);
      }
      return;
    }
  
    //线程入口函数
    void thr_begin(std::string path, uint64_t begin,uint64_t end, int *flag, int fd){
      
      //调试信息<
      std::cout << "begin: "<< begin << "end:"<<end<<std::endl; 

      *flag = 0;
		  std::string downfile = DOWNPATH + "/"  +path; 
     //先得到发送的主机ip
      std::string host = _onlinepath[_onlineindex];
      //path是文件选择文件的名称
      //拼接上Range的头信息，
      std::string range = "bytes=" + std::to_string(begin) + "-" + std::to_string(end);
      std::string realpath = "/list/"+path;
      httplib::Headers head;
      head.insert(std::make_pair("Range",range.c_str()));

      //向服务端发送请求
      Client client(host.c_str(),9000);
      auto res = client.Get(realpath.c_str() , head);
      if(res && res->status == 200){
        //打开文件并且文件不存在的时候创建新文件
        if(!bf::exists(DOWNPATH)){
          //此时新建存储下载文件目录
          bf::create_directory(DOWNPATH);
        }
        //在写入文件的时候对文件加上写锁，防止在写入的时候别的线程也在写入或者读取
        
        //使用系统调用接口实现文件的写入
        int ret = lseek(fd, begin, SEEK_SET);
        if(ret < 0){
          std::cerr << "设置偏移量失败\n";
          return;
        }

        //写入数据
        ret = write(fd, &res->body[0], res->body.size());
        if(ret < 0){
          std::cerr << "写入数据失败\n";
        }
        //这个文件已经下载完了就可以解锁了
        //std::fstream file(downfile, std::ios::binary | std::ios::out | std::ios::in);
        //if(!file.is_open()){
        //  std::cerr << "文件流打开失败\n";
        //  return;
        //}
        //设置偏移量
        //file.seekp(begin,std::ios::beg);
        //file.write(&res->body[0], res->body.size());
        //if(!file.good()){
        //  std::cerr << "文件流出错了\n";
        //  return;
        //}
        //file.close();
        *flag = 1;
        std::cerr << "片段下载成功\n";
        return;
      } 
      std::cerr << "下载出错了\n";
      return;
    }

    void GetDownload(){
      std::cout << "目前连接的ip:" << _onlinepath[_onlineindex]<<"    ";
      std::cout<<"主机文件列表：" << std::endl;
      for(int i = 0;i < _file_list.size(); i++){
        std::cout<<"["<< i << "]:" << _file_list[i] << std::endl;
      }
      std::cout<< "选择主机文件的编号：";
      std::fflush(stdout);
      int file_index = 0;
      std::cin >> file_index;
      std::string real_path = "/list/";
      Client client(_onlinepath[_onlineindex].c_str(),9000);
      real_path = real_path + _file_list[file_index];
      //先发送一个Head
      auto res = client.Head(real_path.c_str());
      if(!(res && res->status == 200)){
        std::cerr << "请求错误" << std::endl;
        return;
      }
      uint64_t fsize = atoi(res->get_header_value("Content-Length").c_str());
      //长度
      std::cout<<"文件长度fsize"<<fsize<<std::endl;

      uint64_t start = 0;
      uint64_t end = 0;
      uint64_t len = 0;
      int size = fsize / MAXFILE;
      std::vector<boost::thread>threadv;
      threadv.resize(size + 1);
      std::vector<int> flag;
      flag.resize(size + 1,1);
 	    std::string path = _file_list[file_index];     
      //Range:bytes=begin-ends;
		  std::string downfile = DOWNPATH + "/"  +path; 
      //打开文件，不在线程中打开，并且对文件进行加写锁，在下载的时候别人不能读也不能写，
      //在线程中进行数独的读写，传入fd即可
      int fd = open(downfile.c_str(),O_CREAT | O_WRONLY,0664);
      if(fd < 0){
        std::cerr << "文件打开失败\n";
      }
      //此时对文件加上读锁
      struct flock lock;
      lock.l_type = F_WRLCK;
      lock.l_whence = SEEK_SET;
      lock.l_start = 0;
      lock.l_len = 0;
      if(fcntl(fd, F_SETLKW, &lock) < 0){
        std::cerr << "别人正在操作此文件，请稍等\n";
      }
      for(uint64_t i = 0;i <= size; i++){
        start = i * MAXFILE;
        end = (i + 1) * MAXFILE - 1;       
        if(i == size){
          //表示,最后一个，如果能够求模最后一个位置不能为
          end = fsize - 1;
        }        
        //创建一个线程去实现传输。并且传递进path是文件的名称
        threadv[i] = (boost::thread(&P2pclient::thr_begin, this, path, start, end, &flag[i],fd));
      }
      for(int i = 0;i < threadv.size(); i++){
        threadv[i].join();
      }
      //此时就表示所有线程都结束了，解锁
      lock.l_type = F_UNLCK;
      if(fcntl(fd, F_SETLKW, &lock) < 0){
        std::cerr << "解锁失败\n";
      }
      close(fd);
      //此时判断所有的文件是不是都
      //下载文件失败
      for(int i = 0; i < size;i++){
        if(flag[i] == 0){
          std::cerr  << "下载失败\n";
          return;
        }
      }
      std::cerr << "下载文件成功\n";
      return;
    }
////////////////////////////////////////////////////////////////////////
  public:
    void Run(){
      int num = 0;
      std::cout << "=======================================\n";
      std::cout << "0.退出当前系统\n";
      std::cout << "1.搜索在线在主机序列,选择指定主机\n";
      std::cout << "2.查看制定主机下共享目录下的文件\n";
      std::cout << "3.选择主机对应文件进行下载\n";
      std::cout << "=======================================\n";
      std::cout << "请输入您的选择";
      std::cin >> num;
      switch(num){
        case 1:
          //将在线的主机序列打印出来并且供用户选择
          GetHostpath(); 
          GetOnlinepath();
          break;
          //选择主机序号，要得到我们的目录下所有的文件名
        case 2:
          GetFileList();
          break;
        case 3:
          GetDownload();
          break;
        case 0:
          std::cout << "你退出了系统\n";
          exit(0);
        default:
          std::cout << "操作失败，请重新选择";
          break;
      }
      
      std::cout<<std::endl<<std::endl;
    }
};

int main(){

  P2pclient c;
  while(1){
    c.Run();
  }
  return 0;
}
