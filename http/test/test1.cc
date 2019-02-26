#include<iostream>
#include<string>
#include<algorithm>

int main(){
  std::string str = "GET /x/x";
  std::cout << str << std::endl;
  transform(str.begin(),str.end(),str.begin(),::toupper);//小写转大写
  std::cout << str << std::endl;
  return 0;
}
