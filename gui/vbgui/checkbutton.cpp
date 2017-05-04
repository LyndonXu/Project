#include "checkbutton.h"
#include <QDebug>
#include "common_define.h"

CCheckButton::CCheckButton(const QString &text, QWidget *parent, CElemObj *pObjParent) :
	CPushButton(text, parent, pObjParent)
{
	setCheckable(true);
}

void CCheckButton::checkStateSet()
{
	CPushButton::checkStateSet();

	SetImage();
}

void CCheckButton::SetImage()
{
	QString qss = "";

	if(isChecked())
	{
		if(m_strImg2 != "")
		{
			qss = "QPushButton{color:white;border-image:url(" GUI_PATH + m_strImg2 + ")}";
		}
	}
	else
	{
		if(m_strImg1 != "")
		{
			qss = "QPushButton{color:white;border-image:url(" GUI_PATH + m_strImg1 + ")}";
		}
	}

	if(qss != "")
	{
		setStyleSheet(qss);
	}
}

void CCheckButton::onClicked()
{
	setChecked(isChecked());
}
