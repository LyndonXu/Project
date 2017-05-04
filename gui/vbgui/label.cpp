#include "label.h"
#include "common_define.h"

CLabel::CLabel(const QString &text, QWidget *parent, CElemObj *pObjParent) :
	CPushButton(text, parent, pObjParent)
{
	QObject::disconnect(this, SIGNAL(clicked()), this, SLOT(onClicked()));
	setDisabled(TRUE);
}

void CLabel::SetImage()
{
	QString qss = "QPushButton:!pressed{color:white;border-image:url(" GUI_PATH + m_strImg1 + ")}:pressed{border-image:url(" GUI_PATH + m_strImg2 + ")}";
	setStyleSheet(qss);
}
