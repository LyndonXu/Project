#ifndef TAP_H
#define TAP_H

#include <radiobutton.h>

class CTap : public CRadioButton
{
Q_OBJECT
public:
    explicit CTap(const QString &text, QWidget *parent = 0, CElemObj *pObjParent = NULL);
    virtual ~CTap();
    void ShowSubItem();
    void HideSubItem();
protected:
    virtual void checkStateSet();
signals:

public slots:
    virtual void onClicked();
    virtual void setVisible(bool visible);
};

#endif // TAP_H
