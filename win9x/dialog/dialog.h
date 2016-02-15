/**
 * @file	dialog.h
 * @breif	ダイアログの宣言
 */

#pragma once

void dialog_newdisk(HWND hWnd);
void dialog_changefdd(HWND hWnd, REG8 drv);
void dialog_changehdd(HWND hWnd, REG8 drv);

// d_about.cpp
void dialog_about(HWND hwndParent);

// d_bmp.cpp
void dialog_writebmp(HWND hWnd);

// d_clnd.cpp
void dialog_calendar(HWND hwndParent);

// d_config.cpp
void dialog_configure(HWND hwndParent);

// d_font.cpp
void dialog_font(HWND hWnd);

// d_mpu98.cpp
void dialog_mpu98(HWND hwndParent);

// d_soundlog.cpp
void dialog_soundlog(HWND hWnd);

void dialog_scropt(HWND hWnd);
void dialog_sndopt(HWND hWnd);
void dialog_serial(HWND hWnd);
