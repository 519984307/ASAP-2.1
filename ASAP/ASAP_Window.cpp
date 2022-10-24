#include <string>
#include <vector>
#include <iostream>

#include <QFileDialog>
#include <QToolButton>
#include <QIcon>
#include <QLabel>
#include <QGraphicsEffect>
#include <QDebug>
#include <QPushButton>
#include <QDockWidget>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPluginLoader>
#include <QComboBox>
#include <QToolBar>
#include <QStyle>
#include <QActionGroup>
#include <QSettings>
#include <QFileInfo>
#include <QStandardPaths>
#include <QtUiTools>
#include <QTreeWidget>

#include "ASAP_Window.h"
#include "PathologyViewer.h"
#include "interfaces/interfaces.h"
#include "WSITileGraphicsItemCache.h"
#include "config/ASAPMacros.h"
#include "multiresolutionimageinterface/MultiResolutionImageReader.h"
#include "multiresolutionimageinterface/MultiResolutionImage.h"
#include "multiresolutionimageinterface/MultiResolutionImageFactory.h"
#include "multiresolutionimageinterface/OpenSlideImage.h"

#ifdef WIN32
const char* ASAP_Window::sharedLibraryExtensions = ".dll";
#elif __APPLE__
const char* ASAP_Window::sharedLibraryExtensions = ".dylib";
#else
const char* ASAP_Window::sharedLibraryExtensions = ".so";
#endif

using namespace std;

ASAP_Window::ASAP_Window(QWidget *parent) :
    QMainWindow(parent),
    _cacheMaxByteSize(1000*512*512*3),
    _settings(NULL)
{
    //初始化界面
  setupUi();
    //翻译界面
  retranslateUi();
  connect(actionOpen, SIGNAL(triggered(bool)), this, SLOT(on_actionOpen_triggered()));
  connect(actionClose, SIGNAL(triggered(bool)), this, SLOT(on_actionClose_triggered()));
  connect(actionAbout, SIGNAL(triggered(bool)), this, SLOT(on_actionAbout_triggered()));

    //中心控件
  PathologyViewer* view = this->findChild<PathologyViewer*>("pathologyView");
    //加载插件
  this->loadPlugins();
    //中心界面设置缓存大小
  view->setCacheSize(_cacheMaxByteSize);

    //pan工具
  if (view->hasTool("pan")) {
    view->setActiveTool("pan");
    QList<QAction*> toolButtons = mainToolBar->actions();
    for (QList<QAction*>::iterator it = toolButtons.begin(); it != toolButtons.end(); ++it) {
      if ((*it)->objectName() == "pan") {
        (*it)->setChecked(true);
      }
    }
  }  
    //设置view（QGraphicsView）是否可见
  view->setEnabled(false);

    //QSettings使用户可以保存应用程序设置，并且支持用户自定义存储格式。
    //QSetings API基于QVariant,因而你可以存储却大部分类型的数据
    //IniFormat:将设置存储在INI文件中。
    //UserScope:将设置存储在特定于当前用户的位置(例如，在用户的主目录中)
  _settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "DIAG", "ASAP", this);
    //读取设置
  readSettings();
    //返回命令行参数列表
  QStringList args = QApplication::arguments();  
    //如果存在文件
  if (args.size() > 1) {
      //打开文件
    openFile(args[1], "default");
  }
}

void ASAP_Window::writeSettings()
{
  _settings->beginGroup("ASAP");
  _settings->setValue("size", size());
  _settings->setValue("maximized", isMaximized());
  _settings->endGroup();
}

//读取设置
void ASAP_Window::readSettings()
{
  _settings->beginGroup("ASAP");
  //尺寸大小
  resize(_settings->value("size", QSize(1037, 786)).toSize());
  if (_settings->value("maximized", false).toBool()) {
    this->setWindowState(Qt::WindowMaximized);
  }
  _settings->endGroup();
}

