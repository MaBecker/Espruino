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
 
#include "jsutils.h"
#include "jshardwareRMT.h"

#include "driver/rmt.h"

#include <stdio.h>

#define RMTPinEmpty 111

// called on 'jshReset()' in jshardware.c
// free all RMT channels and unistall the driver for each channel
void RMTReset(){
  for(int i = 0; i < RMTChannelMax; i++){
    if(RMTChannels[i].pin != RMTPinEmpty) rmt_driver_uninstall(i);
  }
}

// called on 'espruinoTask(...)' in main.c
void RMTInit(){
  int i;
  for(i = 0; i < RMTChannelMax; i++) RMTChannels[i].pin = RMTPinEmpty;
}

int getRMTIndex(Pin pin){
  int i;
  for(i = 0; i < RMTChannelMax; i++){
	if(RMTChannels[i].pin == pin) return i;	
  }
  return -1;	
}

int getFreeRMT(Pin pin){
  for(int i = 0; i < RMTChannelMax; i++){
	if(RMTChannels[i].pin == RMTPinEmpty) {
	  RMTChannels[i].pin = pin;	
	  return i;
	}
  }
  return -1;
}

