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
    //���캯��
  MultiResolutionImage();
    //��������
  virtual ~MultiResolutionImage();
  ImageSource* clone();

  //����ͼ�񣬷����Ƿ�����Ч��ͼ��
  bool initialize(const std::string& imagePath);

  //ʵ�ʵĳ�ʼ��ʵ��
  virtual bool initializeType(const std::string& imagePath) = 0;

  //֧�־��ж��zƽ��Ļõ�Ƭ
  int getNumberOfZPlanes() const;
  void setCurrentZPlaneIndex(const unsigned int& zPlaneIndex);
  unsigned int getCurrentZPlaneIndex() const;

  //��ȡ�洢����������(����Ŀ��Ŵ�)
  virtual std::string getProperty(const std::string& propertyName) { return std::string(); };

  //��ȡ/���û�������ֵ
  virtual const unsigned long long getCacheSize();
  virtual void setCacheSize(const unsigned long long cacheSize);

  //! Gets the number of levels in the slide pyramid
  virtual const int getNumberOfLevels() const;
  
  //! ��ȡ�õ�Ƭ�������еĲ���
  virtual const std::vector<unsigned long long> getDimensions() const;

  //! ��ȡ��������ָ������ĳߴ�
  virtual const std::vector<unsigned long long> getLevelDimensions(const unsigned int& level) const;
  
  //�õ�����ˮƽ����ڻ���ˮƽ���²�������
  virtual const double getLevelDownsample(const unsigned int& level) const;

  //�ڸ���������²������ӵ�����£���ȡ��������²������Ӷ�Ӧ�ļ���
  virtual const int getBestLevelForDownSample(const double& downsample) const;

  //��ȡͨ������Сֵ�����û��ָ��ͨ����Ĭ��Ϊ��һ��ͨ��
  virtual double getMinValue(int channel = -1) = 0;
  
  //��ȡͨ�������ֵ�����û��ָ��ͨ����Ĭ��Ϊ��һ��ͨ��
  virtual double getMaxValue(int channel = -1) = 0;

  //��ȡ��ͼ����ļ�����
  const std::string getFileType() const;
  
  // ��ȡ��Ϊ���������ݣ�����һ��������ͼ���࣬����������ص���Ϣ����һ������
  // �������ݺ���ɫ����
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

  //��ȡ����������������ݡ��û���������㹻���ڴ��������������鲢����ڴ档
  //��ע�⣬����int32 ARGB���ݣ�������OpenSlide�У���ɫ��˳��ȡ������Ļ����Ķ���(Windows���͵�BGRA)
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

  //! To make MultiResolutionImage thread-safe  ʹMultiResolutionImage�̰߳�ȫ
  std::unique_ptr<std::shared_mutex> _openCloseMutex;
  std::unique_ptr<std::mutex> _cacheMutex;
  std::shared_ptr<void> _cache;

  // ��ֱ���ͼ��ĸ�������
  std::vector<std::vector<unsigned long long> > _levelDimensions;
  unsigned int _numberOfLevels;
  unsigned int _numberOfZPlanes;
  unsigned int _currentZPlaneIndex;

  // �Ѽ��ػõ�Ƭ������
  unsigned long long _cacheSize;
  std::string _fileType;
  std::string _filePath;

  // �����ڲ�
  virtual void cleanup();

  // ��ͼ���ȡʵ������
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
