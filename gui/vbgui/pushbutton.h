#ifndef PUSHBUTTON_H
#define PUSHBUTTON_H

#include <elemobj.h>
#include <QPushButton>

class CPushButton : public QPushButton, public CElemObj
{
Q_OBJECT
public:
    explicit CPushButton(const QString &text, QWidget *parent = 0, CElemObj *pObjParent = 0);
    QString m_strImg1;
    QString m_strImg2;
	QString m_strPressCmd;
	QString m_strReleaseCmd;
	void SetCmd(QString strPressCmd, QString strReleaseCmd);
protected:
    virtual void SetImage();
signals:
public slots:
	void OnPressed();
	void OnReleased();

    virtual void setVisible(bool visible);
    virtual void onClicked();
    virtual void focusInEvent(QFocusEvent *event);
};

#endif // PUSHBUTTON_H
