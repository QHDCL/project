#ifndef __PROTACOLUTIL_HPP__
#define __PROTACOLUTIL_HPP__

#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <strings.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <unordered_map>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <sys/wait.h>

#define NORMAL 0
#define WARNING 1
#define FATAL 2

#define BUFF_NUM 1024
#define BACKLOG 5

#define WEBROOT "wwwroot"
#define HOMEPAGE "index.html"

const char *errorstr[] = {
		"Normal",
		"Warning",
		"Fatal Error"
};

void Log(std::string msg, int level, std::string file, int line){
		std::cout<< file << ": " << "[" <<line << "] "  << " : "<< msg << "  level : [" << errorstr[level] << "]"<< std::endl;
}

#define  LOG(msg, level) Log(msg, level, __FILE__, __LINE__)


class Util{
  public:
    //将字符串分成以k,v形式存在
    //xxxx: yyyy
    static void MakeKv(std::string s,std::string &k,std::string &v){
      std::size_t pos = s.find(": ");
      k = s.substr(0, pos);
      v = s.substr(pos+2);
    }

    //int类型转string类型
    static std::string IntToString(int &x){
      std::stringstream ss;
      ss << x;
      return ss.str();
    }

    //状态码-->状态码描述
    static std::string CodeToDesc(int code){
      switch(code){
        case 200:
          return "OK";
        case 404:
          return "Not Found";
        default:
          break;
      }
      return "Unknow";
    }
    //后缀映射为 媒体类型---这种做法不好,可map等
    static std::string SuffixToContent(std::string &suffix){
      if(suffix == ".css"){
        return "text/css";
      }
      if(suffix == ".js"){
        return "application/x-javascript";
      }
      if(suffix == ".html" || suffix == ".htm"){
        return "text/html";
      }
      if(suffix == ".jpg"){
        return "application/x-jpg";
      }
      return "text/html";
    }
};

class SocketApi{
	public:
		static int Socket(){
			int sock = socket(AF_INET, SOCK_STREAM, 0);
			if(sock < 0){
				LOG("Create Socket Failure", FATAL);
				exit(2);
			}
      int opt = 1;
      setsockopt(sock, SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
			return sock;
		}

		static void Bind(int sock, int port){
			struct sockaddr_in local;
			local.sin_family = AF_INET;
			local.sin_port = htons(port);
			local.sin_addr.s_addr = htonl(INADDR_ANY);

			if(bind(sock, (struct sockaddr*)&local, sizeof(local)) < 0){
				LOG("Bind Failure", FATAL);
				exit(3);
			}
		}

		static void Listen(int sock){
			if(listen(sock, BACKLOG) < 0){
				LOG("Listen Error", FATAL);
				exit(4);
			}
		}

    static int Accept(int listen_sock, std::string &ip, int &port){
      struct sockaddr_in peer;
      socklen_t len = sizeof(peer);
      int sock = accept(listen_sock,(struct sockaddr *)&peer,&len);
      if(sock < 0){
        LOG("accept error!",WARNING);
        return -1;
      }
      port = ntohs(peer.sin_port);
      ip = inet_ntoa(peer.sin_addr);
      return sock;
    }
};

class Http_Response{
  public:
    //基本协议字段
    std::string status_line;  //响应行
    std::vector<std::string> response_header; //响应报头
    std::string blank;         //空行
    std::string response_text;  //正文
  private:
    int code;  //状态码
    std::string path; //路径
    int recource_size;
  public:
    Http_Response():blank("\r\n"),code(200),recource_size(0)
    {}

    int &Code(){
      return code;
    }

    void SetPath(std::string &path_){
      path = path_;
    }

    std::string &Path(){
      return path; 
    }
    void SetRecourSize(int rs){
      recource_size = rs;
    }
    int RecourceSize(){
      return recource_size;
    }

    //响应行
    void MakeStatusLine(){
      status_line = "HTTP/1.0 "; //协议
      status_line += " ";
      status_line += Util::IntToString(code);//状态码
      status_line += " ";
      status_line += Util::CodeToDesc(code); //状态码描述
      status_line += "\r\n";
      LOG("Make Status Line Done!",NORMAL);
    }
    
    //响应报头
    void MakeResponseHeader(){
      std::string line;
      std::string suffix;  //后缀
      //构建content-type   媒体类型
      line = "Connect-Type: ";
      std::size_t pos = path.rfind('.');
      //找到后缀
      if(std::string::npos != pos){
          suffix = path.substr(pos);
          transform(suffix.begin(),suffix.end(),suffix.begin(), ::tolower);  //请求方法全都转为大写
          line += Util::SuffixToContent(suffix);
          line += "\r\n";
          response_header.push_back(line);
      }
      //构建content-length 正文长度
      line = "Conten-Length: ";
      line += Util::IntToString(recource_size);
      line += "\r\n";
      response_header.push_back(line);

      line = "\r\n";
      response_header.push_back(line);
      LOG("Make Response Header Done!",NORMAL);
    }
    ~Http_Response()
    {}
};

class Http_Request{
  public:
    //基本协议字段
    std::string request_line;  //请求行
    std::vector<std::string> request_header; //请求报头
    std::string blank;         //空行
    std::string request_text;  //正文
  private:
    //解析字段
    std::string method;  //请求方法
    std::string uri;     //path(资源路径)?arg(参数)
    std::string version; //http协议版本
//    int recource_size  //
    std::string path;
    std::string query_string;
    std::unordered_map <std::string,std::string>header_kv;
    bool cgi;
  public:
    Http_Request():path(WEBROOT),cgi(false),blank("\r\n")
    {}

