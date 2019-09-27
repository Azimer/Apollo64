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