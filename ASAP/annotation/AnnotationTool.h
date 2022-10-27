#ifndef ANNOTATIONTOOL_H
#define ANNOTATIONTOOL_H

#include "interfaces/interfaces.h"
#include "core/Point.h"
#include "annotationplugin_export.h"

class AnnotationWorkstationExtensionPlugin;
class PathologyViewer;
	
	//标注工具
class ANNOTATIONPLUGIN_EXPORT AnnotationTool : public  ToolPluginInterface {
  Q_OBJECT

public :
	//构造函数
  AnnotationTool(AnnotationWorkstationExtensionPlugin* annotationPlugin, PathologyViewer* viewer);
  virtual std::string name() = 0;  //工具名
  virtual void mouseMoveEvent(QMouseEvent *event);		//鼠标移动事件
  virtual void mousePressEvent(QMouseEvent *event);		//鼠标按下
  virtual void mouseReleaseEvent(QMouseEvent *event);	//鼠标释放
  virtual void mouseDoubleClickEvent(QMouseEvent *event);	//双击
  virtual void keyPressEvent(QKeyEvent *event);		//键盘
  virtual QAction* getToolButton() = 0;		//获得工具按钮
  void setActive(bool active);

public slots:
  virtual void cancelAnnotation();

protected:
	//添加坐标
  virtual void addCoordinate(const QPointF& scenePos);
	//标注插件
  AnnotationWorkstationExtensionPlugin* _annotationPlugin;
  bool _generating;
  Point _start;
  Point _last;

  bool _startSelectionMove;
  QPointF _moveStart;

};

#endif