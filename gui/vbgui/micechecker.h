#ifndef MICECHECKER_H
#define MICECHECKER_H

#include <mouse.h>
#include <QObject>
#include <string>

class MiceChecker : public QObject
{
    Q_OBJECT
protected:
    CMouse m_csMouse;
    std::string m_strButton;
    bool m_boRunning;
public:
    MiceChecker();
    ~MiceChecker();
    friend void handler();
public slots:
    void Start();
    void Stop();
signals:
    void SigLeft();
    void SigRight();
    void SigMid();
    void SigWUp();
    void SigWDown();
public:
    virtual int OnButtonLeft();
    virtual int OnButtonRight();
    virtual int OnButtonMid();
    virtual int OnWheelUp();
    virtual int OnWheelDown();
};

#endif // MICECHECKER_H
