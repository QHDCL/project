#include <unordered_map>
#include <iostream>

int main(){
  std::string k1 = "hello";
  int v1 = 100;
  std::string k2 = "he";
  int v2 = 200;
  std::string k3 = "h";
  int v3 = 300;
  std::string k4 = "hllo";
  int v4 = 400;

  std::unordered_map<std::string, int> kv;
  kv.insert({k1, v1});
  kv.insert({k2, v2});
  kv.insert({k3, v3});
  kv.insert({k4, v4});
  
  for(auto it = kv.begin();it != kv.end();it++){
    std::cout<< it->first<<" : "<<it->second<<std::endl;
  }
  return 0;
}
