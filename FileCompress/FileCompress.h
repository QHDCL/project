#pragma once
#include <string>
#include <vector>
#include "huffman.hpp"



//�洢�ַ���Ϣ---�����Ӧ�ַ�
struct CharInfo{
	CharInfo(unsigned long long count = 0)
		:_count(count)
	{}
	unsigned char _ch;   //�ַ�
	unsigned long long _count;  //�ַ����ֵĴ���
	std::string  strCode;   //�ַ���Ӧ�ı���

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

//�ļ�ѹ�����ѹ��
class FileCompress{
public:
	FileCompress();
	void CompressFile(const std::string& strFilePath);//Դ�ļ�·��
	void UNCompressFile(const std::string& strFilePath);  
private:
	void GetHuffmanCode(HTNode<CharInfo>* pRoot);
	void WriteHeaderInfo(FILE* pf,const std::string& strFileName);
	void GetLine(FILE* pf, std::string& strContent);
private:
	std::vector<CharInfo> _fileInfo;
};