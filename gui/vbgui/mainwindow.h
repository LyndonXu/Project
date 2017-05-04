#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "transparentdialog.h"
#include <QMainWindow>
#include <cfgreader.h>
#include "cmywindows.h"

namespace Ui {
    class MainWindow;
}

class CTransparentDialog;

class MainWindow : public CMyWindows {
    Q_OBJECT
public:
    MainWindow(QString strCfg, QWidget *parent = 0);
    ~MainWindow();
    void EmitSigMouseChecker(bool boIsStart);
    void SetDefault();
    QString m_strNextConfig;
protected:
    void changeEvent(QEvent *e);
    void mouseDoubleClickEvent(QMouseEvent * event);
//    void resizeEvent(QResizeEvent *event);
private:
    Ui::MainWindow *ui;
public slots:
	void SwitchShow();
signals:
    void SigMCStart();
    void SigMCStop();
protected:
    CCfgReader m_csCfgReader;
    CTransparentDialog m_csTouchRcver;
};

#endif // MAINWINDOW_H
