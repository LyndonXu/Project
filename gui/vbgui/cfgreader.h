#ifndef CFGREADER_H
#define CFGREADER_H

#include <QWidget>
#include <QVBoxLayout>
#include <QGridLayout>
#include <xmlreader.h>

class CCfgReader : public CXmlReader
{
public:
    CCfgReader(QWidget *pParent);
    void SetDefault();
    QString m_strNextConfig;
protected:
    virtual CElemObj *ProcElemEarly(CElemObj *pObj);
    virtual CElemObj *ProcElemLate(CElemObj *pObj);
    CElemObj m_csRootElem;
    QWidget *m_pParent;
    void SetWndBackground(QWidget *pWnd, QString strImg);
    CElemObj * OnTap(CElemObj *pObj, QXmlStreamAttributes &attr);
    CElemObj * OnCheck(CElemObj *pObj, QXmlStreamAttributes &attr);
    CElemObj * OnRadio(CElemObj *pObj, QXmlStreamAttributes &attr);
    CElemObj * OnNext(CElemObj *pObj, QXmlStreamAttributes &attr);
    CElemObj * OnButton(CElemObj *pObj, QXmlStreamAttributes &attr);
    CElemObj * OnBackground(CElemObj *pObj, QXmlStreamAttributes &attr);
    CElemObj * OnWelcome(CElemObj *pObj, QXmlStreamAttributes &attr);
    CElemObj * OnGUI(CElemObj *pObj, QXmlStreamAttributes &attr);
    CElemObj * OnGroup(CElemObj *pObj, QXmlStreamAttributes &attr);
    CElemObj * OnLabel(CElemObj *pObj, QXmlStreamAttributes &attr);

private:
    QRect TransSize(QXmlStreamAttributes &attr);
};

#endif // CFGREADER_H
