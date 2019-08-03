#include <iostream>
#include <string>
#include <vector>
#include <sstream>
<<<<<<< HEAD
#include <fstream>
=======
>>>>>>> b0556dbd4a94dd97c38b78dc807a28b9801c170c
#include <boost/filesystem.hpp>
#include "httplib.h"
using namespace httplib;
namespace bf = boost::filesystem;
#define LIST_PATH "./list/"
class P2pserver{
private:
   Server _server;
public:
  //定义三个请求函数指针
  //附近主机配对请求处理
  static void GetHostPair(const Request& req,Response& res){
    //如果服务器开着就发送一个200表示能够进行匹配
    res.status = 200;
    res.set_header("Content-Length","0");
    return; 
  }
  //文件列表请求处理
  static void GetFileList(const Request& req,Response& res){
    std::string readpath = LIST_PATH;
    if(!bf::exists(readpath)){
      //如果目录不存在的话就直接创建一个新的realpath文件
      bf::create_directory(readpath);
    }
    std::vector<std::string> list;
    bf::directory_iterator it_begin(readpath);
    bf::directory_iterator it_end;
    //通过迭代器去访问目录下面的文件
    for(;it_begin != it_end; it_begin++){
      if(bf::is_directory(it_begin->status())){
        continue;
      }
    //不是目录的时候将文件添加到list中去
      std::string name = it_begin->path().filename().string();
      list.push_back(name); 
    }
    std::string body;
    for(auto& e:list){
		std::cout<<e<<std::endl;
      body += e + "\n";
    }
    //将数据发送过去
    std::cout<<body<<std::endl;
    res.body = body;
    res.set_header("Content-Type","text/html");
    res.set_header("Content-Length",std::to_string(body.size()).c_str());
    res.status = 200;
    return;
  }

  //获得range头部信息的数据bytes=start-end;
  static void getDataRange(std::string& range, int64_t& start, int64_t& end){
    //查找=的位置
    int pos1 = range.find("=");
    //找到-的位置
    int pos2 = range.find("-");
    
    start = atoi(range.substr(pos1+1,pos2-pos1-1).c_str());
    end = atoi(range.substr(pos2+1).c_str());
  }
  //文件下载请求处理
  static void GetFileData(const Request& req,Response& res){
    //下载文件。就是将文件中的内容传送给我们的客户端指定的
    std::string realpath = "." + req.path;
    //此时realpath就是我们要下载的文件
    if(!bf::exists(realpath)){
      //此时文件不存在的时候
      res.status = 404;
      return;
    }
    //获取文件的大小
<<<<<<< HEAD
    
=======
>>>>>>> b0556dbd4a94dd97c38b78dc807a28b9801c170c
    int64_t fsize = bf::file_size(realpath);
    std::cout<< "文件的大小" << fsize << std::endl;
    if(req.method == "HEAD"){
      //如果是HEAD方法的话，就只传递给一个文件的大小
      res.set_header("Content-Length",std::to_string(fsize).c_str());
      res.status = 200;
      return;
    }
    //如果不是HEAD请求，要传送数据
<<<<<<< HEAD
    //std::ifstream file(realpath, std::ios::binary);
    int fd = open(realpath.c_str(), O_RDONLY);
    if(fd < 0){
      std::cerr << "文件打开失败或者不存在此文件" << std::endl;
      return;
    }
    struct flock lock;

    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    if(fcntl(fd, F_SETLKW, &lock) < 0){
      std::cerr << "加读锁失败\n";
    }
    //直接将所有的数据都发送到client中
   //  if(!file.is_open()){
      //此时打开文件失败，服务端错误，返回值500
   //    res.status = 500;
    //      return;
   // }
=======
    std::ifstream file(realpath,std::ios::binary);
    
    //直接将所有的数据都发送到client中
    if(!file.is_open()){
      //此时打开文件失败，服务端错误，返回值500
        res.status = 500;
        return;
    }
>>>>>>> b0556dbd4a94dd97c38b78dc807a28b9801c170c
    //read：将流中的数据提取到数组中,提取长度是fsize
    //提取头信息中Range的信息，将长度提取出来。bytes=start-end;
    int64_t start = 0;
    int64_t end = 0;
    std::string range;
    range = req.get_header_value("Range");
    std::cout<<range<<std::endl; 
    getDataRange(range,start,end);
    std::cout<<"begin:"<<start <<"end:"<<end<<std::endl;
    int64_t  lsize = end - start + 1;
    std::cout<<"段大小"<<std::endl;
<<<<<<< HEAD
   //设置偏移量
    int ret = lseek(fd, start, SEEK_SET);
    if(ret < 0){
      std::cerr << "偏移量设置失败\n";
      return;
    }

    //向body中写入数据
    res.body.resize(lsize);
    ret = read(fd, &res.body[0], res.body.size());
    if(ret < 0){
      std::cerr << "文件读取失败\n";
      return;
    }
    lock.l_type = F_UNLCK;
    // file.seekg(start,std::ios::beg);
  //  file.read(&res.body[0],lsize);
    //判断此次之后流是不是好的
//    if(!file.good()){
//      res.status = 500;
//      return;
 //   }
  //  file.close();
    //传输body,设置application/octet-stream表示就是下载文件
    
    //对读锁解锁
    if(fcntl(fd, F_SETLKW, &lock) < 0){
      std::cerr << "解读锁失败\n";
      return ;
    }

    //关闭文件描述符
    close(fd);
=======
    res.body.resize(lsize);
    file.seekg(start,std::ios::beg);
    file.read(&res.body[0],lsize);
    //判断此次之后流是不是好的
    if(!file.good()){
      res.status = 500;
      return;
    }
    file.close();
    //传输body,设置application/octet-stream表示就是下载文件
>>>>>>> b0556dbd4a94dd97c38b78dc807a28b9801c170c
    res.set_header("Content-Type","application/octet-stream");
    res.status = 200;

    return;
  }
public:
  void Run(){
    if(!bf::exists(LIST_PATH)){
       bf::create_directory(LIST_PATH);
     }
  
    _server.Get("/",GetHostPair);
    _server.Get("/list",GetFileList);
    _server.Get("/list/(.*)",GetFileData);
<<<<<<< HEAD
    _server.listen("192.168.244.143",9000);
=======
    _server.listen("192.168.244.142",9000);
>>>>>>> b0556dbd4a94dd97c38b78dc807a28b9801c170c
  }
};

int main(){
  
  P2pserver ser;
  ser.Run();

  return 0;
}
