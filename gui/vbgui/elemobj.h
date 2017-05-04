#ifndef ELEMOBJ_H
#define ELEMOBJ_H
#include <QString>
#include <QQueue>
#include <QWidget>

class CElemObj
{
public:
    CElemObj(CElemObj *pRoot = NULL);
    virtual ~CElemObj();
    QString m_strText;
    QQueue<QWidget *>m_csWidQue;
    CElemObj *m_pObjParent;
};

#endif // ELEMOBJ_H
