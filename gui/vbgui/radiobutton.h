#ifndef RADIOBUTTON_H
#define RADIOBUTTON_H

#include <checkbutton.h>

class CRadioButton : public CCheckButton
{
Q_OBJECT
public:
    explicit CRadioButton(const QString &text, QWidget *parent = 0, CElemObj *pObjParent = 0);
    int m_nRadioGroup;
protected:
    virtual void checkStateSet();
signals:

public slots:

};

#endif // RADIOBUTTON_H
