#ifndef PATHOLOGYVIEWER_H
#define PATHOLOGYVIEWER_H
#include "asaplib_export.h"
#include <QGraphicsView>
#include <vector>
#include <memory>

class MultiResolutionImage;
class IOThread;
class PrefetchThread;
class ToolPluginInterface;
class MiniMap;
class WSITileGraphicsItemCache;
class TileManager;
class ScaleBar;
class QSettings;

namespace pathology {
  struct LUT;
}

class ASAPLIB_EXPORT PathologyViewer : public QGraphicsView
{
    Q_OBJECT
    
public:
      // ���ŵ�Sceneλ��
    QPointF _zoomToScenePos;
      //���ŵ�Viewλ��
    QPointF _zoomToViewPos;

    PathologyViewer(QWidget *parent = 0);
    ~PathologyViewer();
      
      //��ʼ��
    void initialize(std::shared_ptr<MultiResolutionImage> img);
      //�ر�
    void close();

    float getZoomSensitivity() const;
    void  setZoomSensitivity(float zoomSensitivity);
    void  setPanSensitivity(float panSensitivity);
    float getPanSensitivity() const;

    float getForegroundOpacity() const;
    void  setForegroundOpacity(const float& opacity);

    void setForegroundLUT(const pathology::LUT& LUT);
    void setForegroundChannel(unsigned int channel);
    void setEnableForegroundRendering(bool enableForegroundRendering);

    void togglePan(bool pan, const QPoint& startPos = QPoint());
    void pan(const QPoint& panTo);
    bool isPanning();
    void zoom(float numSteps);
   
    bool hasTool(const std::string& toolName) const;
    void addTool(std::shared_ptr<ToolPluginInterface> tool);
    void setActiveTool(const std::string& toolName);
    std::shared_ptr<ToolPluginInterface> getActiveTool();
    float getSceneScale() { return _sceneScale; }

    unsigned long long getCacheSize();
    void setCacheSize(unsigned long long& maxCacheSize);
    void setAutoUpdate(bool autoUpdate);

    void updateCurrentFieldOfView();

signals :
    void fieldOfViewChanged(const QRectF& FOV, const unsigned int level);
    void updateBBox(const QRectF& FOV);
    void backgroundChannelChanged(int channelNr);

public slots :
    void moveTo(const QPointF& pos);
    void changeActiveTool();
    void onFieldOfViewChanged(const QRectF& FOV, const unsigned int level);
    void onForegroundImageChanged(std::weak_ptr<MultiResolutionImage> for_img, float scale);

private :
    
    // Functions for panning
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseDoubleClickEvent(QMouseEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void resizeEvent(QResizeEvent *event);

    // Functions for zooming and resizing
    void wheelEvent(QWheelEvent *event);    

    // Functions for overviewmap
    void initializeGUIComponents(unsigned int level);
    void initializeImage(QGraphicsScene *scn, unsigned int tileSize, unsigned int lastLevel);
        //��������
    float _sceneScale;

    // Interface to the multi-resolution image. Please note that PathologyViewer
    // should never modify _img (it is modified in the RenderThread due to calls
    // to readRegion), otherwise race conditions could occur.
    std::shared_ptr<MultiResolutionImage> _img;
    std::weak_ptr<MultiResolutionImage> _for_img;

    // Minimap
    MiniMap* _map;

    // ScaleBar
    ScaleBar* _scaleBar;

    // QSettings
    QSettings* _settings;

    // Tools
    std::shared_ptr<ToolPluginInterface> _activeTool;

    // Members to track panning and zooming  ����ƽ�ƺ����ŵĳ�Ա
    float _zoomSensitivity; //����������
    float _numScheduledScalings;  //Ԥ��������
    float _panSensitivity;  //�ƶ�������
    bool _pan;  //bool�ƶ�
    QPoint _prevPan;

    // Members related to rendering ����ʾ��صĳ�Ա
    IOThread* _ioThread;
    int _backgroundChannel;
    int _foregroundChannel;
    float _opacity;
    std::string _LUTname;
    float _foregroundImageScale;
    bool _renderForeground;  //��ʾǰ��

    PrefetchThread* _prefetchthread;   
    TileManager* _manager;  //��Ƭ����
    unsigned long long _cacheSize;  //�����С
    WSITileGraphicsItemCache* _cache;  //����

    std::map<std::string, std::shared_ptr<ToolPluginInterface> > _tools;

private slots :
    void showContextMenu(const QPoint& pos);
    void scalingTime(qreal x);
    void zoomFinished();
};
  

#endif