//加载插件
void ASAP_Window::loadPlugins() {
  PathologyViewer* viewer = this->findChild<PathologyViewer*>("pathologyView");
  _pluginsDir = QDir(qApp->applicationDirPath());
  if (_pluginsDir.cd("plugins")) {
    if (_pluginsDir.cd("tools")) {
      foreach(QString fileName, _pluginsDir.entryList(QDir::Files)) {
        if (fileName.toLower().endsWith(sharedLibraryExtensions)) {
          QPluginLoader loader(_pluginsDir.absoluteFilePath(fileName));
          QObject *plugin = loader.instance();
          if (plugin) {
            std::shared_ptr<ToolPluginInterface> tool(qobject_cast<ToolPluginInterface*>(plugin));
            if (tool) {
              tool->setViewer(viewer);
              QAction* toolAction = tool->getToolButton();
              connect(toolAction, SIGNAL(triggered(bool)), viewer, SLOT(changeActiveTool()));
              _toolPluginFileNames.push_back(fileName.toStdString());
              viewer->addTool(tool);
              QToolBar* mainToolBar = this->findChild<QToolBar *>("mainToolBar");
              toolAction->setCheckable(true);
              _toolActions->addAction(toolAction);
              mainToolBar->addAction(toolAction);
            }
          }
        }
      }
      _pluginsDir.cdUp();
    }
    if (_pluginsDir.cd("workstationextension")) {
      QDockWidget* lastDockWidget = NULL;
      QDockWidget* firstDockWidget = NULL;
      foreach(QString fileName, _pluginsDir.entryList(QDir::Files)) {
        if (fileName.toLower().endsWith(sharedLibraryExtensions)) {        
          QPluginLoader loader(_pluginsDir.absoluteFilePath(fileName));       
          QObject *plugin = loader.instance();    
          if (plugin) {         
            std::unique_ptr<WorkstationExtensionPluginInterface> extension(qobject_cast<WorkstationExtensionPluginInterface*>(plugin));
            if (extension) {
              _extensionPluginFileNames.push_back(fileName.toStdString());
              connect(this, SIGNAL(newImageLoaded(std::weak_ptr<MultiResolutionImage>, std::string)), &*extension, SLOT(onNewImageLoaded(std::weak_ptr<MultiResolutionImage>, std::string)));
              connect(this, SIGNAL(imageClosed()), &*extension, SLOT(onImageClosed()));
              extension->initialize(viewer);
              if (extension->getToolBar()) {
                this->addToolBar(extension->getToolBar());
              }
              if (extension->getDockWidget()) {
                QDockWidget* extensionDW = extension->getDockWidget();
                extensionDW->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
                if (lastDockWidget) {
                  this->tabifyDockWidget(lastDockWidget, extensionDW);
                }
                else {
                  this->addDockWidget(Qt::LeftDockWidgetArea, extensionDW);
                  firstDockWidget = extensionDW;
                }
                extensionDW->setTitleBarWidget(new QWidget());
                lastDockWidget = extensionDW;
                QMenu* viewMenu = this->findChild<QMenu*>("menuView");
                QMenu* viewDocksMenu = viewMenu->findChild<QMenu*>("menuViewDocks");
                if (!viewDocksMenu) {
                  viewDocksMenu = viewMenu->addMenu("Docks");
                  viewDocksMenu->setObjectName("menuViewDocks");
                }
                viewDocksMenu->addAction(extensionDW->toggleViewAction());
              }
              if (extension->getMenu()) {
                this->menuBar->addMenu(extension->getMenu());
              }
              std::vector<std::shared_ptr<ToolPluginInterface> > tools = extension->getTools();
              if (!tools.empty()) {
                mainToolBar->addSeparator();
                for (unsigned int i = 0; i < tools.size(); ++i) {
                  QAction* toolAction = tools[i]->getToolButton();
                  connect(toolAction, SIGNAL(triggered(bool)), viewer, SLOT(changeActiveTool()));
                  viewer->addTool(tools[i]);
                  mainToolBar->addAction(toolAction);
                  toolAction->setCheckable(true);
                  _toolActions->addAction(toolAction);
                  toolAction->setParent(this);
                }
              }
              _extensions.push_back(std::move(extension));
            }
          }
        }
      }
      _pluginsDir.cdUp();
      if (firstDockWidget) {
        firstDockWidget->raise();
      }
    }
  }
}


