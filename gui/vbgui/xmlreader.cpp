#include <stdio.h>
#include "xmlreader.h"
#include <QFile>
#include <QDebug>

CXmlReader::CXmlReader()
	: m_unDeep(0)
{
}

bool CXmlReader::ParseFile(const char *pFileName)
{
	QFile csFile(pFileName);
	if(csFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		setDevice(&csFile);

		//while(!atEnd() && !hasError())
		{
			ParseElem(NULL);
		}

		csFile.close();
	}

	return true;
}

QString CXmlReader::ParseElem(CElemObj *pObj)
{
	m_unDeep ++;

	readNext();//读取元素
	while(!atEnd() && !hasError())
	{
		//元素为开始标志，则递归调用自身
		if(isStartElement())
		{
			//早期处理
			//递归调用
			ParseElem(ProcElemEarly(pObj));

			//后期处理
			ProcElemLate(pObj);
		}
		//元素为结束标志
		else if(isEndElement())
		{
			m_unDeep --;
			QString ret = name().toString();
			return ret;
		}
		else if(isCharacters())
		{
			if(pObj)
			{
				pObj->m_strText = text().toString();
			}

			qDebug()<<"Call stack deep "<<m_unDeep<<" text:"<<text();
		}

		readNext();
	}

	m_unDeep --;
	QString ret = name().toString();
	return ret;
}

CElemObj *CXmlReader::ProcElemEarly(CElemObj *pObj)
{
	pObj = pObj;
	return NULL;
}

CElemObj *CXmlReader::ProcElemLate(CElemObj *pObj)
{
	pObj = pObj;
	return NULL;
}

