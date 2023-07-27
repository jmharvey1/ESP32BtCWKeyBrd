ESP32 Bluetooh KeyBoard user notes

***********************************************************************************
KeyBoard Encoder Notes:
Special Keys & their functions:
Keys that send special Morse Characters
1. “=”   <BT>
2. “+”	<KN>
3. ”>”	<AR>
4. ”<”	<AS>
5. ”%”	<SK>
6. All other unassigned keys (i.e. “{“, “]”,…) send 6 dits, CW error code

Special Functions:
1. cntrl+T	Generates continuous key down state. Press “cntrl+T” again (or another key) to stop.
2. Enter [pressed by itself] sends the "My Call" memory entry. (see settings screen)
3. “delete” Back space, to delete unsent buffered code.
4. Right Arrow	Same as F12.
5. Left Arrow Same as F1.  
6. Lshift+Enter” Send "Left Arrow" stored text plus Your call sign.
7. Cntrl+Enter” Send "Left Arrow" stored text.
8. "Esc"   Abort/dump outgoing text 	
10. Cntrl+S Go to settings Screen; Press Cntrl+S again to return to normal CW mode.(Note: No CW sent while in the "settings"mode); i.e., Default WPM. Call Sign, F2 memory
11. F1 Save up to ten characters (usually the DX call sign) to be sent when L. Shift (or Cntrl)+enter is pressed, Press F1 again to stop the "save"  mode (Note: status box in lower left is white while in "save" mode)
12. F2 Send stored F2 message
13. F3 Send stored F3 message
14. F4 Send stored F4 message
15. F12 suspend outgoing text; Press F12 again to resume sending outgoing text (Note: Status box is yellow while in "suspend" mode)

***********************************************************************************
CW Decoder Notes:
Special Keys & their functions:
1. Left & Rght Ctrl+d: index through CW decode modes
        Nrm (Normal)
        Bg1 (Bug 1)
        Bg2 (Bug 2)
        Bg3 (Bug 2 cootie keys)
2. Left Ctrl+f: index through select tone tune modes
        FF  (Fixed Frequency)
        AF  (Auto-Tune 500 to 900Hz)
3. Left Ctrl-P:  Plot On/off        




