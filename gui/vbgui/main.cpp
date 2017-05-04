#include <QtGui/QApplication>
#include "mainwindow.h"
#include <QTextCodec>
#include <QThread>
#include <QFile>
#include <QString>
#include <QDebug>
#include <QLabel>

#include "common_define.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QString strCfgName = GUI_PATH "default.xml";
	MainWindow *w;
	int ret = -1;

	qDebug() << strCfgName;

	while(1)
	{
		bool boIsCfgValid = true;
		if(strCfgName == "")
		{
			boIsCfgValid = false;
		}
		if (boIsCfgValid)
		{
			QFile csFile(strCfgName);
			if(csFile.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				csFile.close();
			}
			else
			{
				boIsCfgValid = false;
			}
		}


		w = new MainWindow(strCfgName);
		if (w == NULL)
		{
			break;
		}

		QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));

		w->show();
		w->SetDefault();
		if (!boIsCfgValid)
		{
			QLabel *pLabel = new QLabel;
			if (pLabel != NULL)
			{
				pLabel->setText("config file maybe is error or not exsit");
				pLabel->setParent(w);
				pLabel->show();
			}
		}
		ret = a.exec();

		if (w->m_strNextConfig == "")
		{
			boIsCfgValid = false;
		}
		else
		{
			strCfgName = GUI_PATH;
			strCfgName += w->m_strNextConfig;
		}

		delete w;
		if (!boIsCfgValid)
		{
			break;
		}
	}

	return ret;
}
