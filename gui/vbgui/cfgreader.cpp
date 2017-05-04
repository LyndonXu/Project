#include "cfgreader.h"
#include <nextbutton.h>
#include <QDebug>
#include <tap.h>
#include <label.h>
#include <QBitmap>
#include <QSplashScreen>
#include <QElapsedTimer>
#include <QApplication>
#include <QDesktopWidget>
#include <QGroupBox>

#include <unistd.h>
#include "common_define.h"

CCfgReader::CCfgReader(QWidget *pParent) :
	m_strNextConfig("")
{
	m_pParent = pParent;
}

CElemObj *CCfgReader::ProcElemEarly(CElemObj *pObj)
{
	CElemObj *pRet = NULL;

	qDebug()<<"call stack deep"<<m_unDeep<<"<"<<name()<<">";

	QStringRef strName = name();
	QXmlStreamAttributes attr = attributes();
	if("tap" == strName)
	{
		pRet = OnTap(pObj, attr);
	}
	else if("check" == strName)
	{
		pRet = OnCheck(pObj, attr);
	}
	else if("radio" == strName)
	{
		pRet = OnRadio(pObj, attr);
	}
	else if("next" == strName)
	{
		pRet = OnNext(pObj, attr);
	}
	else if("button" == strName)
	{
		pRet = OnButton(pObj, attr);
	}
	else if("background" == strName)
	{
		pRet = OnBackground(pObj, attr);
	}
	else if("welcome" == strName)
	{
		pRet = OnWelcome(pObj, attr);
	}
	else if("gui" == strName)
	{
		pRet = OnGUI(pObj, attr);
	}
	else if("group" == strName)
	{
		pRet = OnGroup(pObj, attr);
	}
	else if("label" == strName)
	{
		pRet = OnLabel(pObj, attr);
	}

	return pRet;
}

QRect CCfgReader::TransSize(QXmlStreamAttributes &attr)
{
#if 0
	QDesktopWidget* desktop = QApplication::desktop();
	int x = attr.value("x").toString().toUInt() * desktop->width() / 1920;
	int y = attr.value("y").toString().toUInt() * desktop->height() / 1080;
	int w = attr.value("w").toString().toUInt() * desktop->width() / 1920;
	int h = attr.value("h").toString().toUInt() * desktop->height() / 1080;
#else
	int x = attr.value("x").toString().toUInt() * m_pParent->width() / SRC_SCREEN_WIDTH;
	int y = attr.value("y").toString().toUInt() * m_pParent->height() / SRC_SCREEN_HEIGHT;
	int w = attr.value("w").toString().toUInt() * m_pParent->width() / SRC_SCREEN_WIDTH;
	int h = attr.value("h").toString().toUInt() * m_pParent->height() / SRC_SCREEN_HEIGHT;
#endif
	QRect csRect(x, y, w, h);

	qDebug()<<"rect="<<csRect;

	return csRect;
}

CElemObj * CCfgReader::OnGroup(CElemObj *pObj, QXmlStreamAttributes &attr)
{
	if(!pObj)
	{
		return NULL;
	}

	QGroupBox *pGroupBox = new QGroupBox(m_pParent);
	if(pGroupBox)
	{
		pGroupBox->setFocusPolicy(Qt::NoFocus);
		QString qss = "QGroupBox{border-width:2px;border-style:solid;border-color:lightGray;margin-top:1ex;font-weight:bold}" \
					  "QGroupBox::title{subcontrol-origin:margin;position:relative;left:12px;color:white;}";
		pGroupBox->setStyleSheet(qss);
		pGroupBox->setTitle(attr.value("name").toString());
#if 0
		pGroupBox->setGeometry(attr.value("x").toString().toUInt(),
							   attr.value("y").toString().toUInt(),
							   attr.value("w").toString().toUInt(),
							   attr.value("h").toString().toUInt());
#else
		pGroupBox->setGeometry(TransSize(attr));
#endif
		pObj->m_csWidQue.enqueue(pGroupBox);
	}
	return NULL;
}