    //解析请求行信息
    void RequestLineParse(){
      //将一个字符转化为三个
      //GET /x/y HTTP/1.1\n
      std::stringstream ss(request_line);
      ss >> method >> uri >> version;
      transform(method.begin(),method.end(),method.begin(), ::toupper);  //请求方法全都转为大写
    }

    void Uriparse(){
      if(method == "GET"){
        std::size_t pos = uri.find('?');
        //找?的位置  == 则没有?
        if(pos != std::string::npos){
          cgi = true;
          path += uri.substr(0, pos);//substr:第二个参数是长度
          query_string = uri.substr(pos+1);
        }
        else{
          path += uri;   //注意目录不是根目录,而是外部根目录.拼接wwwroot说明;  如果拼接出来是wwwroot/-->首页
        }
      }
      else{
        cgi = true;
        path += uri;        //wwwroot/
      }
      if(path[path.size()-1] == '/'){
        path += HOMEPAGE;   //wwwroot/index
      }
    }

    //解析报文头部信息
    void HeaderParse(){
      std::string k,v;
      for(auto it = request_header.begin();it != request_header.end();it++){
        Util::MakeKv(*it, k, v);
        header_kv.insert({k,v});
      }
    }

    //请求方式是否合法---只支持GET和POST
    bool IsMethodLegal(){
      //大小写问题:
      if(method != "GET" && method != "POST"){
        return false;
      }
      return true;
    }
    //判断路径是否合法
    int IsPathLegal(Http_Response *rsp){ //wwwroot/a/b/c.html
      //linux下文件分为两部分---内容和属性(权限,大小,创建时间等)
      //stat 定位文件
      int rs = 0;
      struct stat st;
      if(stat(path.c_str(),&st) < 0){
        std::cout << path <<std::endl;
        

        LOG("file is not exist!",WARNING);
        return 404;
      }
      //非cgi就是一个简单的请求.cgi就是去执行可执行程序
      else{
        //判断是否是目录,每个目录都应该有一个首页,
        rs = st.st_size;
        if(S_ISDIR(st.st_mode)){ 
          //是目录
          path += "/";
          path += HOMEPAGE;
          stat(path.c_str(),&st);
          rs = st.st_size;
        }else if((st.st_mode & S_IXUSR) ||\
                 (st.st_mode & S_IXGRP) ||\
                 (st.st_mode & S_IXOTH)){ //判断是否是可执行程序--权限中有一个是X即表明是可执行程序
          cgi = true;
        }else{
            //
        }
      }
      rsp->SetPath(path);
      rsp->SetRecourSize(rs);
      LOG("Path is OK!",NORMAL);
      return 0;
    }

    //以请求方式判断读取
    bool IsNeedRecv(){
      return method == "POST"? true : false;
    }

    bool IsCgi(){
      return cgi;
    }

    //获取conten-length 正文长度
    int ContenLength(){
      int content_length = -1;
      std::string cl = header_kv["Conten-Length"];
      //将string类型转换为int类型
      std::stringstream ss(cl);
      ss >> content_length;
      return content_length;
    }

    std::string GetParam(){
      if(method == "GET"){
        return query_string;
      }
      else{
        return request_text;
      }
    }

    ~Http_Request()
    {}
};

class Connect{
  private:
    int sock;
  public:
    Connect(int sock_):sock(sock_)
    {}
    
    //读取一行
    int RecvOneLine(std::string &line_){
      char buff[BUFF_NUM];
      int i = 0;
      char c = 'C';
      while(c != '\n' && i < BUFF_NUM - 1){
        ssize_t s = recv(sock, &c, 1, 0);
        if(s>0){
          if(c == '\r'){
           // \r \r\n \n ->\n
            recv(sock, &c, 1, MSG_PEEK);
            if(c == '\n'){
              recv(sock, &c, 1, 0);
            }
            else{
              c == '\n';
            }
           }
           buff[i++] = c;
         }
         else{
           break;
         }
       }
       buff[i] = 0;
       line_ = buff;
       return i;
    }

    //读取报头
    void RecvRequestHeader(std::vector<std::string> &v){
      std::string line = "X";
      while(line != "\n"){
        RecvOneLine(line);
        if(line != "\n"){
          v.push_back(line);
        }
      }
      LOG("Header Recv is OK",NORMAL);
    }

