#include "radiobutton.h"
#include <QDebug>

CRadioButton::CRadioButton(const QString &text, QWidget *parent, CElemObj *pObjParent) :
	CCheckButton(text, parent, pObjParent),
	m_nRadioGroup(-1)
{
}

void CRadioButton::checkStateSet()
{
	CCheckButton::checkStateSet();

	if(isChecked())
	{
		if(m_pObjParent)
		{
			for(int i = 0; i < m_pObjParent->m_csWidQue.size(); ++ i)
			{
				QWidget *pBtn = (QWidget *)m_pObjParent->m_csWidQue.at(i);
				//qDebug()<<pBtn->metaObject()->className();
				if(pBtn)
				{
					if(pBtn->inherits("CRadioButton"))
					{
						CRadioButton *pRadio = (CRadioButton *)pBtn;
						if((pRadio != this) && (m_nRadioGroup == pRadio->m_nRadioGroup) && (m_nRadioGroup >= 0))
						{
							pRadio->setChecked(false);
						}
					}
				}
			}
		}
	}
}
