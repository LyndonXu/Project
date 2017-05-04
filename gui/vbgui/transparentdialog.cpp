#include "transparentdialog.h"
#include "ui_transparentdialog.h"
#include "mainwindow.h"
#include "common_define.h"
#include <QDesktopWidget>
#include <QMouseEvent>
#include <QWSServer>
#include <QDebug>

CTransparentDialog::CTransparentDialog(QWidget *parent) :
	CMyWindows(parent),
	ui(new Ui::CTransparentDialog)
{
	ui->setupUi(this);

	setFocusPolicy(Qt::NoFocus);

#ifdef Q_WS_QWS
	//    QWSServer::setBackground(QColor(0,0,0,0));
	//    QWSServer::setCursorVisible(FALSE);
	//#else
	setAttribute(Qt::WA_TranslucentBackground, true);
#endif

	setWindowOpacity(0.5);

	QDesktopWidget* desktop = QApplication::desktop();
	setWindowFlags(Qt::FramelessWindowHint);//去掉标题栏
	move((desktop->width() - this->width()) / 2, (desktop->height() - this->height()) / 2);


#ifdef Q_WS_QWS
	setGeometry(0, 0, desktop->width(), desktop->height());
#endif

	connect(this, SIGNAL(LeftButtonPressTimeOut()), this, SLOT(SwitchShow()));

}

CTransparentDialog::~CTransparentDialog()
{
	delete ui;
}

void CTransparentDialog::mousePressEvent(QMouseEvent *event)
{
	CMyWindows::mousePressEvent(event);

	if(event->button()==Qt::LeftButton)
	{
		qDebug() << "mouse left" <<  event->x() * SRC_SCREEN_WIDTH / width()
				 << event->y() * SRC_SCREEN_HEIGHT / height();
	}
	else if(event->button()==Qt::RightButton)
	{

	}
	else if(event->button()==Qt::MidButton)
	{

	}
}

void CTransparentDialog::SwitchShow()
{
	if (m_pParent != NULL)
	{
		((MainWindow *)m_pParent)->SwitchShow();
	}
}



