#include "mouse.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <string.h>

#define MOUSE_DEV "/dev/input/mice"

CMouse::CMouse()
    : m_s32Mouse(-1)
    , m_u32RcvCnt(0)
{
    m_u8Buf[0] = 0;
    m_u8Buf[1] = 0;
    m_u8Buf[2] = 0;
    m_u8Buf[3] = 0;
    const unsigned char u8Param[] = {243, 200, 243, 100, 243, 80};
    m_s32Mouse = open(MOUSE_DEV, O_RDWR);
    if(m_s32Mouse < 0)
    {
        m_s32Mouse = open(MOUSE_DEV, O_RDONLY);;
        if(m_s32Mouse < 0)
        {
            printf("Open mouse devices faield %s!\n", strerror(errno));
        }
    }
    else
    {
        int s32Ret = write(m_s32Mouse, u8Param, sizeof(u8Param));
        if(s32Ret < 0)
        {
            printf("Init mouse devices failed!\n");
        }
    }
}

CMouse::~CMouse()
{
    if(m_s32Mouse < 0)
    {
        return;
    }
    close(m_s32Mouse);
    m_s32Mouse = -1;
}

int CMouse::Read(int &x, int &y, int &button, int &b)
{
    const int s32ButtonTable[] = {0, 1, 3, 0, 2, 0, 0};
    int n = 0;

    while(1)
    {
        fd_set fd;
        FD_ZERO(&fd);
        FD_SET(m_s32Mouse, &fd);
        struct timeval stTimeOut = {0, 500};
        int s32Ret = select(m_s32Mouse + 1, &fd, NULL, NULL, &stTimeOut);
        if(s32Ret < 0)
        {
            printf("Read mouse deivces failed with err %d\n", errno);
        }
        else if(s32Ret == 0)
        {
            return -1;
        }
        else if(!FD_ISSET(m_s32Mouse, &fd))
        {
            return -1;
        }

        n = read(m_s32Mouse, m_u8Buf + m_u32RcvCnt, 4 - m_u32RcvCnt);
        if(n < 0)
        {
            int err = errno;
            if(err == EINTR)
            {
                printf("Read mouse devices again!\n");
                continue;
            }
            else
            {
                printf("Read mouse devices failed with err %d\n", err);
                return -1;
            }
        }
        else if(n == 0)
        {
            break;
        }
        m_u32RcvCnt += n;
        if(m_u32RcvCnt == 4)
        {
            int s32Wheel;
            if(m_u8Buf[0] & 0xc0)
            {
                m_u8Buf[0] = m_u8Buf[1];
                m_u8Buf[1] = m_u8Buf[2];
                m_u8Buf[2] = m_u8Buf[3];
                m_u32RcvCnt = 3;
                printf("Read mouse devices need sync!\n");
                return -1;
            }

            /* FORM XFree86 4.0.1 */
            b = m_u8Buf[0] & 0x7;
            button = s32ButtonTable[(m_u8Buf[0] & 0x07)];
            x = (m_u8Buf[0] & 0x10) ? m_u8Buf[1] - 256 : m_u8Buf[1];
            y = (m_u8Buf[0] & 0x20) ? - (m_u8Buf[2] - 256) : - m_u8Buf[2];
            /* Is a wheel event? */
            if((s32Wheel = m_u8Buf[3]) != 0)
            {
                if(s32Wheel > 0x7f)
                {
                    button |= WHEEL_UP;
                }
                else
                {
                    button |= WHEEL_DOWN;
                }
            }

            m_u32RcvCnt = 0;
            return 0;
        }
    }

    if(n == 0)
    {
        printf("Can not read mouse devices!\n");
    }

    return -1;
}
