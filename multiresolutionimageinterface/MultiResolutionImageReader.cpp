#include "MultiResolutionImageReader.h"
#include "MultiResolutionImage.h"
#include "core/filetools.h"

#include "MultiResolutionImageFactory.h"

using std::string;
//��ֱ���ͼ���Ķ���
MultiResolutionImageReader::MultiResolutionImageReader()
{
}

MultiResolutionImageReader::~MultiResolutionImageReader() {
}
  //�򿪶�ֱ���ͼ��
MultiResolutionImage* MultiResolutionImageReader::open(const std::string& fileName, const std::string factoryName) { 
	//����һ��MultiResolutionImage��
  return MultiResolutionImageFactory::openImage(fileName, factoryName);
}