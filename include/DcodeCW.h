/*
 * DcodeCW.h
 *
 *  Created on: Mar 28, 2021
 *      Author: jim
 */

#ifndef INC_DCODECW_H_
#define INC_DCODECW_H_
//Single character decode values/table(s)
// The following two sets of tables are paired.
// The 1st table contains the 'decode' value , & the 2nd table contains the letter /character at the same index row
// so the process is use the first table to find a row match for the decode value, then use the row number
// in the 2nd table to translate it to the actual character(s) needed to be displayed.

#define ARSIZE 44
static unsigned int CodeVal1[ARSIZE]={
  5,
  24,
  26,
  12,
  2,
  18,
  14,
  16,
  4,
  23,
  13,
  20,
  7,
  6,
  15,
  22,
  29,
  10,
  8,
  3,
  9,
  17,
  11,
  25,
  27,
  28,
  63,
  47,
  39,
  35,
  33,
  32,
  48,
  56,
  60,
  62,
  49,
  50,
  76,
  255,
  115,
  94,
  85,
  90
};

const char DicTbl1[ARSIZE][2]=
{
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "-",
    "/",
    "?",
    " ",
    ",",
    "'",
    ".",
	"@"
};
//Multi character decode values/table(s)
#define ARSIZE2 125
static unsigned int CodeVal2[ARSIZE2]={
  19,
  21,
  28,
  /*31,*/
  34,
  38,
  40,
  41,
  42,
  44,
  45,
  46,
  52,
  54,
  55,
  57,
  58,
  59,
  61,
  64,
  69,
  70,
  72,
  74,
  78,
  80,
  81,
  82,
  84,
  86,
  88,
  89,
  90,
  91,
  96,
  105,
  106,
  110,
  113,
  114,
  116,
  118,
  120,
  121,
  122,
  /*123,*/
  124,
  125,
  126,
  127,
  138,
  145,
  146,
  148,
  150,
  156,
  162,
  176,
  178,
  180,
  185,
  191,
  202,
  209,
  211,
  212,
  213,
  216,
  218,
  232,
  234,
  240,
  241,
  242,
  243,
  244,
  246,
  248,
  249,
  251,
  283,
  296,
  324,
  328,
  360,
  362,
  364,
  416,
  429,
  442,
  443,
  468,
  482,
  486,
  492,
  493,
  494,
  500,
  502,
  510,
  596,
  708,
  716,
  731,
  790,
  832,
  842,
  862,
  899,
  922,
  938,
  968,
  970,
  974,
  1348,
  1451,
  1480,
  1785,
  1795,
  1940,
  1942,
  6134,
  6580,
  14752,
  99990
};

const char DicTbl2[ARSIZE2][6]={
  "UT",
  "<AA>",
  "GE",
  /*"TO",*/
  "VE",
  "UN",
  "<AS>",
  "AU",
  "<AR>",
  "AD",
  "WA",
  "AG",
  "ND",
  "<KN>",
  "NO",
  "MU",
  "GN",
  "TY",
  "OA",
  "HI",
  "<SK>",
  "SG",
  "US",
  "UR",
  "UG",
  "RS",
  "AV",
  "AF",
  "AL",
  "86",
  "AB",
  "WU",
  "AC",
  "AY",
  "THE",
  "CA",
  "TAR",
  "TAG",
  "GU",
  "GR",
  "MAI",
  "MP",
  "OS",
  "OU",
  "OR",
  /*"my/ow",*/
  "OD",
  "OK",
  "OME",
  "TOO",
  "VR",
  "FU",
  "UF",
  "UL",
  "UP",
  "UZ",
  "AVE",
  "WH",
  "PR",
  "AND",
  "WX",
  "JO",
  "TUR",
  "CU",
  "CW",
  "CD",
  "CK",
  "YS",
  "YR",
  "QS",
  "QR",
  "OH",
  "OV",
  "OF",
  "OUT",
  "OL",
  "OP",
  "OB",
  "OX",
  "OY",
  "VY",
  "FB",
  "LL",
  "RRI",
  "WAS",
  "WAR",
  "AYI",
  "CH",
  "CQ",
  "NOR",
  "NOW",
  "MAL",
  "OVE",
  "OUN",
  "OAD",
  "OAK",
  "OWN",
  "OKI",
  "OYE",
  "OON",
  "UAL",
  "WIL",
  "W?",
  "WAY",
  "NING",
  "CHE",
  "CAR",
  "CON",
  "<73>",
  "MUC",
  "QRN",
  "OUS",
  "OUR",
  "OUG",
  "ALL",
  "WARM",
  "JUS",
  "YOU",
  "73",
  "OUL",
  "OUP",
  "JOYE",
  "XYL",
  "MUCH",
  "[er]"
};



//struct DF_t DFault;
////////////////////////////////////////////////////////////////////
// Special handler functions to store and retieve non-volital user data
//template <class T> int EEPROM_write(uint ee, const T& value)
//{
//   const byte* p = (const byte*)(const void*)&value;
//   int i;
//   for (i = 0; i < sizeof(value); i++)
//       EEPROM.write(ee++, *p++);
//   return i;
//}

//template <class T> int EEPROM_read(int ee, T& value)
//{
//   byte* p = (byte*)(void*)&value;
//   int i;
//   for (i = 0; i < sizeof(value); i++)
//       *p++ = EEPROM.read(ee++);
//   return i;
//}
////////////////////////////////////////////////////////////////////////////
void StartDecoder(void);
void KeyEvntSR(void);
void Dcodeloop(void);
void WPMdefault(void);
void ChkDeadSpace(void);
void SetLtrBrk(void);
void chkChrCmplt(void);
int CalcAvgPrd(unsigned long thisdur);
int CalcWPM(int dotT, int dahT, int spaceT);
void CalcAvgDah(unsigned long thisPeriod);
void SetModFlgs(int ModeVal);
void DisplayChar(unsigned int decodeval);
int Srch4Match(unsigned int decodeval, bool DpScan);
int linearSearchBreak(long val, unsigned int arr[], int sz);
int linearSrchChr(char val, char arr[ARSIZE][2], int sz);
void dispMsg(char Msgbuf[50]);
void scrollpg(void);
void DrawButton(void);
void DrawButton2(void);
void ModeBtn(void);
void showSpeed(void);
void SftReset(void);

#endif /* INC_DCODECW_H_ */