CElemObj * CCfgReader::OnTap(CElemObj *pObj, QXmlStreamAttributes &attr)
{
	qDebug()<<"name="<<attr.value("name");

	CTap *pGB = new CTap(attr.value("name").toString(), m_pParent, pObj);
	//CTap *pGB = new CTap("", m_pParent, pObj);
	if(pGB)
	{
#if 0
		pGB->setGeometry(attr.value("x").toString().toUInt(),
						 attr.value("y").toString().toUInt(),
						 attr.value("w").toString().toUInt(),
						 attr.value("h").toString().toUInt());
#else
		pGB->setGeometry(TransSize(attr));
#endif
		pGB->m_strImg1 = attr.value("image1").toString();
		pGB->m_strImg2 = attr.value("image2").toString();

		pGB->m_nRadioGroup = -1;
		pGB->setChecked(false);

		if(attr.value("radioid").toString() != "")
		{
			pGB->m_nRadioGroup = attr.value("radioid").toString().toInt();
		}

		if(pObj)
		{
			//pObj->m_csElemQueue.enqueue(pGB);
			pObj->m_csWidQue.enqueue(pGB);
		}

		return pGB;
	}

	return NULL;
}

CElemObj * CCfgReader::OnCheck(CElemObj *pObj, QXmlStreamAttributes &attr)
{
	if(!pObj)
	{
		return NULL;
	}

	qDebug()<<"name="<<attr.value("name");

	CCheckButton *pGB = new CCheckButton(attr.value("name").toString(), m_pParent, pObj);
	if(pGB)
	{
#if 0
		pGB->setGeometry(attr.value("x").toString().toUInt(),
						 attr.value("y").toString().toUInt(),
						 attr.value("w").toString().toUInt(),
						 attr.value("h").toString().toUInt());
#else
		pGB->setGeometry(TransSize(attr));
#endif

		pGB->m_strImg1 = attr.value("image1").toString();
		pGB->m_strImg2 = attr.value("image2").toString();
		pGB->SetCmd(attr.value("press_cmd").toString(), attr.value("release_cmd").toString());

		pObj->m_csWidQue.enqueue(pGB);
	}

	return NULL;
}

CElemObj * CCfgReader::OnRadio(CElemObj *pObj, QXmlStreamAttributes &attr)
{
	if(!pObj)
	{
		return NULL;
	}

	qDebug()<<"name="<<attr.value("name");

	CRadioButton *pGB = new CRadioButton(attr.value("name").toString(), m_pParent, pObj);
	if(pGB)
	{
#if 0
		pGB->setGeometry(attr.value("x").toString().toUInt(),
						 attr.value("y").toString().toUInt(),
						 attr.value("w").toString().toUInt(),
						 attr.value("h").toString().toUInt());
#else
		pGB->setGeometry(TransSize(attr));
#endif

		pGB->m_strImg1 = attr.value("image1").toString();
		pGB->m_strImg2 = attr.value("image2").toString();

		if(attr.value("radioid").toString() != "")
		{
			pGB->m_nRadioGroup = attr.value("radioid").toString().toInt();
		}
		pGB->SetCmd(attr.value("press_cmd").toString(), attr.value("release_cmd").toString());

		pObj->m_csWidQue.enqueue(pGB);
	}

	return NULL;
}

CElemObj * CCfgReader::OnNext(CElemObj *pObj, QXmlStreamAttributes &attr)
{
	if(!pObj)
	{
		return NULL;
	}

	qDebug()<<"name="<<attr.value("name");

	CNextButton *pGB = new CNextButton(attr.value("name").toString(), m_pParent, pObj);
	//CTap *pGB = new CTap("", m_pParent, pObj);
	if(pGB)
	{
#if 0
		pGB->setGeometry(attr.value("x").toString().toUInt(),
						 attr.value("y").toString().toUInt(),
						 attr.value("w").toString().toUInt(),
						 attr.value("h").toString().toUInt());
#else
		pGB->setGeometry(TransSize(attr));
#endif

		pGB->m_strImg1 = attr.value("image1").toString();
		pGB->m_strImg2 = attr.value("image2").toString();

		pGB->m_strNextConfig = attr.value("config").toString();

		pObj->m_csWidQue.enqueue(pGB);

		return pGB;
	}

	return NULL;
}

