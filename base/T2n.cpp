/**************************************************************************
	Souliss
    Copyright (C) 2011  Veseo

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
	
	Originally developed by Dario Di Maio and Alessandro DelPex
	
***************************************************************************/
/*!
    \file 
    \ingroup

*/

/**************************************************************************/
/*!
	Define the use of Typical 21 : Motorized devices with limit switches
*/	
/**************************************************************************/
void Souliss_SetT21(U8 *memory_map, U8 slot)
{
	memory_map[MaCaco_TYP_s + slot] = Souliss_T21;
}

/**************************************************************************/
/*!
	Typical 21 : Motorized devices with limit switches
	
		It handle OPEN / CLOSE devices with alternate direction moving throught
		the STOP state. Is used for garage doors or generally devices that need 
		to be fully opened or closed.
		
		Once a command, it remain active until the relevant limit switch is
		recognized or the associated timer is expired. Timer must be used
		for proper release of command.
		Limit switches are not mandatory, but if not used, the device engine
		shall be protected from extra current or shall have its own control
		center.
		
		Hardware Limit Switches :
		
			Using bistable switches for identification of open and close position			
				#define Souliss_T2n_LimSwitch_Close	0x08
				#define Souliss_T2n_LimSwitch_Open	0x10

		Hardware Command:
			
			Using a monostable wall switch (press and spring return) or a 
			software command from user interface, each press will toggle 
			the output status.		
				#define Souliss_T2n_ToggleCmd		0x04
				
		Software Command:
				#define Souliss_T2n_CloseCmd		0x01
				#define Souliss_T2n_OpenCmd			0x02
				#define Souliss_T2n_StopCmd			0x03
				
		Output status:
		- 1(hex) for CLOSE,
		- 2(hex) for OPEN,
		- 3(hex) for STOP.
	
*/	
/**************************************************************************/
U8 Souliss_Logic_T21(U8 *memory_map, U8 slot, U8 *trigger, U8 timeout=Souliss_T2n_Timer_Val)
{
	U8 i_trigger=0;														// Internal trigger
	if(timeout<=Souliss_T2n_Timer_Off)	timeout=Souliss_T2n_Timer_Val;
	
	// Look for input value, update output. If the output is not set, trig a data
	// change, otherwise just reset the input
	
	if((memory_map[MaCaco_IN_s + slot] == Souliss_T2n_ToggleCmd) || (memory_map[MaCaco_IN_s + slot] == Souliss_T2n_StopCmd) || (memory_map[MaCaco_IN_s + slot] == Souliss_T2n_OpenCmd_Local) || (memory_map[MaCaco_IN_s + slot] == Souliss_T2n_CloseCmd_Local))
	{
		if(memory_map[MaCaco_IN_s + slot] == Souliss_T2n_ToggleCmd)
		{		
			// Change the output value, between OPEN and CLOSE always OFF is performed	
			if((memory_map[MaCaco_OUT_s + slot] == Souliss_T2n_Coil_Close) || (memory_map[MaCaco_OUT_s + slot] == Souliss_T2n_Coil_Open))
			{
				memory_map[MaCaco_AUXIN_s + slot] = memory_map[MaCaco_OUT_s + slot];	// Save actual state	
				memory_map[MaCaco_OUT_s + slot] = Souliss_T2n_Coil_Stop;				// Off Command
			}
			else if((memory_map[MaCaco_AUXIN_s + slot] == Souliss_T2n_Coil_Close) || (memory_map[MaCaco_OUT_s + slot] == Souliss_T2n_State_Close))
				memory_map[MaCaco_OUT_s + slot] = Souliss_T2n_Coil_Open;			// Open Command
			else if((memory_map[MaCaco_AUXIN_s + slot] == Souliss_T2n_Coil_Open) || (memory_map[MaCaco_OUT_s + slot] == Souliss_T2n_State_Open))
				memory_map[MaCaco_OUT_s + slot] = Souliss_T2n_Coil_Close;			// Close command
			else
				memory_map[MaCaco_OUT_s + slot] = Souliss_T2n_Coil_Open;			// If state is undefined, Open Command
		}
		else if((memory_map[MaCaco_IN_s + slot] == Souliss_T2n_OpenCmd_Local) && ((memory_map[MaCaco_OUT_s + slot] == Souliss_T2n_Coil_Stop) || (memory_map[MaCaco_OUT_s + slot] == Souliss_T2n_State_Close)))
				memory_map[MaCaco_OUT_s + slot] = Souliss_T2n_Coil_Open;			// Open Command
		else if((memory_map[MaCaco_IN_s + slot] == Souliss_T2n_CloseCmd_Local) && ((memory_map[MaCaco_OUT_s + slot] == Souliss_T2n_Coil_Stop) || (memory_map[MaCaco_OUT_s + slot] == Souliss_T2n_State_Open)))
				memory_map[MaCaco_OUT_s + slot] = Souliss_T2n_Coil_Close;			// Close command
		else 
				memory_map[MaCaco_OUT_s + slot] = Souliss_T2n_Coil_Stop;			// Stop Command
		
		// If a command was issued, set the timer
		if((memory_map[MaCaco_OUT_s + slot] == Souliss_T2n_Coil_Open) || (memory_map[MaCaco_OUT_s + slot] == Souliss_T2n_Coil_Close))
		{
			memory_map[MaCaco_AUXIN_s + slot] = timeout;						// Set timer value
			memory_map[MaCaco_IN_s + slot] = Souliss_T2n_RstCmd;					// Reset
	  		i_trigger = Souliss_TRIGGED;
		}
		
	}
	else if((memory_map[MaCaco_IN_s + slot] == Souliss_T2n_LimSwitch_Close) || ((memory_map[MaCaco_OUT_s + slot] == Souliss_T2n_Coil_Close) && (memory_map[MaCaco_AUXIN_s + slot] == Souliss_T2n_Timer_Off)))
	{
		memory_map[MaCaco_OUT_s + slot] = Souliss_T2n_State_Close;			// Close Limit Switch
		memory_map[MaCaco_IN_s + slot] = Souliss_T2n_RstCmd;					// Reset
  		i_trigger = Souliss_TRIGGED;
	}
	else if((memory_map[MaCaco_IN_s + slot] == Souliss_T2n_LimSwitch_Open) || ((memory_map[MaCaco_OUT_s + slot] == Souliss_T2n_Coil_Open) && (memory_map[MaCaco_AUXIN_s + slot] == Souliss_T2n_Timer_Off)))
	{
		memory_map[MaCaco_OUT_s + slot] = Souliss_T2n_State_Open;			// Open Limit Switch
		memory_map[MaCaco_IN_s + slot] = Souliss_T2n_RstCmd;					// Reset
  		i_trigger = Souliss_TRIGGED;	
	}

	// Update the trigger
	if(i_trigger)
		*trigger = i_trigger;
	
	return i_trigger;	
	
}

