#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QDesktopWidget>
#include <QPushButton>
#include <QMouseEvent>
#ifdef Q_WS_QWS
#include <QWSServer>
#endif
#include <QDebug>

MainWindow::MainWindow(QString strCfg, QWidget *parent) :
	CMyWindows(parent),
	m_strNextConfig(""),
	ui(new Ui::MainWindow),
	m_csCfgReader(this)
{
	ui->setupUi(this);

	setFocusPolicy(Qt::NoFocus);

	QDesktopWidget* desktop = QApplication::desktop();
	//    setParent(desktop);
#ifdef Q_WS_QWS
	QWSServer::setBackground(QColor(0,0,0,0));
	QWSServer::setCursorVisible(FALSE);
#else
	//   QApplication::setOverrideCursor(Qt::BlankCursor);
#endif

	//setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);//去掉标题栏
	setWindowFlags(Qt::FramelessWindowHint);//去掉标题栏
	//ui->statusBar->hide();

	move((desktop->width() - this->width()) / 2, (desktop->height() - this->height()) / 2);
#ifdef Q_WS_QWS
	setGeometry(0, 0, desktop->width(), desktop->height());
#endif

	connect(this, SIGNAL(LeftButtonPressTimeOut()), this, SLOT(SwitchShow()));

	//setWindowState(Qt::WindowMaximized);
	//showFullScreen();
	//showMaximized();
	m_csCfgReader.ParseFile(strCfg.toAscii());
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
	QMainWindow::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent * event)
{
	QMainWindow::mouseDoubleClickEvent(event);
}


void MainWindow::SwitchShow()
{
	if(isVisible())
	{
		m_csTouchRcver.SetParent(this);

		m_csTouchRcver.show();
		hide();
	}
	else
	{
		m_csTouchRcver.hide();
		show();
	}
}

void MainWindow::SetDefault()
{
	m_csCfgReader.SetDefault();
}

/*
void MainWindow::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	QPalette pal(palette());

	pal.setBrush(QPalette::Window, QBrush(_image.scaled(event->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
	setPalette(pal);
}
*/