void ASAP_Window::keyPressEvent(QKeyEvent* event)
{
    event->ignore();
    if (event->key() == Qt::Key::Key_F1) {
        this->showShortcutOverview();
    }
    for (auto const& extension : _extensions) {
        extension->keyPressEvent(event);
    }
}

void ASAP_Window::closeEvent(QCloseEvent *event) {
  event->accept();
}

ASAP_Window::~ASAP_Window()
{
  on_actionClose_triggered();
  writeSettings();
}

void ASAP_Window::on_actionAbout_triggered() {
  QUiLoader loader;
  QFile file(":/ASAP_ui/aboutdialog.ui");
  file.open(QFile::ReadOnly);
  QDialog* content = qobject_cast<QDialog*>(loader.load(&file, this));
  if (content) {
    QLabel* generalInfoLabel = content->findChild<QLabel*>("generalInfoLabel");
    QString generalInfoText = generalInfoLabel->text();
    generalInfoText.replace("@VERSION_STRING@", ASAP_VERSION_STRING);
    generalInfoLabel->setText(generalInfoText);
    QTreeWidget* pluginList = content->findChild<QTreeWidget*>("loadedPluginsOverviewTreeWidget");
    QList<QTreeWidgetItem*> root_items = pluginList->findItems("Tool", Qt::MatchExactly);
    if (!root_items.empty()) {
      QTreeWidgetItem* root_item = root_items[0];
      for (std::vector<std::string>::const_iterator it = _toolPluginFileNames.begin(); it != _toolPluginFileNames.end(); ++it) {
        root_item->addChild(new QTreeWidgetItem(QStringList(QString::fromStdString(*it))));
      }
    }
    root_items = pluginList->findItems("Workstation Extension", Qt::MatchExactly);
    if (!root_items.empty()) {
      QTreeWidgetItem* root_item = root_items[0];
      for (std::vector<std::string>::const_iterator it = _extensionPluginFileNames.begin(); it != _extensionPluginFileNames.end(); ++it) {
        root_item->addChild(new QTreeWidgetItem(QStringList(QString::fromStdString(*it))));
      }
    }    
    content->exec();
  }
  file.close();
}

//关闭事件点击
void ASAP_Window::on_actionClose_triggered()
{
    for (std::vector<std::unique_ptr<WorkstationExtensionPluginInterface> >::iterator it = _extensions.begin(); it != _extensions.end(); ++it) {
      if (!(*it)->canClose()) {
        return;
      }
    }
    emit imageClosed();
    _settings->setValue("currentFile", QString());
    this->setWindowTitle("ASAP");
    if (_img) {
		  PathologyViewer* view = this->findChild<PathologyViewer*>("pathologyView");
		  view->close();
		  _img.reset();
		  statusBar->showMessage("Closed file!", 5);
    }
}

