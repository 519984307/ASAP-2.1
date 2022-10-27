#include "PanTool.h"
#include <QAction>
#include "../PathologyViewer.h"

    //����ƶ��¼�
void PanTool::mouseMoveEvent(QMouseEvent *event) {
  if (_viewer) {
    if (_viewer->isPanning()) {
      _viewer->pan(event->pos());
      event->accept();
    }
  }
}
    //��갴���¼�
void PanTool::mousePressEvent(QMouseEvent *event) {
  if (_viewer) {
    _viewer->togglePan(true, event->pos());
    event->accept();
  }
}
    //����ͷ��¼�
void PanTool::mouseReleaseEvent(QMouseEvent *event) {
  if (_viewer) {
    _viewer->togglePan(false);
    event->accept();
  }
}
    //���pan���߰�ť
QAction* PanTool::getToolButton() {
  if (!_button) {
    _button = new QAction("Pan", this);
    _button->setObjectName(QString::fromStdString(name()));
    _button->setIcon(QIcon(QPixmap(":/basictools_icons/pan.png")));
    _button->setShortcut(QKeySequence("x"));
  }
  return _button;
}
    //������
std::string PanTool::name() {
  return std::string("pan");
}