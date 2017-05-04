#ifndef TRANSPARENTDIALOG_H
#define TRANSPARENTDIALOG_H

#include <QDialog>
#include "cmywindows.h"

namespace Ui {
class CTransparentDialog;
}

class CTransparentDialog : public CMyWindows
{
    Q_OBJECT
    
public:
    explicit CTransparentDialog(QWidget *parent = 0);
    ~CTransparentDialog();
    
private:
    Ui::CTransparentDialog *ui;

protected:
    void mousePressEvent(QMouseEvent *event);

public slots:
	void SwitchShow();

private:
	QWidget *m_pParent;

public:
	void SetParent(QWidget *pParent)
	{
		m_pParent = pParent;
	}


};

#endif // TRANSPARENTDIALOG_H
