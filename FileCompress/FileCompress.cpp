#define _CRT_SECURE_NO_WARNINGS
#include "FileCompress.h"
#include <iostream>
#include <assert.h>
using namespace std;

FileCompress::FileCompress(){
	_fileInfo.resize(256);    //   0 1	字节  
	for (size_t i = 0; i < 256; i++){
		_fileInfo[i]._ch = i;
		_fileInfo[i]._count = 0;
	}
}

//压缩
void FileCompress::CompressFile(const std::string& strFilePath){//源文件路径
	//1.统计源文件中每个字符出现的次数
	FILE* fIn = fopen(strFilePath.c_str(), "rb");
	if (nullptr == fIn){
		cout << "打开文件失败" << endl;
		return;
	}

	unsigned char* pReadBuff = new unsigned char[1024];
	while(true){
		size_t rdSize = fread(pReadBuff, 1, 1024, fIn);
		if (0 == rdSize)
			break;
		for (size_t i = 0; i < rdSize; i++)
			_fileInfo[pReadBuff[i]]._count++;
	}
	//2.以每次字符出现的 次数作为权值创建Huffman树
	HuffmanTree<CharInfo> ht;
	ht.CreatHuffmanTree(_fileInfo,CharInfo(0));
	
	//3.通过Huffman树获取每个字符的编码
	GetHuffmanCode(ht.GetRoot());

	//4.用获取到的Huffman比编码来改写文件--压缩
	char ch = 0;
	char bitCount = 0;
	FILE* fOut = fopen("1.MyZip", "wb"); //
	assert(fOut);

	WriteHeaderInfo(fOut, strFilePath);
	//将fIn放置起始位置
	fseek(fIn,0,SEEK_SET);
	while (true){
		size_t rdSize = fread(pReadBuff, 1, 1024, fIn);
		if (rdSize == 0)
			break;
		//用编码改写源文件
		for (size_t i = 0; i < rdSize; i++){
			string& strCode = _fileInfo[pReadBuff[i]].strCode;
			//A-->100
			//ch = 0000 0000
			//将每个字符的编码放在ch中
			for (size_t j = 0; j < strCode.size(); j++){
				ch <<= 1;
				if ('1' == strCode[j])
					ch |= 1;

				bitCount++;
				if (8 == bitCount){
					//ch写入到压缩文件中
					fputc(ch, fOut);
					ch = 0;
					bitCount = 0;
				}
			}
		}
	}
	//最后一次ch个比特位没有填充满
	if (bitCount > 0 && bitCount < 8){
		ch <<= (8 - bitCount);
		fputc(ch, fOut);
	}

	delete[]  pReadBuff;
	fclose(fIn);
	fclose(fOut);
}

//解压
void FileCompress::UNCompressFile(const std::string& strFilePath){
	//判断文件格式---是否能解压缩
	string posFix = strFilePath.substr(strFilePath.rfind('.') + 1);
	if (posFix != "MyZip"){
		cout << "压缩文件格式错误!" << endl;
		return;
	}
	FILE* fIn = fopen(strFilePath.c_str(), "rb");
	if (fIn == nullptr){
		cout << "打开文件失败!" << endl;
		return;
	}

	//从压缩文件中获取源文件的后缀
	posFix = "";
	GetLine(fIn, posFix);

	//从压缩文件中获取字符编码信息
	string strContent;
	GetLine(fIn, strContent);
	size_t lineCount = atoi(strContent.c_str());
	
	size_t charCount = 0;
	for (size_t i = 0; i < lineCount; i++){
		strContent = "";
		GetLine(fIn, strContent);
		//处理换行
		if ("" == strContent){
			strContent += "\n";
			GetLine(fIn, strContent);
		}

		charCount = atoi(strContent.c_str() + 2);
		_fileInfo[(unsigned char)strContent[0]]._count = charCount ;
	}

	//还原huffman树
	HuffmanTree<CharInfo> ht;
	ht.CreatHuffmanTree(_fileInfo, CharInfo(0));

	//解压缩
	string strUNFileName("Unzip");
	strUNFileName += posFix;
	FILE* fOut = fopen(strUNFileName.c_str(), "wb");
	
	unsigned char* pReadBuff = new unsigned char[1024];
	int pos = 7;
	HTNode<CharInfo>* pCur = ht.GetRoot();
	long long fileSize = pCur->_weight._count;

	while (true){
		size_t rdSize = fread(pReadBuff, 1, 1024, fIn);
		if (rdSize == 0){
			break;   //用于跳出while
		}
		//获取字节
		for (size_t i = 0; i < rdSize; i++){
			//解压缩当前字节的压缩数据
			//0x96--->1001 0110  1000 0000
			//        1001 0110  0100 0000
			for (size_t j = 0; j < 8; j++){
				if (pReadBuff[i] & (1 << pos))
					pCur = pCur->_pRight;
				else
					pCur = pCur->_pLeft;
				pos--;   //pos = -1 当前8个比特位处理完
				
				//叶子结点时写入
				if (nullptr == pCur->_pLeft && nullptr == pCur->_pRight){
					fputc(pCur->_weight._ch, fOut);  
					pCur = ht.GetRoot();
					--fileSize;
					if (0 == fileSize)
						break;
				}
				//取下一个字节
				if (pos < 0){
					pos = 7;
					break;
				}
			}
		}
	}
	delete[] pReadBuff;
	fclose(fIn);
	fclose(fOut);
}
	

void FileCompress::GetHuffmanCode(HTNode<CharInfo>* pRoot){
	if (nullptr == pRoot)
		return;

	GetHuffmanCode(pRoot->_pLeft);
	GetHuffmanCode(pRoot->_pRight);

	if (nullptr==pRoot->_pLeft && nullptr==pRoot->_pRight){
		HTNode<CharInfo>* pCur = pRoot;
		HTNode<CharInfo>* pParent = pCur->_pParent;
		string& strCode =_fileInfo[pRoot->_weight._ch].strCode;

		while (pParent){
			if (pCur == pParent->_pLeft)
				strCode += '0';
			else
				strCode += '1';
			pCur = pParent;
			pParent = pCur->_pParent;
		}
		reverse(strCode.begin(), strCode.end());
	}
}

void FileCompress::WriteHeaderInfo(FILE* pf, const string& strFileName){
	//源文件后缀
	string postFix = strFileName.substr(strFileName.rfind('.'));
	//有效编码的函数

	//有效字符以及出现的次数
	string strCharCount;
	size_t lineCount = 0;
	char szCount[32] = { 0 };
	for (size_t i = 0; i < 256; i++){
		if (0 != _fileInfo[i]._count){
			strCharCount += _fileInfo[i]._ch;
			strCharCount += ',';
			memset(szCount, 0, 32);
			//_itao-->int->char 参数意义:目标,保存目标,所用的进制
			_itoa(_fileInfo[i]._count, szCount, 10);
			strCharCount += szCount;
			strCharCount += "\n";
			lineCount++;
		}
	}
	string strHeadInfo;
	strHeadInfo += postFix;
	strHeadInfo += "\n";

	memset(szCount, 0, 32);
	_itoa(lineCount, szCount, 10);
	strHeadInfo += szCount;
	strHeadInfo += "\n";

	strHeadInfo += strCharCount;

	fwrite(strHeadInfo.c_str(), 1, strHeadInfo.size(), pf);
}

//获取文件中一行信息
void FileCompress::GetLine(FILE* pf, string& strCotent){
	while (!feof(pf)){    //判断文件指针是否在文件末尾
		char ch = fgetc(pf);
		if ('\n' == ch){
			return;
		}
		strCotent += ch;
	}
}
