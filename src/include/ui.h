//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      ui.h
//
// Description:
//
//      Provides a user interface over the serial port.
//
// Author:
//
//      Mike Kirkland
//
// Created:
//
//      October 2001
//
////////////////////////////////////////////////////////////////////////////////

#ifndef UI_H
#define UI_H

#include <parser.h>

#define MAX_COMMAND_SIZE           256
#define UI_TIMEOUT                 2

void prompt_ui(mode cur_mode);
void init_ui(int timeout, mode cur_mode);

#endif //UI_H
