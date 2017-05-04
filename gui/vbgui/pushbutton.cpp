#include "pushbutton.h"
#include "common_define.h"
#include <QDebug>
CPushButton::CPushButton(const QString &text, QWidget *parent, CElemObj *pObjParent) :
	QPushButton(text, parent), CElemObj(pObjParent),
	m_strImg1(""),
	m_strImg2(""),
	m_strPressCmd(""),
	m_strReleaseCmd("")

{
	setCheckable(false);
	setFocusPolicy(Qt::NoFocus);
	//setFocusPolice(QPushButton::NoFocus);

	connect(this, SIGNAL(clicked()), this, SLOT(onClicked()));
	connect(this, SIGNAL(pressed()), this, SLOT(OnPressed()));
	connect(this, SIGNAL(released()), this, SLOT(OnReleased()));
	SetImage();
}
void CPushButton::SetCmd(QString strPressCmd, QString strReleaseCmd)
{
	m_strPressCmd = strPressCmd;
	m_strReleaseCmd = strReleaseCmd;
}

void CPushButton::SetImage()
{
	QString qss = "QPushButton:!pressed{color:white;border-image:url(" GUI_PATH + m_strImg1 +
			")}:pressed{border-image:url(" GUI_PATH + m_strImg2 + ")}";
	setStyleSheet(qss);
}

void CPushButton::setVisible(bool visible)
{
	if(visible)
	{
		SetImage();
	}

	QPushButton::setVisible(visible);
}

void CPushButton::onClicked()
{

}

void CPushButton::OnPressed()
{
	//qDebug() << "button pressed\n";
	if (m_strPressCmd != "")
	{
		qDebug() << m_strPressCmd;
	}

}

void CPushButton::OnReleased()
{
	//qDebug() << "button released\n";
	if (m_strReleaseCmd != "")
	{
		qDebug() << m_strReleaseCmd;
	}

}

void CPushButton::focusInEvent(QFocusEvent *event)
{
	event = event;
	onClicked();
}
