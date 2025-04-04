/****************************************************************************
 * Copyright (C) 2012 FIX94
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
 
#ifndef _USBTHREAD_H_
#define _USBTHREAD_H_

#ifdef __cplusplus
extern "C"
{
#endif

// #define USBKEEPALIVE // uncomment to enable "USB keep alive" functions

void CreateUSBKeepAliveThread();
void KillUSBKeepAliveThread();
void USBKeepAliveThreadReset();
static volatile bool reading = false;

#ifdef __cplusplus
}
#endif

#endif /* _USBTHREAD_H_ */