/**************************************************************************/
/*!
	Timer associated to T21, timeout the OPEN/CLOSE commands
*/	
/**************************************************************************/
void Souliss_T21_Timer(U8 *memory_map, U8 slot)
{
	if(memory_map[MaCaco_AUXIN_s + slot] > Souliss_T2n_Timer_Off)				// Memory value is used as timer
		memory_map[MaCaco_AUXIN_s + slot]--;				// Decrease timer
}

/**************************************************************************/
/*!
	Define the use of Typical 22 : Motorized devices with limit switches
*/	
/**************************************************************************/
void Souliss_SetT22(U8 *memory_map, U8 slot)
{
	memory_map[MaCaco_TYP_s + slot] = Souliss_T22;
	memory_map[MaCaco_OUT_s + slot] = Souliss_T2n_Coil_Stop;
}

/**************************************************************************/
/*!
	Typical 22 : Motorized devices with limit switches and middle position
	
		It handle OPEN / CLOSE devices with alternate direction, dedicated
		commands for OPEN, CLOSE and STOP are available. Every state change
		goes through the STOP command for security reason.
		Is used for curtains or other devices that need an adjustable position.
		
		Once a command, it remain active until the relevant limit switch is
		recognized or the associated timer is expired. Timer must be used
		for proper release of command.
		Limit switches are not mandatory, but if not used, the device engine
		shall be protected from extra current or shall have its own control
		center.
		
		Hardware Limit Switches :
		
			Using bistable switches for identification of open and close position			
				#define Souliss_T2n_LimSwitch_Close	0x14
				#define Souliss_T2n_LimSwitch_Open	0x16

		Hardware and/or Software Command:
			
			Using a monostable wall switch (press and spring return) or a 
			software command from user interface, each press will toggle 
			the output status.			
				#define Souliss_T2n_CloseCmd_Local	0x08
				#define Souliss_T2n_OpenCmd_Local	0x10
				#define Souliss_T2n_StopCmd			0x04
				
			Following constant are defined for sketch source compatibility with versions < A6.1.1
			and their use is now deprecated.
				#define Souliss_T2n_CloseCmd		0x01
				#define Souliss_T2n_OpenCmd			0x02

			This commands are designed to be used by an application (Souliss App, OpenHAB binding, user applications)
			in order to support software "scenario". When the Open/Close command is received it is always excuted;
			if the motor is running opposite direction it stops for 4 cycles then it revert motion.
				#define Souliss_T2n_CloseCmd_SW			0x01
				#define Souliss_T2n_OpenCmd_SW			0x02
				#define Souliss_T2n_StopCmd					0x03
				
		Command recap, using: 
		-  0x01(hex) as command, Software CLOSE request (stop 4 cycles if opening) 
		-  0x02(hex) as command, Software OPEN request (stop 4 cycles if closing)
		-  0x04(hex) as command, STOP request
		-  0x08(hex) as command, CLOSE request (stop if opening) 
		-  0x10(hex for OPEN request (stop if closing)
		
		Output status:
		- 1(hex) for CLOSING,
		- 2(hex) for OPENING,
		- 3(hex) for STOP.
		- 8(hex) for CLOSE
		- 10(hex for OPEN
	
*/	
/**************************************************************************/
U8 Souliss_Logic_T22(U8 *memory_map, U8 slot, U8 *trigger, U8 porcentaje_destino)
{
    U8 i_trigger = 0;
    // Compatibilidad con comandos estándar Souliss (app Android)
    if (memory_map[MaCaco_IN_s + slot] == Souliss_T2n_OpenCmd_SW ||
        memory_map[MaCaco_IN_s + slot] == Souliss_T2n_OpenCmd_Local) {
        porcentaje_destino = 100;
    } else if (memory_map[MaCaco_IN_s + slot] == Souliss_T2n_CloseCmd_SW ||
               memory_map[MaCaco_IN_s + slot] == Souliss_T2n_CloseCmd_Local) {
        porcentaje_destino = 0;
    }
    S16 pos_actual = persiana_pos[slot];
    S16 pos_destino = porcentaje_destino;
    // Si la posición es desconocida, solo permite movimientos completos
    if (pos_actual == -1 && (pos_destino != 0 && pos_destino != 100)) {
        memory_map[MaCaco_OUT_s + slot] = Souliss_T2n_Coil_Stop;
        return 0;
    }
    U32 tiempo_mov = (U32)(abs(diferencia) * tiempo_total_persiana[slot]) / 100;
    if (tiempo_mov == 0) tiempo_mov = 100; // mínimo 100 ms
    // Inicia movimiento
    if (diferencia > 0) {
        memory_map[MaCaco_OUT_s + slot] = Souliss_T2n_Coil_Open;
    } else {
        memory_map[MaCaco_OUT_s + slot] = Souliss_T2n_Coil_Close;
    }
    persiana_mov_start[slot] = millis();
    persiana_mov_destino[slot] = pos_destino;
    memory_map[MaCaco_IN_s + slot] = Souliss_T2n_RstCmd;
    i_trigger = 1; // Souliss_TRIGGED suele ser 1
    if (i_trigger)
        *trigger = i_trigger;
    return i_trigger;
}

