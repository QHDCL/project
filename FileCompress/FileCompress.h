#pragma once
#include <string>
#include <vector>
#include "huffman.hpp"



//存储字符信息---编码对应字符
struct CharInfo{
	CharInfo(unsigned long long count = 0)
		:_count(count)
	{}
	unsigned char _ch;   //字符
	unsigned long long _count;  //字符出现的次数
	std::string  strCode;   //字符对应的编码

	CharInfo operator+(const CharInfo& info){
		return CharInfo(_count + info._count);
	}

	bool operator>(const CharInfo& info){
		return _count > info._count;
	}

	bool operator!=(const CharInfo& info)const{
		return _count != info._count;
	}
};

//文件压缩与解压缩
class FileCompress{
public:
	FileCompress();
	void CompressFile(const std::string& strFilePath);//源文件路径
	void UNCompressFile(const std::string& strFilePath);  
private:
	void GetHuffmanCode(HTNode<CharInfo>* pRoot);
	void WriteHeaderInfo(FILE* pf,const std::string& strFileName);
	void GetLine(FILE* pf, std::string& strContent);
private:
	std::vector<CharInfo> _fileInfo;
};