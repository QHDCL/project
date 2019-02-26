#include <iostream>
#include <string>

int main(){
  std::string s = "XXX: YYY";
  std::size_t pos =s.find(": ");
  std::cout << pos << std::endl;

  std::string k = s.substr(0,pos);
  std::string v = s.substr(pos+2);

  std::cout<< k <<"->"<<v<<std::endl;
  return 0;
}
