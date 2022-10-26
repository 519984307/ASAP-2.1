#ifndef _MultiResolutionImage
#define _MultiResolutionImage
#include <string>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include "multiresolutionimageinterface_export.h"
#include "TileCache.h"
#include "core/PathologyEnums.h"
#include "core/ImageSource.h"
#include "core/Patch.h"

class MULTIRESOLUTIONIMAGEINTERFACE_EXPORT MultiResolutionImage : public ImageSource {

public :
    //构造函数
  MultiResolutionImage();
    //析构函数
  virtual ~MultiResolutionImage();
  ImageSource* clone();

  //加载图像，返回是否获得有效的图像
  bool initialize(const std::string& imagePath);

  //实际的初始化实现
  virtual bool initializeType(const std::string& imagePath) = 0;

  //支持具有多个z平面的幻灯片
  int getNumberOfZPlanes() const;
  void setCurrentZPlaneIndex(const unsigned int& zPlaneIndex);
  unsigned int getCurrentZPlaneIndex() const;

  //获取存储的数据属性(例如目标放大)
  virtual std::string getProperty(const std::string& propertyName) { return std::string(); };

  //获取/设置缓存的最大值
  virtual const unsigned long long getCacheSize();
  virtual void setCacheSize(const unsigned long long cacheSize);

  //! Gets the number of levels in the slide pyramid
  virtual const int getNumberOfLevels() const;
  
  //! 获取幻灯片金字塔中的层数
  virtual const std::vector<unsigned long long> getDimensions() const;

  //! 获取金字塔的指定级别的尺寸
  virtual const std::vector<unsigned long long> getLevelDimensions(const unsigned int& level) const;
  
  //得到给定水平相对于基本水平的下采样因子
  virtual const double getLevelDownsample(const unsigned int& level) const;

  //在给定请求的下采样因子的情况下，获取与最近的下采样因子对应的级别
  virtual const int getBestLevelForDownSample(const double& downsample) const;

  //获取通道的最小值。如果没有指定通道，默认为第一个通道
  virtual double getMinValue(int channel = -1) = 0;
  
  //获取通道的最大值。如果没有指定通道，默认为第一个通道
  virtual double getMaxValue(int channel = -1) = 0;

  //获取打开图像的文件类型
  const std::string getFileType() const;
  
  // 获取作为补丁的数据，这是一个基本的图像类，包含所有相关的信息供进一步处理，
  // 比如数据和颜色类型
  template <typename T> 
  Patch<T> getPatch(const long long& startX, const long long& startY, const unsigned long long& width,
    const unsigned long long& height, const unsigned int& level) 
  {
    std::vector<unsigned long long> dims(3,0);
    dims[0] = width;
    dims[1] = height;
    dims[2] = _samplesPerPixel;
    T* data = new T[width*height*_samplesPerPixel];
    getRawRegion<T>(startX, startY, width, height, level, data);
    std::vector<double> patchSpacing(_spacing.size(), 1.0);
    double levelDownsample = this->getLevelDownsample(level);
    for (unsigned int i = 0; i < _spacing.size(); ++i) {
      patchSpacing[i] = _spacing[i] * levelDownsample;
    }
    std::vector<double> minValues, maxValues;
    for (unsigned int i = 0; i < this->getSamplesPerPixel(); ++i) {
      minValues.push_back(this->getMinValue(i));
      maxValues.push_back(this->getMaxValue(i));
    }
    Patch<T> patch = Patch<T>(dims, this->getColorType(), data, true, minValues, maxValues);
    patch.setSpacing(patchSpacing);
    return patch;
  }

  //获取请求区域的像素数据。用户负责分配足够的内存来容纳数据数组并清除内存。
  //请注意，对于int32 ARGB数据，就像在OpenSlide中，颜色的顺序取决于你的机器的端序(Windows典型的BGRA)
  template <typename T> 
  void getRawRegion(const long long& startX, const long long& startY, const unsigned long long& width, 
    const unsigned long long& height, const unsigned int& level, T*& data) {
      if (level >= getNumberOfLevels()) {
        return;
      }
      unsigned int nrSamples = getSamplesPerPixel();
      if (this->getDataType()==pathology::DataType::Float) {
        float * temp = (float*)readDataFromImage(startX, startY, width, height, level);
        std::copy(temp, temp + width*height*nrSamples, data);
        delete[] temp;
      }
      else if (this->getDataType()==pathology::DataType::UChar) {
        unsigned char * temp = (unsigned char*)readDataFromImage(startX, startY, width, height, level);
        std::copy(temp, temp + width*height*nrSamples, data);
        delete[] temp;
      }
      else if (this->getDataType()==pathology::DataType::UInt16) {
        unsigned short * temp = (unsigned short*)readDataFromImage(startX, startY, width, height, level);
        std::copy(temp, temp + width*height*nrSamples, data);
        delete[] temp;
      }
      else if (this->getDataType()==pathology::DataType::UInt32) {
        unsigned int * temp = (unsigned int*)readDataFromImage(startX, startY, width, height, level);
        std::copy(temp, temp + width*height*nrSamples, data);
        delete[] temp;
      }
    }

protected :

  //! To make MultiResolutionImage thread-safe  使MultiResolutionImage线程安全
  std::unique_ptr<std::shared_mutex> _openCloseMutex;
  std::unique_ptr<std::mutex> _cacheMutex;
  std::shared_ptr<void> _cache;

  // 多分辨率图像的附加属性
  std::vector<std::vector<unsigned long long> > _levelDimensions;
  unsigned int _numberOfLevels;
  unsigned int _numberOfZPlanes;
  unsigned int _currentZPlaneIndex;

  // 已加载幻灯片的属性
  unsigned long long _cacheSize;
  std::string _fileType;
  std::string _filePath;

  // 清理内部
  virtual void cleanup();

  // 从图像读取实际数据
  virtual void* readDataFromImage(const long long& startX, const long long& startY, const unsigned long long& width, 
    const unsigned long long& height, const unsigned int& level) = 0;

  template <typename T> void createCache() {
    if (_isValid) {
      _cache.reset(new TileCache<T>(_cacheSize));
    }
  }
};

template <> void MULTIRESOLUTIONIMAGEINTERFACE_EXPORT MultiResolutionImage::getRawRegion(const long long& startX, const long long& startY, const unsigned long long& width,
  const unsigned long long& height, const unsigned int& level, unsigned char*& data);

template <> void MULTIRESOLUTIONIMAGEINTERFACE_EXPORT MultiResolutionImage::getRawRegion(const long long& startX, const long long& startY, const unsigned long long& width,
  const unsigned long long& height, const unsigned int& level, unsigned short*& data);

template <> void MULTIRESOLUTIONIMAGEINTERFACE_EXPORT MultiResolutionImage::getRawRegion(const long long& startX, const long long& startY, const unsigned long long& width,
  const unsigned long long& height, const unsigned int& level, unsigned int*& data);

template <> void MULTIRESOLUTIONIMAGEINTERFACE_EXPORT MultiResolutionImage::getRawRegion(const long long& startX, const long long& startY, const unsigned long long& width,
  const unsigned long long& height, const unsigned int& level, float*& data);

#endif