void Souliss_T22_Timer(U8 *memory_map, U8 slot)
{
    if (persiana_mov_start[slot] == 0 || persiana_mov_destino[slot] == -1)
        return; // No hay movimiento en curso
    S16 pos_actual = persiana_pos[slot];
    S16 pos_destino = persiana_mov_destino[slot];
    S16 diferencia = pos_destino - pos_actual;
    U32 tiempo_mov = (U32)(abs(diferencia) * tiempo_total_persiana[slot]) / 100;
    U32 tiempo_transcurrido = millis() - persiana_mov_start[slot];
    if (tiempo_transcurrido >= tiempo_mov) {
        // Movimiento terminado
        persiana_pos[slot] = pos_destino;
        memory_map[MaCaco_OUT_s + slot] = Souliss_T2n_Coil_Stop;
        persiana_mov_start[slot] = 0;
        persiana_mov_destino[slot] = -1;
    }
    // Si quieres actualizar la posición estimada en tiempo real (opcional):
    else {
        S16 avance = (S16)((tiempo_transcurrido * abs(diferencia)) / tiempo_mov);
        if (diferencia > 0)
            persiana_pos[slot] = pos_actual + avance;
        else
            persiana_pos[slot] = pos_actual - avance;
    }
}
  return 0;
    }
    if (pos_actual == -1) pos_actual = (pos_destino == 0) ? 100 : 0; // Asume lo opuesto para calcular tiempo
    S16 diferencia = pos_destino - pos_actual;
    if (diferencia == 0) {
        memory_map[MaCaco_OUT_s + slot] = Souliss_T2n_Coil_Stop;
        return 0;
    }
    U32 tiempo_mov = (U32)(abs(diferencia) * tiempo_total_persiana[slot]) / 100;
    if (tiempo_mov == 0) tiempo_mov = 100; // mínimo 100 ms
    // Inicia movimiento
    if (diferencia > 0) {
        memory_map[MaCaco_OUT_s + slot] = Souliss_T2n_Coil_Open;
    } else {
        memory_map[MaCaco_OUT_s + slot] = Souliss_T2n_Coil_Close;
    }
    persiana_mov_start[slot] = millis();
    persiana_mov_destino[slot] = pos_destino;
    memory_map[MaCaco_IN_s + slot] = Souliss_T2n_RstCmd;
    i_trigger = 1; // Souliss_TRIGGED suele ser 1
    if (i_trigger)
        *trigger = i_trigger;
    return i_trigger;
}

void Souliss_T22_Timer(U8 *memory_map, U8 slot)
{
    if (persiana_mov_start[slot] == 0 || persiana_mov_destino[slot] == -1)
        return; // No hay movimiento en curso
    S16 pos_actual = persiana_pos[slot];
    S16 pos_destino = persiana_mov_destino[slot];
    S16 diferencia = pos_destino - pos_actual;
    U32 tiempo_mov = (U32)(abs(diferencia) * tiempo_total_persiana[slot]) / 100;
    U32 tiempo_transcurrido = millis() - persiana_mov_start[slot];
    if (tiempo_transcurrido >= tiempo_mov) {
        // Movimiento terminado
        persiana_pos[slot] = pos_destino;
        memory_map[MaCaco_OUT_s + slot] = Souliss_T2n_Coil_Stop;
        persiana_mov_start[slot] = 0;
        persiana_mov_destino[slot] = -1;
    }
    // Si quieres actualizar la posición estimada en tiempo real (opcional):
    else {
        S16 avance = (S16)((tiempo_transcurrido * abs(diferencia)) / tiempo_mov);
        if (diferencia > 0)
            persiana_pos[slot] = pos_actual + avance;
        else
            persiana_pos[slot] = pos_actual - avance;
    }
}
