#ifndef NEXTBUTTON_H
#define NEXTBUTTON_H

#include <pushbutton.h>

class CNextButton : public CPushButton
{
Q_OBJECT
public:
    CNextButton(const QString &text, QWidget *parent = 0, CElemObj *pObjParent = 0);
    QString m_strNextConfig;
signals:
    void SetNextConfig(QString strNextConfig);
public slots:
    virtual void onClicked();
};

#endif // NEXTBUTTON_H
