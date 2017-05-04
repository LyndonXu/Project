#include "micechecker.h"
#include <QThread>
#include <unistd.h>
#include <signal.h>

bool boTiming = false;
bool boLongPress = false;

void handler(int)
{
    if(boTiming)
    {
        boLongPress = true;
    }
    boTiming = false;
}

MiceChecker::MiceChecker()
    : m_strButton("")
    , m_boRunning(false)
{
    signal(SIGALRM, handler);
}

MiceChecker::~MiceChecker()
{

}

void MiceChecker::Start()
{
    QThread *pThd;
    pThd = thread();
    if(!pThd)
    {
        qDebug("Thread is null !\n");
    }

    m_boRunning = true;

    int x, y, button, b;
    while(m_boRunning)
    {
        if(m_csMouse.Read(x, y, button, b))
        {
//            qDebug("Read mouse devices faield!\n");
        }
        else
        {
            m_strButton = "";
            if((button & 3) == BUTTON_L)
            {
                emit SigLeft();
                OnButtonLeft();
            }
            if((button & 3) == BUTTON_M)
            {
                emit SigMid();
                OnButtonMid();
            }
            if((button & 3) == BUTTON_R)
            {
                emit SigRight();
                OnButtonRight();
            }
            if((button & 0x18) == WHEEL_UP)
            {
                emit SigWUp();
                OnWheelUp();
            }
            if((button & 0x18) == WHEEL_DOWN)
            {
                emit SigWDown();
                OnWheelDown();
            }
            if((!button) & (!b))
            {
                if(boLongPress)
                {
                    emit SigRight();
                    boLongPress = false;
                }

                boTiming = false;
            }

            qDebug("x: %d, y: %d, button: %d-%s, %d\n", x, y, button, m_strButton.c_str(), b);
        }
    }

    qDebug("MiceChecker has gone!\n");
}

void MiceChecker::Stop()
{
    m_boRunning = false;
}

int MiceChecker::OnButtonLeft()
{
    if(!boTiming)
    {
        boTiming = true;
        alarm(3);
    }

    if("" != m_strButton)
    {
        m_strButton += " | ";
    }
    m_strButton += "left";

    return 0;
}

int MiceChecker::OnButtonRight()
{
    if("" != m_strButton)
    {
        m_strButton += " | ";
    }
    m_strButton += "right";

    return 0;
}

int MiceChecker::OnButtonMid()
{
    if("" != m_strButton)
    {
        m_strButton += " | ";
    }
    m_strButton += "middle";

    return 0;
}

int MiceChecker::OnWheelUp()
{
    if("" != m_strButton)
    {
        m_strButton += " | ";
    }
    m_strButton += "wheel up";

    return 0;
}

int MiceChecker::OnWheelDown()
{
    if("" != m_strButton)
    {
        m_strButton += " | ";
    }
    m_strButton += "wheel down";

    return 0;
}
