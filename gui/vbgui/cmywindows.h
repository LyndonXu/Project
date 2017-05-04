#ifndef CMYWINDOWS_H
#define CMYWINDOWS_H

#include <QMainWindow>
#include <QWidget>
#include <QMouseEvent>
#include <QTimerEvent>

class CMyWindows : public QMainWindow
{
	Q_OBJECT
public:
	explicit CMyWindows(QWidget *parent = 0);
	~CMyWindows();

signals:
	void LeftButtonPressTimeOut();

public slots:

private:
	int m_s32TimerId;
	int m_s32MouseLeftButtonPressTime;

protected:
	void mousePressEvent(QMouseEvent * event);
	void mouseReleaseEvent(QMouseEvent * event);

	void timerEvent(QTimerEvent *event);
};

#endif // CMYWINDOWS_H
