//stringstream 的使用
//将一个字符串变成多个字符串
#include<iostream>
#include<sstream>
#include<string>

int main(){
  std::string str = "GET /x/z HTTP/1.1\n";
  std::stringstream ss(str);

  std::string method,uri,version;
  ss >> method >> uri >> version;
  std::cout << method << std::endl;
  std::cout << uri << std::endl;
  std::cout << version << std::endl;
  return 0;
}