CElemObj * CCfgReader::OnButton(CElemObj *pObj, QXmlStreamAttributes &attr)
{
	if(!pObj)
	{
		return NULL;
	}

	qDebug()<<"name="<<attr.value("name");

	CPushButton *pGB = new CPushButton(attr.value("name").toString(), m_pParent, pObj);
	//CTap *pGB = new CTap("", m_pParent, pObj);
	if(pGB)
	{
#if 0
		pGB->setGeometry(attr.value("x").toString().toUInt(),
						 attr.value("y").toString().toUInt(),
						 attr.value("w").toString().toUInt(),
						 attr.value("h").toString().toUInt());
#else
		pGB->setGeometry(TransSize(attr));
#endif

		pGB->m_strImg1 = attr.value("image1").toString();
		pGB->m_strImg2 = attr.value("image2").toString();

		if(attr.value("event").toString() == "quit")
		{
			if(m_pParent)
			{
				QObject::connect(pGB, SIGNAL(clicked()), m_pParent, SLOT(close()), Qt::QueuedConnection);
			}
		}
		pGB->SetCmd(attr.value("press_cmd").toString(), attr.value("release_cmd").toString());

		pObj->m_csWidQue.enqueue(pGB);

		return pGB;
	}

	return NULL;
}

CElemObj * CCfgReader::OnLabel(CElemObj *pObj, QXmlStreamAttributes &attr)
{
	if(!pObj)
	{
		return NULL;
	}

	qDebug() << "name=" << attr.value("name");

	CPushButton *pGB = new CLabel(attr.value("name").toString(), m_pParent, pObj);
	if(pGB)
	{
#if 0
		pGB->setGeometry(attr.value("x").toString().toUInt(),
						 attr.value("y").toString().toUInt(),
						 attr.value("w").toString().toUInt(),
						 attr.value("h").toString().toUInt());
#else
		pGB->setGeometry(TransSize(attr));
#endif

		pGB->m_strImg1 = attr.value("image1").toString();
		pGB->m_strImg1 = attr.value("image").toString();

		pObj->m_csWidQue.enqueue(pGB);

		return pGB;
	}

	return NULL;
}


CElemObj * CCfgReader::OnBackground(CElemObj *pObj, QXmlStreamAttributes &attr)
{
	pObj = pObj;
	SetWndBackground(m_pParent, attr.value("name").toString());

	return NULL;
}

CElemObj * CCfgReader::OnWelcome(CElemObj *pObj, QXmlStreamAttributes &attr)
{
	pObj = pObj;
	if("" == attr.value("image").toString())
	{
		return NULL;
	}

	QPixmap pixmap(attr.value("image").toString());
	if(pixmap.isNull())
	{
		return NULL;
	}
	QSplashScreen screen(pixmap);

	screen.show();
	//screen.showMessage(QObject::tr(""), Qt::AlignCenter, Qt::red);

	unsigned int unDelay = 1;
	if("" != attr.value("delay").toString())
	{
		unDelay = attr.value("delay").toString().toUInt();
	}
	QElapsedTimer timer;
	timer.start();
	while(timer.elapsed() < (unDelay * 1000))
	{
		qApp->processEvents();
	}

	screen.finish(m_pParent);
	return NULL;
}

CElemObj * CCfgReader::OnGUI(CElemObj *pObj, QXmlStreamAttributes &attr)
{
	pObj = pObj;
	attr = attr;
	return &m_csRootElem;
}

void CCfgReader::SetWndBackground(QWidget *pWnd, QString strImg)
{
	if(("" != strImg) && (pWnd))
	{
#if 1
		pWnd->setAutoFillBackground(true);
		QPalette palette;
		QString csStr = GUI_PATH;
		csStr += strImg;
		QPixmap pixmap(csStr);
		palette.setBrush(QPalette::Window, QBrush(pixmap.scaled(pWnd->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
		pWnd->setPalette(palette);
#else
		QString csStr = "border-image: url(";
		csStr += GUI_PATH;
		csStr += strImg;
		csStr += ");";

		pWnd->setStyleSheet(csStr);
#endif
	}
}

CElemObj *CCfgReader::ProcElemLate(CElemObj *pObj)
{
	pObj = pObj;
	qDebug()<<"call stack deep"<<m_unDeep<<"</"<<name()<<">";

	QStringRef strName = name();
	if("button" == strName)
	{

	}

	return NULL;
}

void CCfgReader::SetDefault()
{
	for(int i = 0; i < m_csRootElem.m_csWidQue.size(); i ++)
	{
		QWidget *pBtn = (QWidget *)m_csRootElem.m_csWidQue.at(i);
		if(pBtn)
		{
			if(pBtn->inherits("CTap"))
			{
				((CTap *)pBtn)->setChecked(true);
				break;
			}
		}
	}
}
