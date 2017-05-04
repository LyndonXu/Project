#include "nextbutton.h"
#include "mainwindow.h"
CNextButton::CNextButton(const QString &text, QWidget *parent, CElemObj *pObjParent) :
	CPushButton(text, parent, pObjParent)
{
}

void CNextButton::onClicked()
{
	MainWindow *pWid = (MainWindow *)parent();
	if(pWid)
	{
		pWid->m_strNextConfig = m_strNextConfig;
		pWid->close();
	}
}
