/*
 *  JtagKnocker - JTAG finder and enumerator for STM32 dev boards
 *  Copyright (C) 2014 Nathan Dyer
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "jtag.h"
#include "jtagtap.h"

static jtagTAP_TAPState TAPState;	//<< Holds the current state of the TAP

const char * const jtagTAP_StateNames[JTAGTAP_STATE_MAX] = {
	[JTAGTAP_STATE_UNKNOWN] = "Unknown",
	[JTAGTAP_STATE_RESET] = "Reset",
	[JTAGTAP_STATE_IDLE] = "Run/Idle",
	[JTAGTAP_STATE_DR_SCAN] = "Scan DR",
	[JTAGTAP_STATE_DR_CAPTURE] = "Capture DR",
	[JTAGTAP_STATE_DR_SHIFT] = "Shift DR",
	[JTAGTAP_STATE_DR_EXIT1] = "Exit 1 DR",
	[JTAGTAP_STATE_DR_PAUSE] = "Pause DR",
	[JTAGTAP_STATE_DR_EXIT2] = "Exit 2 DR",
	[JTAGTAP_STATE_DR_UPDATE] = "Update DR",
	[JTAGTAP_STATE_IR_SCAN] = "Scan IR",
	[JTAGTAP_STATE_IR_CAPTURE] = "Capture IR",
	[JTAGTAP_STATE_IR_SHIFT] = "Shift IR",
	[JTAGTAP_STATE_IR_EXIT1] = "Exit 1 IR",
	[JTAGTAP_STATE_IR_PAUSE] = "Pause IR",
	[JTAGTAP_STATE_IR_EXIT2] = "Exit 2 IR",
	[JTAGTAP_STATE_IR_UPDATE] = "Update IR"
};

const unsigned int delay_count = 20000;

/**
 * @brief Initalize the TAP module
 *
 */
void jtagTAP_Init()
{
	TAPState = JTAGTAP_STATE_UNKNOWN;
}

/**
 * @brief Advance the TAP to the requested state
 *
 * @param[in] target The state to get the TAP into
 */
