#include "MultiResolutionImageReader.h"
#include "MultiResolutionImage.h"
#include "core/filetools.h"

#include "MultiResolutionImageFactory.h"

using std::string;
//¶à·Ö±æÂÊÍ¼ÏñÔÄ¶ÁÆ÷
MultiResolutionImageReader::MultiResolutionImageReader()
{
}

MultiResolutionImageReader::~MultiResolutionImageReader() {
}

MultiResolutionImage* MultiResolutionImageReader::open(const std::string& fileName, const std::string factoryName) { 
  return MultiResolutionImageFactory::openImage(fileName, factoryName);
}