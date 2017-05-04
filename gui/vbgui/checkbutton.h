#ifndef CHECKBUTTON_H
#define CHECKBUTTON_H

#include <pushbutton.h>

class CCheckButton : public CPushButton
{
Q_OBJECT
public:
    explicit CCheckButton(const QString &text, QWidget *parent = 0, CElemObj *pObjParent = 0);
protected:
    virtual void checkStateSet();
    virtual void SetImage();
signals:

public slots:
    virtual void onClicked();
};

#endif // CHECKBUTTON_H