void jtagTAP_SetState(jtagTAP_TAPState target)
{
	if(target != JTAGTAP_STATE_UNKNOWN)
	{
		if((target == JTAGTAP_STATE_RESET) && (jtag_IsAllocated(JTAG_SIGNAL_TRST)))
		{
			unsigned int count = 0;

			//take the easy way to reset
			jtag_Set(JTAG_SIGNAL_TMS, true);
			jtag_Set(JTAG_SIGNAL_TRST, false);	//Assumes an active low signal

			//delay a bit
			while(++count < delay_count)
			{
				__asm("nop");
			}
			jtag_Set(JTAG_SIGNAL_TRST, true);	//Assumes an active low signal
			TAPState = JTAGTAP_STATE_RESET;
		}

		while(TAPState != target)
		{
			//only process if there is a state change requested.
			switch(TAPState)
			{
				//Handle the unknown case by performing a reset
				case JTAGTAP_STATE_UNKNOWN:
					if(jtag_IsAllocated(JTAG_SIGNAL_TRST))
					{
						unsigned int count = 0;

						//take the easy way to reset
						jtag_Set(JTAG_SIGNAL_TMS, true);
						jtag_Set(JTAG_SIGNAL_TRST, false);	//Assumes an active low signal

						//delay a bit
						while(++count < delay_count)
						{
							__asm("nop");
						}
						jtag_Set(JTAG_SIGNAL_TRST, true);	//Assumes an active low signal
					}
					else
					{
						jtag_Set(JTAG_SIGNAL_TMS, true);
						jtag_Clock();
						jtag_Clock();
						jtag_Clock();
						jtag_Clock();
						jtag_Clock();
					}
					TAPState = JTAGTAP_STATE_RESET;
					break;

				case JTAGTAP_STATE_RESET:
					jtag_Set(JTAG_SIGNAL_TMS, false);
					jtag_Clock();
					TAPState = JTAGTAP_STATE_IDLE;
					break;

				case JTAGTAP_STATE_IDLE:
					jtag_Set(JTAG_SIGNAL_TMS, true);
					jtag_Clock();
					TAPState = JTAGTAP_STATE_DR_SCAN;
					break;

				case JTAGTAP_STATE_DR_SCAN:
					if((target >= JTAGTAP_STATE_DR_CAPTURE) && (target <= JTAGTAP_STATE_DR_UPDATE))
					{
						jtag_Set(JTAG_SIGNAL_TMS, false);
						TAPState = JTAGTAP_STATE_DR_CAPTURE;
					}
					else
					{
						jtag_Set(JTAG_SIGNAL_TMS, true);
						TAPState = JTAGTAP_STATE_IR_SCAN;
					}
					jtag_Clock();
					break;

				case JTAGTAP_STATE_DR_CAPTURE:
					if(target == JTAGTAP_STATE_DR_SHIFT)
					{
						jtag_Set(JTAG_SIGNAL_TMS, false);
						TAPState = JTAGTAP_STATE_DR_SHIFT;
					}
					else
					{
						jtag_Set(JTAG_SIGNAL_TMS, true);
						TAPState = JTAGTAP_STATE_DR_EXIT1;
					}
					jtag_Clock();
					break;

				case JTAGTAP_STATE_DR_SHIFT:
					jtag_Set(JTAG_SIGNAL_TMS, true);
					jtag_Clock();
					TAPState = JTAGTAP_STATE_DR_EXIT1;
					break;

				case JTAGTAP_STATE_DR_EXIT1:
					if((target == JTAGTAP_STATE_DR_PAUSE) || (target == JTAGTAP_STATE_DR_EXIT2) || (target == JTAGTAP_STATE_DR_SHIFT))
					{
						jtag_Set(JTAG_SIGNAL_TMS, false);
						TAPState = JTAGTAP_STATE_DR_PAUSE;
					}
					else
					{
						jtag_Set(JTAG_SIGNAL_TMS, true);
						TAPState = JTAGTAP_STATE_DR_UPDATE;
					}
					jtag_Clock();
					break;

				case JTAGTAP_STATE_DR_PAUSE:
					jtag_Set(JTAG_SIGNAL_TMS, true);
					jtag_Clock();
					TAPState = JTAGTAP_STATE_DR_EXIT2;
					break;

				case JTAGTAP_STATE_DR_EXIT2:
					if((target == JTAGTAP_STATE_DR_PAUSE) || (target == JTAGTAP_STATE_DR_EXIT1) || (target == JTAGTAP_STATE_DR_SHIFT))
					{
						jtag_Set(JTAG_SIGNAL_TMS, false);
						TAPState = JTAGTAP_STATE_DR_SHIFT;
					}
					else
					{
						jtag_Set(JTAG_SIGNAL_TMS, true);
						TAPState = JTAGTAP_STATE_DR_UPDATE;
					}
					jtag_Clock();
					break;

				case JTAGTAP_STATE_DR_UPDATE:
					if(target == JTAGTAP_STATE_IDLE)
					{
						jtag_Set(JTAG_SIGNAL_TMS, false);
						TAPState = JTAGTAP_STATE_IDLE;
					}
					else
					{
						jtag_Set(JTAG_SIGNAL_TMS, true);
						TAPState = JTAGTAP_STATE_DR_SCAN;
					}
					jtag_Clock();
					break;

				case JTAGTAP_STATE_IR_SCAN:
					if((target >= JTAGTAP_STATE_IR_CAPTURE) && (target <= JTAGTAP_STATE_IR_UPDATE))
					{
						jtag_Set(JTAG_SIGNAL_TMS, false);
						TAPState = JTAGTAP_STATE_IR_CAPTURE;
					}
					else
					{
						jtag_Set(JTAG_SIGNAL_TMS, true);
						TAPState = JTAGTAP_STATE_RESET;
					}
					jtag_Clock();
					break;

				case JTAGTAP_STATE_IR_CAPTURE:
					if(target == JTAGTAP_STATE_IR_SHIFT)
					{
						jtag_Set(JTAG_SIGNAL_TMS, false);
						TAPState = JTAGTAP_STATE_IR_SHIFT;
					}
					else
					{
						jtag_Set(JTAG_SIGNAL_TMS, true);
						TAPState = JTAGTAP_STATE_IR_EXIT1;
					}
					jtag_Clock();
					break;

				case JTAGTAP_STATE_IR_SHIFT:
					jtag_Set(JTAG_SIGNAL_TMS, true);
					jtag_Clock();
					TAPState = JTAGTAP_STATE_IR_EXIT1;
					break;

				case JTAGTAP_STATE_IR_EXIT1:
					if((target == JTAGTAP_STATE_IR_PAUSE) || (target == JTAGTAP_STATE_IR_EXIT2) || (target == JTAGTAP_STATE_IR_SHIFT))
					{
						jtag_Set(JTAG_SIGNAL_TMS, false);
						TAPState = JTAGTAP_STATE_IR_PAUSE;
					}
					else
					{
						jtag_Set(JTAG_SIGNAL_TMS, true);
						TAPState = JTAGTAP_STATE_IR_UPDATE;
					}
					jtag_Clock();
					break;

				case JTAGTAP_STATE_IR_PAUSE:
					jtag_Set(JTAG_SIGNAL_TMS, true);
					jtag_Clock();
					TAPState = JTAGTAP_STATE_IR_EXIT2;
					break;

				case JTAGTAP_STATE_IR_EXIT2:
					if((target == JTAGTAP_STATE_IR_PAUSE) || (target == JTAGTAP_STATE_IR_EXIT1) || (target == JTAGTAP_STATE_IR_SHIFT))
					{
						jtag_Set(JTAG_SIGNAL_TMS, false);
						TAPState = JTAGTAP_STATE_IR_SHIFT;
					}
					else
					{
						jtag_Set(JTAG_SIGNAL_TMS, true);
						TAPState = JTAGTAP_STATE_IR_UPDATE;
					}
					jtag_Clock();
					break;

				case JTAGTAP_STATE_IR_UPDATE:
					if(target == JTAGTAP_STATE_IDLE)
					{
						jtag_Set(JTAG_SIGNAL_TMS, false);
						TAPState = JTAGTAP_STATE_IDLE;
					}
					else
					{
						jtag_Set(JTAG_SIGNAL_TMS, true);
						TAPState = JTAGTAP_STATE_DR_SCAN;
					}
					jtag_Clock();
					break;

			}

		}
	}
	else
	{
		TAPState = JTAGTAP_STATE_UNKNOWN;
	}
}

/**
 * @brief Get the current state of the TAP
 *
 */
jtagTAP_TAPState jtagTAP_GetState()
{
	return TAPState;
}
