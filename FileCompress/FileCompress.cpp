#define _CRT_SECURE_NO_WARNINGS
#include "FileCompress.h"
#include <iostream>
#include <assert.h>
using namespace std;

FileCompress::FileCompress(){
	_fileInfo.resize(256);    //   0 1	�ֽ�  
	for (size_t i = 0; i < 256; i++){
		_fileInfo[i]._ch = i;
		_fileInfo[i]._count = 0;
	}
}

//ѹ��
void FileCompress::CompressFile(const std::string& strFilePath){//Դ�ļ�·��
	//1.ͳ��Դ�ļ���ÿ���ַ����ֵĴ���
	FILE* fIn = fopen(strFilePath.c_str(), "rb");
	if (nullptr == fIn){
		cout << "���ļ�ʧ��" << endl;
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
	//2.��ÿ���ַ����ֵ� ������ΪȨֵ����Huffman��
	HuffmanTree<CharInfo> ht;
	ht.CreatHuffmanTree(_fileInfo,CharInfo(0));
	
	//3.ͨ��Huffman����ȡÿ���ַ��ı���
	GetHuffmanCode(ht.GetRoot());

	//4.�û�ȡ����Huffman�ȱ�������д�ļ�--ѹ��
	char ch = 0;
	char bitCount = 0;
	FILE* fOut = fopen("1.MyZip", "wb"); //
	assert(fOut);

	WriteHeaderInfo(fOut, strFilePath);
	//��fIn������ʼλ��
	fseek(fIn,0,SEEK_SET);
	while (true){
		size_t rdSize = fread(pReadBuff, 1, 1024, fIn);
		if (rdSize == 0)
			break;
		//�ñ����дԴ�ļ�
		for (size_t i = 0; i < rdSize; i++){
			string& strCode = _fileInfo[pReadBuff[i]].strCode;
			//A-->100
			//ch = 0000 0000
			//��ÿ���ַ��ı������ch��
			for (size_t j = 0; j < strCode.size(); j++){
				ch <<= 1;
				if ('1' == strCode[j])
					ch |= 1;

				bitCount++;
				if (8 == bitCount){
					//chд�뵽ѹ���ļ���
					fputc(ch, fOut);
					ch = 0;
					bitCount = 0;
				}
			}
		}
	}
	//���һ��ch������λû�������
	if (bitCount > 0 && bitCount < 8){
		ch <<= (8 - bitCount);
		fputc(ch, fOut);
	}

	delete[]  pReadBuff;
	fclose(fIn);
	fclose(fOut);
}

//��ѹ
void FileCompress::UNCompressFile(const std::string& strFilePath){
	//�ж��ļ���ʽ---�Ƿ��ܽ�ѹ��
	string posFix = strFilePath.substr(strFilePath.rfind('.') + 1);
	if (posFix != "MyZip"){
		cout << "ѹ���ļ���ʽ����!" << endl;
		return;
	}
	FILE* fIn = fopen(strFilePath.c_str(), "rb");
	if (fIn == nullptr){
		cout << "���ļ�ʧ��!" << endl;
		return;
	}

	//��ѹ���ļ��л�ȡԴ�ļ��ĺ�׺
	posFix = "";
	GetLine(fIn, posFix);

	//��ѹ���ļ��л�ȡ�ַ�������Ϣ
	string strContent;
	GetLine(fIn, strContent);
	size_t lineCount = atoi(strContent.c_str());
	
	size_t charCount = 0;
	for (size_t i = 0; i < lineCount; i++){
		strContent = "";
		GetLine(fIn, strContent);
		//������
		if ("" == strContent){
			strContent += "\n";
			GetLine(fIn, strContent);
		}

		charCount = atoi(strContent.c_str() + 2);
		_fileInfo[(unsigned char)strContent[0]]._count = charCount ;
	}

	//��ԭhuffman��
	HuffmanTree<CharInfo> ht;
	ht.CreatHuffmanTree(_fileInfo, CharInfo(0));

	//��ѹ��
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
			break;   //��������while
		}
		//��ȡ�ֽ�
		for (size_t i = 0; i < rdSize; i++){
			//��ѹ����ǰ�ֽڵ�ѹ������
			//0x96--->1001 0110  1000 0000
			//        1001 0110  0100 0000
			for (size_t j = 0; j < 8; j++){
				if (pReadBuff[i] & (1 << pos))
					pCur = pCur->_pRight;
				else
					pCur = pCur->_pLeft;
				pos--;   //pos = -1 ��ǰ8������λ������
				
				//Ҷ�ӽ��ʱд��
				if (nullptr == pCur->_pLeft && nullptr == pCur->_pRight){
					fputc(pCur->_weight._ch, fOut);  
					pCur = ht.GetRoot();
					--fileSize;
					if (0 == fileSize)
						break;
				}
				//ȡ��һ���ֽ�
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
	//Դ�ļ���׺
	string postFix = strFileName.substr(strFileName.rfind('.'));
	//��Ч����ĺ���

	//��Ч�ַ��Լ����ֵĴ���
	string strCharCount;
	size_t lineCount = 0;
	char szCount[32] = { 0 };
	for (size_t i = 0; i < 256; i++){
		if (0 != _fileInfo[i]._count){
			strCharCount += _fileInfo[i]._ch;
			strCharCount += ',';
			memset(szCount, 0, 32);
			//_itao-->int->char ��������:Ŀ��,����Ŀ��,���õĽ���
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

//��ȡ�ļ���һ����Ϣ
void FileCompress::GetLine(FILE* pf, string& strCotent){
	while (!feof(pf)){    //�ж��ļ�ָ���Ƿ����ļ�ĩβ
		char ch = fgetc(pf);
		if ('\n' == ch){
			return;
		}
		strCotent += ch;
	}
}