    //读取正文-->报文头中的Content-Length
    void RecvText(std::string &text,int  content_length){
      char c;
      for(auto i=0;i<content_length;i++){
        recv(sock,&c,1,0);
        //将读取的字符放入text中
        text.push_back(c);
      }
    }
    //发送
    void SendStatusLine(Http_Response *rsp){
      std::string &sl = rsp->status_line;
      send(sock,sl.c_str(),sl.size(),0);
    } 
    void SendHeader(Http_Response *rsp){
      std::vector<std::string> &v = rsp->response_header;
      for(auto it=v.begin();it != v.end();it++){
        send(sock,it->c_str(),it->size(),0);
      }
    } 
    void SendText(Http_Response *rsp,bool cgi_){
      if(!cgi_){  
        std::string &path = rsp->Path();
        int fd = open(path.c_str(),O_RDONLY); //应判断打开成功性
        if(fd < 0){
          LOG("Open file error!",WARNING);
          return;
        }
        sendfile(sock,fd,NULL,rsp->RecourceSize());  //参数:谁接,谁发
        close(fd);
      }else{
        std::string &rsp_text = rsp->response_text;
        send(sock, rsp_text.c_str(), rsp_text.size(), 0);
      }
    }
    ~Connect(){
      close(sock);
    }
};


class Entry{
  public:
    static int ProcessCgi(Connect *conn,Http_Request *rq,Http_Response *rsp){
      int input[2];
      int output[2];
      pipe(input);
      pipe(output);

      std::string bin = rsp->Path(); //wwwrot/a/
      std::string param = rq->GetParam();
      int size = param.size();
      std::string param_size = "CONTENT-LENGTH=";
      param_size += Util::IntToString(size);
      std::string &response_text = rsp->response_text;

      pid_t id = fork();
      if(id < 0){
        LOG("fork error!",WARNING);
        return 503; //服务器错误
      }
      else if(id == 0){  //child
        close(input[1]);   //写端
        close(output[0]);  //读端
        putenv((char *)param_size.c_str());
        dup2(input[0], 0);
        dup2(output[1], 1);
        //exec*  
        execl(bin.c_str(),bin.c_str(),NULL);
        //exec 失败就不会替换,执行下步,成功替换,不用判断返回值
        exit(1);
      }
      else{          //father
        close(input[0]);
        close(output[1]);

        char c;
        for(auto i = 0;i < size; i++){
          c = param[i];
          write(input[1], &c, 1);
        }
        waitpid(id, NULL, 0);
        while(read(output[0], &c, 1) > 0 ){
          response_text.push_back(c);
        } 

        rsp->MakeStatusLine();
        rsp->SetRecourSize(response_text.size());
        rsp->MakeResponseHeader();


        conn->SendStatusLine(rsp); //发送
        conn->SendHeader(rsp); 
        conn->SendText(rsp, true);
      }
    return 200;
    }
    static int ProcessNonCgi(Connect *conn,Http_Request *rq,Http_Response *rsp){
      rsp->MakeStatusLine();  //构建响应行
      rsp->MakeResponseHeader(); //构建响应头
      //rsp->MakeRespinseText(rq); //构建响应正文

      conn->SendStatusLine(rsp); //发送
      conn->SendHeader(rsp); //add \n-->添加空格
      conn->SendText(rsp, false);
      LOG("Send Response Done!",NORMAL);
    }
    static int ProcessPesponse(Connect *conn,Http_Request *rq,Http_Response *rsp){
      if(rq->IsCgi()){
        //传参  GET/POST  ?/正文
      LOG("MakeResponse Use Cgi",NORMAL);
      ProcessCgi(conn, rq, rsp);
      }
      else{
        LOG("MakeRespone Use Non Cgi!",NORMAL);
        ProcessNonCgi(conn, rq, rsp);
      }
    }

    static void HandlerRequest(int sock){
      pthread_detach(pthread_self());
      //int *sock = (int*)arg;
#ifdef _DEBUG_
      //for test
      char buff[10240];
      read(sock,buff,sizeof(buff));
      std::cout << "##################"<< std::endl;
      std::cout << buff << std::endl;
      std::cout << "##################"<< std::endl;      
#else
      // 链接类---网络通信
      Connect *conn = new Connect(sock);
      // 请求类
      Http_Request *rq = new Http_Request(); 
      // 响应类
      Http_Response *rsp = new Http_Response();
      int code = rsp->Code(); 

      //读取请求行
      conn->RecvOneLine(rq->request_line);
      rq->RequestLineParse();

      //方法非法
      if(!rq->IsMethodLegal()){
        LOG("Request Method Is Not Legal",WARNING);
        goto end;
      }

      rq->Uriparse();
      //路径非法
      if(rq->IsPathLegal(rsp) != 0){
        code = 404;
        LOG("file is not exist!",WARNING);
        goto end;
      }
       
      conn->RecvRequestHeader(rq->request_header);

      rq->HeaderParse();
      //是否继续读取(正文)-->是
      if(rq->IsNeedRecv()){
        LOG("POST Method,Need Recv Begin!",NORMAL);
        conn->RecvText(rq->request_text,rq->ContenLength());
        LOG("Http Request Recv Done,OK!",NORMAL);
      }
       
      //处理响应
      ProcessPesponse(conn,rq,rsp);

end:
      delete conn;
      delete rq;
      delete rsp;
#endif
    }     
};
#endif // __PROTACOLUTIL_HPP__
