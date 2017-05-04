#ifndef XMLREADER_H
#define XMLREADER_H

#include <QXmlStreamReader>
#include <elemobj.h>

class CXmlReader : virtual public QXmlStreamReader
{
public:
    CXmlReader();
    bool ParseFile(const char *pFileName);
    QString ParseElem(CElemObj *pObj);
protected:
    virtual CElemObj *ProcElemEarly(CElemObj *pObj);
    virtual CElemObj *ProcElemLate(CElemObj *pObj);
    unsigned int m_unDeep;
    CElemObj m_csRootElem;
};

#endif // XMLREADER_H
