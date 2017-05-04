#ifndef CLABEL_H
#define CLABEL_H

#include "pushbutton.h"

class CLabel : public CPushButton
{
    Q_OBJECT
public:
    explicit CLabel(const QString &text, QWidget *parent = 0, CElemObj *pObjParent = 0);
    
signals:
    
public slots:
protected:
    virtual void SetImage();
};

#endif // CLABEL_H
