#include "FileCompress.h"

int main(){
	FileCompress fc;
	fc.CompressFile("新建文本文档.txt");
	fc.UNCompressFile("1.MyZip");
	return 0;
}