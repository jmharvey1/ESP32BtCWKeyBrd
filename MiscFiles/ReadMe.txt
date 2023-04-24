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
4. Right Arrow		Alternate action; Allows text to be typed, and not sent. Press again, and normal sending resumes. (In hold mode, lower left box is yellow)
5. Left Arrow		Store text (When active, lower left box is White). Up to 20 characters (no spaces) can be stored to be sent later. (Space bar, auto exit F1 store mode, or press F1 again to stop store) 
6. Lshift+Enter” Send "Left Arrow" stored text plus Your call sign.
7. Cntrl+Enter” Send "Left Arrow" stored text.
8. "Esc"   Abort/dump outgoing text 	
10. Cntrl+S Go to settings Screen; Press Cntrl+S again to return to normal CW mode.(Note: No CW sent while in the "settings"mode); i.e., Default WPM. Call Sign, F2 memory
11. F1 Save up to ten characters (usually the DX call sign) to be sent when L. Shift (or Cntrl)+enter is pressed, Press F1 again to stop the "save"  mode (Note: status box in lower left is white while in "save" mode)
12. F2 Send stored F2 message
13. F3 Send stored F3 message
14. F4 Send stored F4 message
15. F12 suspend outgoing text; Press F12 again to resume sending outgoing text (Note: Status box is yellow while in "suspend" mode)



