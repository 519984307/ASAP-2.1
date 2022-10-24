#include "MultiResolutionImageReader.h"
#include "MultiResolutionImage.h"
#include "core/filetools.h"

#include "MultiResolutionImageFactory.h"

using std::string;
//多分辨率图像阅读器
MultiResolutionImageReader::MultiResolutionImageReader()
{
}

MultiResolutionImageReader::~MultiResolutionImageReader() {
}
  //打开多分辨率图像
MultiResolutionImage* MultiResolutionImageReader::open(const std::string& fileName, const std::string factoryName) { 
	//返回一个MultiResolutionImage类
  return MultiResolutionImageFactory::openImage(fileName, factoryName);
}