#include "cmywindows.h"
#include <QDebug>

CMyWindows::CMyWindows(QWidget *parent) : QMainWindow(parent)
{
	m_s32TimerId = 0;
}

CMyWindows::~CMyWindows()
{
	if (m_s32TimerId != 0)
	{
		killTimer(m_s32TimerId);
		m_s32TimerId = 0;
	}
}

void CMyWindows::mousePressEvent( QMouseEvent * event)
{
	QMainWindow::mousePressEvent(event);

	if (event->buttons() == Qt::LeftButton)
	{

		if (m_s32TimerId != 0)
		{
			killTimer(m_s32TimerId);
			m_s32TimerId = 0;
		}
		m_s32TimerId = startTimer(100);
		m_s32MouseLeftButtonPressTime = 0;
	}

}

void CMyWindows::mouseReleaseEvent(QMouseEvent * event)
{
	QMainWindow::mouseReleaseEvent(event);

	if (m_s32TimerId != 0)
	{
		killTimer(m_s32TimerId);
		m_s32TimerId = 0;
	}

}



void CMyWindows::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_s32TimerId)
	{
		m_s32MouseLeftButtonPressTime += 100;
		if (m_s32MouseLeftButtonPressTime > 2000)
		{
			qDebug() << "Left Button Press: " << m_s32MouseLeftButtonPressTime;
			killTimer(m_s32TimerId);
			m_s32TimerId = 0;

			emit LeftButtonPressTimeOut();
		}
	}
}