//打开文件（文件名，"default"）
void ASAP_Window::openFile(const QString& fileName, const QString& factoryName) {
    //清除状态栏信息
  statusBar->clearMessage();
    //如果文件名不为空
  if (!fileName.isEmpty()) {
      //存在_img
    if (_img) {
        //关闭事件
      on_actionClose_triggered();
    }
      //将fileName返回一个std::string对象给fn
    std::string fn = fileName.toStdString();
      //保存上一次打开的路径
    _settings->setValue("lastOpenendPath", QFileInfo(fileName).dir().path());
      //返回文件的名称，不包括路径。
    _settings->setValue("currentFile", QFileInfo(fileName).fileName());
      //设置窗口标题为ASAP- + 文件名
    this->setWindowTitle(QString("ASAP - ") + QFileInfo(fileName).fileName());
      //创建多分辨率图像读取类
    MultiResolutionImageReader imgReader;

    _img.reset(imgReader.open(fn, factoryName.toStdString()));
    if (_img) {
      if (_img->valid()) {
        vector<unsigned long long> dimensions = _img->getLevelDimensions(_img->getNumberOfLevels() - 1);
        PathologyViewer* view = this->findChild<PathologyViewer*>("pathologyView");
        //初始化场景
        view->initialize(_img);
        emit newImageLoaded(_img, fn);
      }
      else {
        statusBar->showMessage("Unsupported file type version");
      }
    }
    else {
      statusBar->showMessage("Invalid file type");
    }
  }
}

void ASAP_Window::on_actionOpen_triggered()
{ 
	QList<QString> filename_factory = this->getFileNameAndFactory();
	openFile(filename_factory[0], filename_factory[1] == "All supported types" ? "default": filename_factory[1]);
}

QList<QString> ASAP_Window::getFileNameAndFactory() {
	QString filterList;
	std::set<std::string> allExtensions = MultiResolutionImageFactory::getAllSupportedExtensions();
	QString defaultString = "All supported types (";
	for (auto it = allExtensions.begin(); it != allExtensions.end(); ++it) {
		defaultString += " *." + QString::fromStdString(*it);
	}
	defaultString += ")";
	filterList += defaultString;

	std::vector<std::pair<std::string, std::set<std::string>> > factoriesAndExtensions = MultiResolutionImageFactory::getLoadedFactoriesAndSupportedExtensions();
	for (auto it = factoriesAndExtensions.begin(); it != factoriesAndExtensions.end(); ++it) {
		QString extensionString = "(*." + QString::fromStdString(*(it->second.begin()));
		for (auto extensionIt = std::next(it->second.begin(), 1); extensionIt != it->second.end(); ++extensionIt) {
			extensionString += " *." + QString::fromStdString(*extensionIt);
		}
		extensionString += ")";
		filterList += (";;" + QString::fromStdString(it->first) + " " + extensionString);
	}
	QString selectedFilter;
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), _settings->value("lastOpenendPath", QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)).toString(), filterList, &selectedFilter);
	QString selectedFactory = selectedFilter.split("(")[0].trimmed();
	return QList<QString>({ fileName, selectedFactory });
}

void ASAP_Window::setCacheSize(const unsigned long long& cacheMaxByteSize) {
  PathologyViewer* view = this->findChild<PathologyViewer*>("pathologyView");
  if (view) {
    view->setCacheSize(_cacheMaxByteSize);
  }
}
    
unsigned long long ASAP_Window::getCacheSize() const {
  PathologyViewer* view = this->findChild<PathologyViewer*>("pathologyView");
  if (view) {
    return view->getCacheSize();
  }
  else {
      return 0;
  }
}

