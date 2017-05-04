#ifndef COMMON_DEFINE_H
#define COMMON_DEFINE_H
#if defined _WIN32 || defined _WIN64
#define GUI_PATH "E:/Work/LCD_Free_Combination/3520D/GUI/vbgui/"
#elif defined linux

//gcc -E -dM - </dev/null
#if defined __arm__
#define GUI_PATH    "/mnt/GUI/"
#else
#define GUI_PATH "/home/lyndon/workspace/videokeyboard/vbgui/"
#endif

#endif

#define SRC_SCREEN_HEIGHT	1080
#define SRC_SCREEN_WIDTH	1920

#endif // COMMON_DEFINE_H

