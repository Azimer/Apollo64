/*
    Apollo N64 Emulator (c) Eclipse Productions
    Copyright (C) 2001 Azimer (azimer@emulation64.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef CONSOLE_DOT_H
#define CONSOLE_DOT_H
#include <windows.h>
/*												console.h
	Implements the Console Window

	Author			Date		Comment
	-----------		---------	----------------------------
	Azimer			12-04-99	Initial Version
*/

ATOM				RegisterConsoleClass (HINSTANCE hInstance); // Registers the Console Window
LRESULT CALLBACK	ConsoleProc(HWND, UINT, WPARAM, LPARAM); // Console Windows's main windows proc.
void				CreateConsoleWindow (bool shouldHide); // Makes a console window
void				ToggleConsoleWindow (); // Turns the console window on and off
void				DestroyConsoleWindow (); // Kills the Console Window
void				ClearConsoleWindow (); // Clears the Window
// dprintf is defined in WinMain to give access to other files but no access to the above functions
#endif