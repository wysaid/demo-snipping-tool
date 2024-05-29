/**
 * @file demo.cpp
 * @author wysaid (this@xege.org)
 * @brief 展示如何使用 EGE 实现一个简单的截图程序.
 */

#include <graphics.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

int main()
{
    SetProcessDPIAware(); /// 忽略系统 DPI 设定, 方便截图.

    setinitmode(INIT_NOBORDER | INIT_TOPMOST);
    /// 获取屏幕 HDC
    HDC screenHDC = GetDC(NULL);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);  /// 逻辑屏幕宽度
    int screenHeight = GetSystemMetrics(SM_CYSCREEN); /// 逻辑屏幕高度

    /// 一开始只使用一个极小分辨率, 因为要立即隐藏. 避免屏幕闪烁.
    initgraph(1, 1);
    HWND windowHwnd = getHWnd();
    /// 隐藏窗口
    ShowWindow(windowHwnd, SW_HIDE);

    setrendermode(RENDER_MANUAL);

    PIMAGE img = newimage(screenWidth, screenHeight);
    HDC imgHDC = getHDC(img);

    /// 从屏幕截图
    BitBlt(imgHDC, 0, 0, screenWidth, screenHeight, screenHDC, 0, 0, SRCCOPY);

    /// 释放资源, 但是截图程序只执行一次就退出, 所以在这个场景下其实不释放也没事.
    // ReleaseDC(NULL, screenHDC);

    resizewindow(screenWidth, screenHeight);

    putimage_alphablend(NULL, img, 0, 0, 0x50, 0, 0, screenWidth, screenHeight);

    /// 强制刷新一下屏幕.
    Sleep(0);
    /// 先绘制一次, 再显示窗口, 避免闪屏
    ShowWindow(windowHwnd, SW_SHOW);

    bool leftDown = false;
    bool hasSelection = false;
    int startX = 0, startY = 0;
    int endX = 0, endY = 0;
    int lastMouseX = 0, lastMouseY = 0;

    /// 这是一个截屏用的程序, 不应该频繁刷新.
    /// 这里设置一个更新标记, 防止过度刷新空耗cpu.
    bool shouldUpdate = false;

    /// 设置一下提示字体
    setbkmode(TRANSPARENT);
    setfont(20, 10, "宋体");

    /// 主循环
    for (; is_run(); delay_fps(60))
    {
        while (mousemsg())
        {
            mouse_msg msg = getmouse();

            shouldUpdate = true;
            switch (msg.msg)
            {
            case mouse_msg_down:
                if (msg.is_left())
                {
                    leftDown = true;
                    lastMouseX = msg.x;
                    lastMouseY = msg.y;

                    // 如果新的鼠标点击位置在之前的选区内, 则认为是移动选区.
                    if (hasSelection && msg.x >= startX && msg.x <= endX && msg.y >= startY && msg.y <= endY)
                    {
                        break;
                    }

                    startX = msg.x;
                    startY = msg.y;
                    endX = msg.x;
                    endY = msg.y;
                    hasSelection = false;
                }
                else if (msg.is_right())
                {
                    /// 按下鼠标右键, 如果有选区, 则去掉选区, 否则直接退出程序.
                    if (hasSelection)
                    {
                        hasSelection = false;
                        startX = endX = startY = endY = 0;
                        continue;
                    }
                    return 0;
                }
                else if (msg.is_mid())
                { /// 按下鼠标中键, 执行截图保存操作
                    if (hasSelection)
                    { /// 弹窗保存截图.
                        int w = abs(endX - startX);
                        int h = abs(endY - startY);
                        int left = MIN(startX, endX);
                        int top = MIN(startY, endY);
                        PIMAGE selection = newimage(w, h);
                        getimage(selection, img, left, top, w, h);

                        char filename[MAX_PATH] = { 0 };
                        OPENFILENAME ofn = { 0 };
                        ofn.lStructSize = sizeof(ofn);
                        ofn.hwndOwner = windowHwnd;
                        ofn.lpstrFile = filename;
                        ofn.nMaxFile = sizeof(filename);
                        ofn.lpstrFilter = "PNG\0*.png\0";
                        ofn.lpstrTitle = "保存截图";
                        ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;
                        if (GetSaveFileName(&ofn))
                        {
                            // 如果filename后缀不是 .png, 则自动加上.
                            char* ext = strrchr(filename, '.');
                            if (!ext || strcmp(ext, ".png") != 0)
                            {
                                strncat(filename, ".png", sizeof(filename) - strlen(filename) - 1);
                            }
                            savepng(selection, filename);
                        }
                        return 0;
                    }
                }
                break;
            case mouse_msg_up:
                if (msg.is_left())
                {
                    leftDown = false;
                    if (endX != startX && endY != startY)
                        hasSelection = true;
                }

                break;
            case mouse_msg_move:
                if (leftDown)
                {
                    if (hasSelection)
                    {
                        int offsetX = msg.x - lastMouseX;
                        int offsetY = msg.y - lastMouseY;
                        startX += offsetX;
                        startY += offsetY;
                        endX += offsetX;
                        endY += offsetY;
                    }
                    else
                    {
                        endX = msg.x;
                        endY = msg.y;
                    }
                    lastMouseX = msg.x;
                    lastMouseY = msg.y;
                }
                break;
            }
        }

        while (kbhit())
        { /// 键盘按下 esc 键, 退出程序.
            if (getch() == key_esc)
            {
                return 0;
            }
        }

        if (shouldUpdate)
        {
            shouldUpdate = false;
            cleardevice();
            setviewport(0, 0, screenWidth, screenHeight);
            putimage_alphablend(NULL, img, 0, 0, 0x50, 0, 0, screenWidth, screenHeight);

            /// 绘制一个红色的矩形框, 显示整个屏幕.
            setcolor(RED);
            setlinewidth(4);
            rectangle(2, 2, screenWidth - 4, screenHeight - 4);

            if (endX == startX || endY == startY)
                continue; /// 无需绘制选区.

            int w = abs(endX - startX);
            int h = abs(endY - startY);
            int left = MIN(startX, endX);
            int top = MIN(startY, endY);
            putimage(left, top, w, h, img, left, top, w, h);
            /// 绘制一个白色的矩形框, 显示选区.
            setcolor(WHITE);
            setlinewidth(2);
            rectangle(left, top, left + w, top + h);

            /// 绘制一个提示词
            setcolor(YELLOW);
            outtextxy(left, top + h, "按ESC键退出, 右键取消, 按下鼠标中键(滚轮)弹窗保存.");
        }
    }

    return 0;
}