//界面
void ASAP_Window::setupUi()
{
  if (this->objectName().isEmpty()) {
      this->setObjectName(QStringLiteral("ASAP"));
  }
  //窗口大小 QTabWidget::East右侧
  this->resize(1037, 786);
  this->setTabPosition(Qt::DockWidgetArea::LeftDockWidgetArea, QTabWidget::East);
  this->setTabPosition(Qt::DockWidgetArea::RightDockWidgetArea, QTabWidget::West);
  QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  sizePolicy.setHorizontalStretch(0);
  sizePolicy.setVerticalStretch(0);
  //将确定小部件的首选高度是否取决于其宽度的标志设置为依赖。
  sizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
  //将小部件的大小策略设置为水平和垂直，具有标准拉伸且没有高度换宽。
  this->setSizePolicy(sizePolicy);
  //打开
  actionOpen = new QAction(this);
  actionOpen->setObjectName(QStringLiteral("actionOpen"));
  //关闭
  actionClose = new QAction(this);
  actionClose->setObjectName(QStringLiteral("actionClose"));
  //设置图标
  actionOpen->setIcon(QIcon(QPixmap(":/ASAP_icons/open.png")));
  actionClose->setIcon(QIcon(QPixmap(":/ASAP_icons/close.png")));
  //关于
  actionAbout = new QAction(this);
  actionAbout->setObjectName(QStringLiteral("actionAbout"));

  //菜单栏
  menuBar = new QMenuBar(this);
  menuBar->setObjectName(QStringLiteral("menuBar"));
  menuBar->setGeometry(QRect(0, 0, 1037, 21));
  //file菜单
  menuFile = new QMenu(menuBar);
  menuFile->setObjectName(QStringLiteral("menuFile"));
  //view菜单
  menuView = new QMenu(menuBar);
  menuView->setObjectName(QStringLiteral("menuView"));
  //帮助菜单
  menuHelp = new QMenu(menuBar);
  menuHelp->setObjectName(QStringLiteral("menuHelp"));
  this->setMenuBar(menuBar);
  //工具栏
  mainToolBar = new QToolBar(this);
  mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
  mainToolBar->addAction(actionOpen);
  mainToolBar->addAction(actionClose);
  //添加分隔符
  mainToolBar->addSeparator();
  this->addToolBar(Qt::TopToolBarArea, mainToolBar);

  _toolActions = new QActionGroup(this);
  statusBar = new QStatusBar(this);
  statusBar->setObjectName(QStringLiteral("statusBar"));
  this->setStatusBar(statusBar);

  menuBar->addAction(menuFile->menuAction());
  menuBar->addAction(menuView->menuAction());
  menuBar->addAction(menuHelp->menuAction());
  menuFile->addAction(actionOpen);
  menuFile->addAction(actionClose);
  menuHelp->addAction(actionAbout);
  //中心控件
  centralWidget = new QWidget(this);
  centralWidget->setObjectName(QStringLiteral("centralWidget"));
  sizePolicy.setHeightForWidth(centralWidget->sizePolicy().hasHeightForWidth());
  centralWidget->setSizePolicy(sizePolicy);
  //为指定的语言和地区构造一个 QLocale 对象。
  centralWidget->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
  horizontalLayout_2 = new QHBoxLayout(centralWidget);
  horizontalLayout_2->setSpacing(6);
  horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
  horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
  //中心控件
  pathologyView = new PathologyViewer(centralWidget);
  pathologyView->setObjectName(QStringLiteral("pathologyView"));

  horizontalLayout_2->addWidget(pathologyView);

  this->setCentralWidget(centralWidget);
}
//重新翻译界面
void ASAP_Window::retranslateUi()
{
  this->setWindowTitle(QString("ASAP v") + QString(ASAP_VERSION_STRING));
  actionOpen->setText(QApplication::translate("PathologyWorkstation", "Open", 0));
  actionOpen->setIconText(QApplication::translate("PathologyWorkstation", "Open", 0));
  actionAbout->setText(QApplication::translate("PathologyWorkstation", "About...", 0));
  actionOpen->setShortcut(QApplication::translate("PathologyWorkstation", "Ctrl+O", 0));
  actionClose->setText(QApplication::translate("PathologyWorkstation", "Close", 0));
  actionClose->setShortcut(QApplication::translate("PathologyWorkstation", "Ctrl+C", 0));
  actionClose->setIconText(QApplication::translate("PathologyWorkstation", "Close", 0));
  menuFile->setTitle(QApplication::translate("PathologyWorkstation", "File", 0));
  menuView->setTitle(QApplication::translate("PathologyWorkstation", "View", 0));
  menuHelp->setTitle(QApplication::translate("PathologyWorkstation", "Help", 0));
} 

void ASAP_Window::showShortcutOverview() {
    auto actions = this->findChildren<QAction*>();
    for (QAction* action : actions) {
        qDebug() << action->objectName() << "\t" << action->shortcut().toString();
    }
}
