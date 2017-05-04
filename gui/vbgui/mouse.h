#ifndef MOUSE_H
#define MOUSE_H

#define WHEEL_UP 0x10
#define WHEEL_DOWN 0x08
#define BUTTON_L 0x01
#define BUTTON_R 0x03
#define BUTTON_M 0x02

class CMouse
{
protected:
    int m_s32Mouse;
    unsigned int m_u32RcvCnt;
    unsigned char m_u8Buf[4];
public:
    CMouse();
    ~CMouse();
    int Read(int &x, int &y, int &button, int &b);
};

#endif // MOUSE_H
