#include<iostream>
#include<string>
//查找字符串中的字符位置


int main(){
  std::string str = "/a/a/c?html";
  std::size_t pos = str.find('?');
  std::cout << pos << std::endl;
  std::cout << std::string::npos << std::endl;
  return 0;
}
