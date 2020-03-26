/*
 * This file is designed to support RMT hardware driver on ESP32 for Espruino,
 * a JavaScript interpreter for Microcontrollers designed by Gordon Williams
 *
 * Copyright (C) 2020 by Mark Becker & Richard Gomez
 * based on code from 2016 by Juergen Marsch 
 *
 * This Source Code Form is subject to the terms of the Mozilla Publici
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ----------------------------------------------------------------------------
 * This file is designed to be parsed during the build process
 *
 * Contains ESP32 board specific functions.
 * ----------------------------------------------------------------------------
 */

#include "jspininfo.h"

#define RMTChannelMax 8 		//maximum RMT channel

struct RMTChannel{Pin pin;}; //will be extended once we know more about RMT functions for Espruino on ESP32
struct RMTChannel RMTChannels[RMTChannelMax];

void RMTInit();
void RMTReset();

int getRMTIndex(Pin pin);
int getFreeRMT(Pin pin);

