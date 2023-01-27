/*
	This project serves as a simple demonstration for the article "Combining Raw Input and keyboard Hook to selectively block input from multiple keyboards",
	which you should be able to find in this project folder (HookingRawInput.html), or on the CodeProject website (http://www.codeproject.com/).
	The project is based on the idea shown to me by Petr Medek (http://www.hidmacros.eu/), and is published with his permission, huge thanks to Petr!
	The source code is licensed under The Code Project Open License (CPOL), feel free to adapt it.
	Vít Blecha (sethest@gmail.com), 2014
*/

#pragma once

// #include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include "Console.h"

// Structure of a single record that will be saved in the decisionBuffer
struct DecisionRecord
{
	USHORT virtualKeyCode;
	BOOL decision;

	DecisionRecord (USHORT _virtualKeyCode, BOOL _decision) : virtualKeyCode (_virtualKeyCode), decision (_decision) {}
};
