#include "tap.h"
#include <QDebug>

CTap::CTap(const QString &text, QWidget *parent, CElemObj *pObjParent):
	CRadioButton(text, parent, pObjParent)
{
}

CTap::~CTap()
{
	qDebug()<<"exit\r\n"<<this->text();
}

void CTap::onClicked()
{
	if(isCheckable())
	{
		setChecked(isChecked());
	}
}

void CTap::checkStateSet()
{
	CRadioButton::checkStateSet();

	if(isVisible() && isChecked())
	{
		ShowSubItem();
	}
	else
	{
		HideSubItem();
	}
}

void CTap::setVisible(bool visible)
{
	CPushButton::setVisible(visible);
	if(visible && isChecked())
	{
		ShowSubItem();
	}
	else
	{
		HideSubItem();
	}
}

void CTap::ShowSubItem()
{
	for(int i = 0; i < m_csWidQue.size(); ++i)
	{
		QWidget *pWid = (QWidget *)m_csWidQue.at(i);
		if(pWid)
		{
			pWid->show();
		}
	}
}

void CTap::HideSubItem()
{
	for(int i = 0; i < m_csWidQue.size(); ++i)
	{
		QWidget *pWid = (QWidget *)m_csWidQue.at(i);
		if(pWid)
		{
			pWid->hide();
		}
	}
}
