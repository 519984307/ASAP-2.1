#ifndef ANNOTATIONTOOL_H
#define ANNOTATIONTOOL_H

#include "interfaces/interfaces.h"
#include "core/Point.h"
#include "annotationplugin_export.h"

class AnnotationWorkstationExtensionPlugin;
class PathologyViewer;
	
	//��ע����
class ANNOTATIONPLUGIN_EXPORT AnnotationTool : public  ToolPluginInterface {
  Q_OBJECT

public :
	//���캯��
  AnnotationTool(AnnotationWorkstationExtensionPlugin* annotationPlugin, PathologyViewer* viewer);
  virtual std::string name() = 0;  //������
  virtual void mouseMoveEvent(QMouseEvent *event);		//����ƶ��¼�
  virtual void mousePressEvent(QMouseEvent *event);		//��갴��
  virtual void mouseReleaseEvent(QMouseEvent *event);	//����ͷ�
  virtual void mouseDoubleClickEvent(QMouseEvent *event);	//˫��
  virtual void keyPressEvent(QKeyEvent *event);		//����
  virtual QAction* getToolButton() = 0;		//��ù��߰�ť
  void setActive(bool active);

public slots:
  virtual void cancelAnnotation();

protected:
	//�������
  virtual void addCoordinate(const QPointF& scenePos);
	//��ע���
  AnnotationWorkstationExtensionPlugin* _annotationPlugin;
  bool _generating;
  Point _start;
  Point _last;

  bool _startSelectionMove;
  QPointF _moveStart;

};

#endif