#include "PanTool.h"
#include <QAction>
#include "../PathologyViewer.h"

    //鼠标移动事件
void PanTool::mouseMoveEvent(QMouseEvent *event) {
  if (_viewer) {
    if (_viewer->isPanning()) {
      _viewer->pan(event->pos());
      event->accept();
    }
  }
}
    //鼠标按下事件
void PanTool::mousePressEvent(QMouseEvent *event) {
  if (_viewer) {
    _viewer->togglePan(true, event->pos());
    event->accept();
  }
}
    //鼠标释放事件
void PanTool::mouseReleaseEvent(QMouseEvent *event) {
  if (_viewer) {
    _viewer->togglePan(false);
    event->accept();
  }
}
    //获得pan工具按钮
QAction* PanTool::getToolButton() {
  if (!_button) {
    _button = new QAction("Pan", this);
    _button->setObjectName(QString::fromStdString(name()));
    _button->setIcon(QIcon(QPixmap(":/basictools_icons/pan.png")));
    _button->setShortcut(QKeySequence("x"));
  }
  return _button;
}
    //工具名
std::string PanTool::name() {
  return std::string("pan");
}