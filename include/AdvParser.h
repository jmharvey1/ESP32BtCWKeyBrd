/*
 * AdvParser.h
 *
 *  Created on: Jan 7, 2024
 *      Author: jim (KW4KD)
 */
/* 
 * 20240114 numerous adds to methods properties and constants to extend/enhance this class )
 * 20240117 added Dcode4Dahs() to class; 
 * 20240205 added WrdBrkVal
 * 20240206 added StrchdDah property
 * 20240313 added SrchRplc_stuct & SrchRplcDict[]
 * 20240323 expanded SrchRplcDict[] to 212 entries
 * 20240420 expanded SrchRplcDict[] to 472 entries; added auto word break timing 'wrdbrkFtcr' 
 * 20240502 added entries 527 - 586 to  SrchRplcDict[] 
 * 20240504 expanded SrchRplcDict[] to 634 entries
 * 20240518 expanded SrchRplcDict[] to 682 entries
 * 20240608 expanded SrchRplcDict[] to 731 entries 
 * */
#ifndef INC_ADVPARSER_H_
#define INC_ADVPARSER_H_
#include "stdint.h"// need this to use char type
#include <stdio.h>
#define IntrvlBufSize 200
#define MsgbufSize 50
#define SrchDictSize 760
struct Buckt_t
{
	uint16_t Intrvl;
	uint8_t Cnt;
};

struct SrchRplc_stuct
{
  const char srchTerm[13];
  const char NuTerm[10];
  int ChrCnt;
  int Rule;
};

extern bool DbgWrdBrkFtcr;
class AdvParser
{
private:
    /* Properties */
    const SrchRplc_stuct SrchRplcDict[SrchDictSize] ={
        {"PD", "AND", 2, 1}, //0
        {"PY", "ANY", 2, 1}, //1 if(this->StrLength + 1 == this->SrchRplcDict[STptr].ChrCnt){ /*search term & msgbuf size are the same*/
        {"PT", "ANT", 2, 49}, //2 
        {"CP", "CAN", 2, 3}, //3 if (this->StrLength == 1)
        {"QY", "MAY", 2, 2}, //4 if (NdxPtr == 0 || (NdxPtr > 0 && this->Msgbuf[NdxPtr - 1] != 'C'))
        {"S2", "SUM", 2, 10}, //5
        {"WHW", "WHAT", 3, 0}, //6
        {"UAN", "UP", 3, 16}, //7
        {"WJS", "WATTS", 3, 0}, //8
        {"KNS", "YES", 3, 0}, //9
        {"PEK", "WEEK", 3, 0}, //10
        {"NAG", "NAME", 3, 1}, //11 /*NdxPtr == 0*/
        {"SAG", "SAME", 3, 5}, //12
        {"TIG", "TIME", 3, 37}, //13
        {"QLK", "TALK", 3, 0}, //14
        {"TB3", "73", 3, 0}, //15
        {"CTMPY", "COPY", 3, 0}, //16
        {"MPY", "MANY", 3, 0}, //17
        {"SI6", "SIDE", 3, 0}, //18
        {"QDE", "MADE", 3, 2}, //19 if (NdxPtr == 0 || (NdxPtr > 0 && this->Msgbuf[NdxPtr - 1] != 'C'))
        {"LWE", "LATE", 3, 19}, //20
        {"THW", "THAT", 3, 22}, //21
        {"THP", "THAN", 3, 0}, //22
        {"TMN", "ON", 3, 0}, //23
        {"PLL", "WELL", 3, 0}, //24
        {"SJE", "SAME", 3, 75}, //25
        {"CPT", "CANT", 3, 0}, //26
        {"0VE", "MOVE", 3, 0}, //27
        {"RLN", "RAIN", 3, 0}, //28
        {"D9T", "DONT", 3, 0}, //28
        {"TNN", "GN", 3, 20}, //30
        {"TNO", "GO", 3, 7}, //31
        {"SOG", "SOME", 3, 18}, //32 /*search term & msgbuf size are the same*/
        {"D9T", "DONT", 3, 0}, //33
        {"CHW", "CHAT", 3, 69}, //34
        {"WPT", "WANT", 3, 25}, //35
        {"W5N", "WHEN", 3, 38}, //36
        {"PNT", "WENT", 3, 0}, //37
        {"6IS", "THIS", 3, 0}, //38
        {"PEK", "WEEK", 3, 0}, //39"
        {"THJ", "THAT", 3, 0}, //40
        {"9LY", "ONLY", 3, 0}, //41
        {"WXST", "JUST", 4, 0}, //42
        {"TNET", "GET", 4, 0}, //43
        {"EAEA", "REA", 4, 0}, //44
        {"DAKT", "DAY", 4, 0}, //45
        {"TDNG", "TTING", 4, 0}, //46
        {"CETN", "CAN", 4, 0}, //47
        {"QSMT", "QSO", 4, 0}, //48
        {"INTN", "ING", 4, 0}, //49
        {"SETW", "SET AT", 4, 0}, //50
        {"MMMK", "OOK", 4, 0}, //51
        {"GMTT", "GOT", 4, 0}, //52
        {"TTTN", "ON", 4, 0}, //53
        {"WEUT", "PUT", 4, 0}, //54
        {"TBVT", "73", 4, 0}, //55
        {"INME", "ING", 4, 0}, //56", "PUM", 4, 0}, //91      
        {"EZNG", "ETTING", 4, 0}, //57
        {"DTYL", "XYL", 4, 0}, //58
        {"GAEE", "GRE", 4, 0}, //59
        {"NKEE", "NCE", 4, 70}, //60
        {"ARKT", "ARY", 4, 32}, //61
        {"NOAT", "NOW", 4, 0}, //62
        {"TELAI", "TELL", 5, 0}, //63
        {"TTTAN", "OP", 5, 0}, //64
        {"TTEAE", "GRE", 5, 0}, //65
        {"KTES", "YES", 4, 0}, //66
        {"C9DX", "CONDX", 4, 0}, //67
        {"MKT", "MY", 3, 1}, //68 if(this->StrLength + 1 == this->SrchRplcDict[STptr].ChrCnt){ /*search term & msgbuf size are the same*/
        {"EXCA", "EXTRA", 4, 0}, //69
        {"AP", "AGE", 2, 21}, //70 if(this->StrLength + 1 == this->SrchRplcDict[STptr].ChrCnt){ /*search term & msgbuf size are the same*/
        {"NDNG", "KING", 4, 0}, //71
        {"DNG", "TING", 3, 0}, //72
        {"LEP", "LEAN", 3, 25}, //73
        {"MEOT", "GOT", 4, 0}, //74
        {"6E", "THE", 2, 0}, //75
        {"6A", "THA", 2, 0}, //765
        {"VFG", "VING", 3, 0}, //77
        {"HEWH", "HEATH", 4, 0}, //78
        {"GEMIN", "GETTIN", 5, 0}, //79
        {"QKING", "MAKING", 5, 0}, //80
        {"M<AR>MI", "QRZ", 7, 0}, //81 /*search term & msgbuf size are the same*/
        {"M<AR>", "QR", 5, 15}, //82
        {"NTX", "WX", 3, 1}, //83 /*search term & msgbuf size are the same*/
        {"SMAU", "SQU", 4, 0}, //84
        {"PST", "WEST", 3, 1}, //85 /*search term & msgbuf size are the same*/
        {"S0E", "SOME", 3, 0}, //861
        {"TWAE", "TAKE", 4, 0}, //87
        {"LFUX", "LINUX", 4, 0}, //88
        {"ANE2", "ABO", 4, 0}, //89
        {"WERO", "PRO", 4, 0}, //90
        {"WEUM", "PUM", 4, 0}, //91
        {"STOAN", "STOP", 5, 0}, //92
        {"T0", "TOM", 2, 1}, //93 /*search term & msgbuf size are the same*/
        {"9E", "ONE", 2, 1}, //94 /*search term & msgbuf size are the same*/
        {"TTAK", "MAK", 4, 0}, //95
        {"PRETTKT", "PRETTY", 7, 0}, //96
        {"TTKT", "MY", 4, 0}, //97
        {"AQG", "AMAG", 3, 0}, //98
        {"GRWS", "GRATS", 4, 0}, //99
        {"POTH", "ANOTH", 4, 6}, //100
        {"TKSO", "QSO", 4, 0}, //101
        {"W9D", "WOND", 4, 0}, //102
        {"MTNLY", "ONLY", 5, 0}, //103 this needs to precede 104, "TNL"
        {"TNL", "GL", 3, 0}, //104
        {"NTTTT", "NOT", 5, 0}, //105
        {"NTT", "Y", 3, 23}, //106
        {"AIO", "LO", 3, 0}, //107
        {"HPG", "HANG", 3, 0}, //108
        {"TKS", "QS", 3, 8}, //109
        {"FTS", "FB", 3, 51}, //110
        {"CP", "CAN", 2, 4}, //111
        {"BTET", "BK", 4, 0}, //112
        {"CEP", "KEEP", 3, 11}, //113
        {"PDT", "ANDT", 3, 0}, //114
        {"C9C", "CONC", 3, 0}, //115 //i,e, concert
        {"DOAG", "DOWN", 4, 0}, //116
        {"TOUH", "TO USE", 4, 0}, //117
        {"ANAD", "TO PAD", 4, 9}, //118
        {"NNKT", "NNY", 4, 0}, //119
        {"ATPE", "ANGE", 4, 0}, //120
        {"TEKE", "NC", 4, 0}, //121
        {"ZKE", "MIKE", 3, 0}, //122
        {"DFG", "DING", 3, 0}, //123
        {"NTTE", "NG", 4, 0}, //124
        {"AAES", "ARS", 4, 0}, //125
        {"AAI", "AL", 3, 0}, //126
        {"TDKE", "MIKE", 4, 0}, //127
        {"KTE", "YE", 3, 40}, //128
        {"RTMB", "ROB", 4, 0}, //129
        {"DAEI", "DRI", 4, 0}, //130
        {"HADMI", "HW?", 5, 0}, //131
        {"7MS", "73", 3, 0}, //132
        {"TSUT", "BUT", 4, 0}, //133
        {"IBS", "ITS", 3, 64}, //134
        {"ADLL", "WILL", 4, 0}, //135
        {"TTTAID", "OLD", 6, 0}, //136
        {"SKRT", "START", 4, 0}, //137
        {"COG", "COME", 3, 1}, //138
        {"Q6", "QTH", 2, 0}, //139
        {"ORI9", "ORION", 4, 0}, //140
        {"HPD", "HAND", 3, 0}, //141
        {"G?", "GUD", 2, 14}, //142
        {"L9G", "LONG", 3, 0}, //143
        {"<KN>AR", "YEAR", 6, 0}, //144
        {"BPD", "BAND", 3, 0}, //145
        {"SQLL", "SMALL", 4, 0}, //146
        {"AKS", "WAS", 3, 1}, //147
        {"ERLEA", "PLEA", 5, 0}, //148
        {"AGMT", "AGO", 4, 0}, //149
        {"6ST", "BEST", 3, 0}, //150
        {"HETVE", "HAVE", 5, 0}, //151
        {"I9", "ION", 5, 29}, //152
        {"NTEQ", "CQ", 4, 0}, //153
        {"NNQ", "CQ", 3, 0}, //154
        {"TYH", "QTH", 3, 14}, //155
        {"LTURN", "RETURN", 5, 0}, //156
        {"WAMS", "WATTS", 4, 0}, //157
        {"NITRE", "NICE", 5, 0}, //158
        {"CHROG", "CHROME", 5, 0}, //159
        {"LOONT", "LOOK", 5, 0}, //160
        {"ANLUS", "PLUS", 5, 0}, //161
        {"ON0", "90", 3, 0}, //162
        {"EQNT", "WANT", 4, 0}, //163
        {"NFG", "NING", 3, 0}, //164
        {"DMGT", "DONT", 4, 0}, //165
        {"GITD", "GUD", 4, 0}, //166
        {"SUAEE", "SURE", 5, 0}, //167
        {"TBSM", "73", 4, 0}, //168
        {"TTT", "O", 3, 1}, //169
        {"BETND", "BAND", 5, 0}, //170
        {"NOMG", "NOON", 4, 0}, //171
        {"OI0W", "80W", 4, 0}, //172
        {"ATORK", "WORK", 5, 0}, //173
        {"ATRK", "WRK", 4, 1}, //174
        {"DAUME", "DAUG", 5, 0}, //175
        {"DAUME", "DAUG", 5, 0}, //176
        {"VERKT", "VERY", 5, 0}, //177
        {"LAAG", "LAWN", 4, 0}, //178
        {"WAED", "WAL", 4, 0}, //179
        {"OEDD", "OLD", 4, 0}, //180
        /*FINISHED;  SrchTerm: INISH; New: FFISHED; oldStrLength 8; STptr: 181*/
        {"INISH", "FISH", 5, 12}, //181
        {"<KN>S", "YES", 5, 0}, //182
        {"STY", "VY", 3, 17}, //183 rule 17, exact match rule
        {"FMTR", "FOR", 4, 0}, //184
        {"ATITH", "WITH", 5, 0}, //185
        {"TTTT", "TO", 4, 1}, //186
        {"MARU", "QRU", 4, 0}, //187
        {"DEMN", "DWN", 4, 0}, //188
        {"LMN", "LY", 3, 0}, //189
        {"TATKE", "TAKE", 5, 0}, //190
        {"THETNKS", "THANKS", 7, 0}, //191
        {"MTVE", "OVE", 4, 0}, //192
        {"QN", "MAN", 2, 13}, //193
        {"H0E", "HOME", 3, 0}, //194
        {"BCE", "NICE", 3, 0}, //195
        {"DETN", "DEG", 4, 27}, //196
        {"LOOAN", "LOOP", 5, 0}, //197
        {"TTORN", "MORN", 5, 0}, //198
        {"TNUD", "GUD", 4, 0}, //199
        {"YEAN", "YEP", 4, 0}, //200
        {"MD0", "80", 3, 0}, //201
        {"SCONG", "STRONG", 5, 73}, //202
        {"STWION", "STATION", 6, 0}, //203
        {"FMC", "FOR", 3, 0}, //204
        {"M<KN>", "OP", 5, 0}, //205
        {"QRATH", "MARATH", 5, 0}, //206
        {"UTT", "2", 5, 1}, //207
        {"MEELAY", "RELAY", 6, 0}, //208
        {"DMYN", "DOWN", 4, 0}, //209
        {"SKIRS", "STAIRS", 5, 0}, //210
        {"ATPM", "WPM", 4, 0}, //211
        {"IMTN", "ION", 4, 0}, //212
        {"ET", "A", 2, 17}, //213
        {"TT", "M", 2, 17}, //214
        {"DP", "DAN", 2, 17}, //215
        {"SITN", "SUN", 4, 0}, //216
        {"FGABT", "UP ABT", 5, 0}, //217
        {"REDRED", "RETIRED", 6, 0}, //218
        {"TSY", "BY", 3, 0}, //219
        {"AYER", "WATER", 4, 24}, //220
        {"JOTS", "JOB", 4, 0}, //221
        {"OW", "MY", 2, 17}, //222 rule 17, exact match rule
        {"WITD", "WUD", 4, 0}, //223 
        {"EROR", "FOR", 4, 17}, //224 rule 17, exact match rule
        {"KKE", "TAKE", 3, 17}, //225 rule 17, exact match rule
        {"TAC", "TAKE", 3, 17}, //226 rule 17, exact match rule
        {"LMTG", "LOG", 4, 0}, //227
        {"NMKT", "NOW", 4, 0}, //228
        {"KEKT", "KEY", 4, 0}, //229
        {"NMTW", "NOW", 4, 0}, //230
        {"MTP", "OP", 3, 0}, //231
        {"R9", "RON", 2, 0}, //232
        {"PMTMTR", "POOR", 6, 0}, //233
        {"BNTR", "BETTER", 4, 0}, //234
        {"SUANANOSED", "SUPPOSED", 10, 0}, //235
        {"TTUCH", "MUCH", 5, 0}, //236
        {"SOTTE", "SOME", 5, 0}, //237
        {"ANRO", "PRO", 4, 0}, //238
        {"BEMER", "BETTER", 5, 0}, //239
        {"LIMLE", "LITTLE", 5, 0}, //240
        {"INER", "FER", 4, 17}, //241 rule 17, exact match rule
        {"CJE", "CAME", 3, 0}, //242
        {"CMTLD", "COLD", 5, 0}, //243
        {"ENMO", "AGO", 4, 0}, //244
        {"MIERO", "ZERO", 5, 0}, //245
        {"TTTTTTT", "TOO", 7, 0}, //246
        {"CTTTVER", "COVER", 7, 0}, //247
        {"ADTH", "WITH", 4, 30}, //248
        {"LMTCK", "LOCK", 5, 0}, //249
        {"TI9", "TION", 3, 0}, //250
        {"EDAST", "LAST", 5, 0}, //251 
        {"AMIC", "ATTIC", 4, 17}, //252 rule 17, exact match rule
        {"COUBLE", "TROUBLE", 6, 0}, //253 
        {"FOEDK", "FOLK", 5, 0}, //254 
        {"TNRA", "GRA", 4, 0}, //254 
        {"TBSTT", "73", 5, 0}, //255
        {"TTATTER", "MATTER", 7, 0}, //256
        {"AMAMIING", "AMAZING", 8, 0}, //257
        {"AIES", "LES", 4, 0}, //258
        {"LMTTS", "LOTS", 5, 0}, //259
        {"MNM", "MY", 3, 0}, //260
        {"SEIDF", "SELF", 5, 0}, //261
        {"UEER", "FER", 4, 0}, //262
        {"ATHEN", "WHEN", 5, 0}, //263
        {"M9TH", "MONTH", 4, 0}, //264
        {"LPD", "LAND", 3, 0}, //265
        {"QKE", "MAKE", 3, 0}, //267
        {"QSNT", "QSK", 4, 0}, //268
        {"EMID", "WID", 4, 0}, //269
        {"SUNN", "SIGN", 4, 226}, //270
        {"GOFG", "GOING", 4, 0}, //271
        {"0ST", "MOST", 4, 0}, //272
        {"PNK", "ACK", 3, 0}, //273
        {"GFS", "GUES", 3, 0}, //274
        {"FTTTR", "FOR", 5, 0}, //275
        {"MOENN", "MORN", 5, 0}, //276
        {"KTR", "YR", 3, 0}, //277
        {"C9T", "CONT", 3, 47}, //278
        {"GEZNG", "GETTING", 4, 0}, //279
        {"XOR", "XMTR", 3, 10}, //280
        {"WW", "WAT", 2, 28}, //281
        {"FWHER", "FATHER", 5, 0}, //282
        {"SCREAB", "SCREWS", 6, 0}, //283
        {"ADSH", "WISH", 4, 0}, //284
        {"JMTY", "JOY", 4, 0}, //285
        {"BEJ", "BEAM", 3, 0}, //286
        {"LKT", "LY", 3, 0}, //287
        {"TKT", "QT", 3, 0}, //288
        {"CMA", "CQ", 3, 0}, //289
        {"DANM", "DAY", 4, 0}, //290
        {"NNPY", "CPY", 4, 0}, //291
        {"MTN", "ON", 3, 17}, //292 rule 17, exact match rule
        {"GOTI", "GOD", 4, 25}, //293
        {"COZNG", "COMING", 5, 0}, //294
        {"SUL", "SURE", 5, 17}, //295
        {"EEDSE", "ELSE", 5, 0}, //296
        {"PREMY", "PRETTY", 5, 0}, //297
        {"FEEN", "FER", 4, 0}, //298
        {"TEDL", "TELL", 4, 0}, //299
        {"FEDY", "FLY", 4, 0}, //300
        {"UEG", "UP", 3, 0}, //301
        {"D9E", "DONE", 3, 0}, //302
        {"0ST", "MOST", 3, 0}, //303
        {"MAED", "MAL", 4, 0}, //304
        {"ANIOUT", "ABOUT", 6, 0}, //305
        {"IJ", "I AM", 2, 17}, //306 rule 17, exact match rule
        {"TOTT", "TOM", 4, 17}, //307 rule 17, exact match rule
        {"WMTRK", "WORK", 5, 0}, //308
        {"ATENT", "WENT", 5, 0}, //309
        {"TII", "TH", 3, 0}, //310
        {"QIN", "MAINS", 3, 0}, //311
        {"AEAD", "RAD", 4, 0}, //312 
        {"AEIG", "RIG", 4, 0}, //313
        {"MTQEN", "MORN", 5, 0}, //314
        {"VERND", "VERTED", 5, 0}, //315
        {"0NEY", "MONEY", 4, 0}, //316
        {"MTNCE", "ONCE", 5, 0}, //317
        {"BABKT", "BABY", 5, 0}, //318
        {"MARMI", "QRZ", 5, 0}, //319
        {"TFG", "TING", 3, 0}, //320
        {"WFG", "WING", 3, 0}, //321
        {"GECD", "GARD", 4, 0}, //322
        {"BTIT", "BTU", 4, 0}, //323
        {"TMTM", "TOM", 4, 0}, //324
        {"WWER", "WATER", 4, 0}, //325
        {"LICD", "LIKED", 4, 0}, //326
        {"MAY6", "MAYBE", 4, 0}, //327
        {"MEHT", "GHT", 4, 0}, //328
        {"CLY", "KELY", 3, 0}, //329 example LIKELY
        {"MS3", "73", 3, 0}, //330
        {"0NDAY", "MONDAY", 5, 0}, //331
        {"LOME", "LOG", 4, 0}, //332
        {"D8NG", "DOING", 4, 0}, //333
        {"MOSNDY", "MOSTLY", 6, 0}, //334
        {"WARZNG", "WARMING", 6, 0}, //335
        {"COONT", "COOK", 5, 0}, //336
        {"SUENE", "SURE", 5, 0}, //337
        {"EIEN", "ESN", 4, 0}, //338
        {"GNT", "MENT", 3, 34}, //339
        {"WHETT", "WHAT", 5, 0}, //340
        {"TNREAT", "GREAT", 6, 0}, //341
        {"LOMTK", "LOOK", 5, 0}, //342
        {"COFEREE", "COFFE", 7, 0}, //343
        {"NMP", "TEMP", 3, 0}, //344
        {"WI6", "WITH", 3, 0}, //345
        {"ESTEN", "EVEN", 5, 0}, //346
        {"AUIT", "AFT", 4, 0}, //347
        {"WST", "ABT", 3, 61}, //348
        {"PEOWE", "PEOP", 5, 0}, //349
        {"MEUD", "GUD", 4, 0}, //350
        {"HASTE", "HAVE", 5, 0}, //351
        {"HOAT", "HOW", 4, 0}, //352
        {"DIWE", "DIP", 4, 0}, //353 for DIPOLE
        {"MERAS", "GRAS", 5, 0}, //354
        {"ATELL", "WELL", 5, 0}, //355
        {"AMERE", "AGRE", 5, 52}, //356
        {"SMT", "SO", 3, 0}, //357
        {"SAKT", "SAY", 4, 0}, //358
        {"HOANE", "HOPE", 5, 0}, //359
        {"WEICK", "PICK", 5, 0}, //360
        {"STERT", "VERT", 5, 0}, //361
        {"AFNR", "AFTER", 4, 0}, //362
        {"MASO", "QSO", 4, 0}, //363
        {"TTID", "MID", 4, 0}, //364
        {"KTU", "QU", 3, 31}, //365
        {"FEDAT", "FLAT", 5, 0}, //366
        {"NIKEE", "NICE", 5, 0}, //367
        {"TEQE", "TAKE", 4, 0}, //368
        {"WPTT", "WPM", 4, 0}, //369
        {"ZT", "GET", 2, 0}, //370
        {"HJ", "HAM", 2, 0}, //371
        {"VEVE", "STEVE", 4, 0}, //372
        {"SX", "STU", 2, 35}, //373 i.e. SXFF = STUFF
        {"BITG", "BUG", 4, 0}, //374
        {"VEAET", "VERT", 5, 0}, //375
        {"AIUC", "LUC", 4, 0}, //376
        {"KKT", "KY", 3, 0}, //377
        {"GOST", "GOV", 4, 0}, //378
        {"PAIE", "PLE", 4, 0}, //379 i.e. people
        {"PLR", "PAIR", 4, 0}, //380 i.e. repair
        {"DCCT", "TICKET", 4, 0}, //381
        {"EMHEN", "WHEN", 5, 0}, //382
        {"LGM", "REMEM", 3, 0}, //383 i.e. remember
        {"CTML", "COL", 4, 0}, //384
        {"PLP", "PLAN", 3, 0}, //385
        {"RMTG", "ROG", 4, 0}, //386
        {"VTTTR", "VOR", 5, 0}, //387
        {"SMMMN", "SOON", 5, 0}, //388
        {"STTT", "SO", 4, 0}, //389
        {"ATTUST", "JUST", 6, 0}, //390
        {"TTTTT", "1", 5, 0}, //391
        {"ANLE", "PLE", 4, 0}, //392
        {"CCD", "CKED", 3, 0}, //393
        {"ADD", "ADD(WID)", 3, 33}, //394
        {"G9", "GON", 2, 36}, //395
        {"UT0", "30", 3, 0}, //396
        {"LANR", "LATERE", 4, 0}, //397
        {"ESNAG", "ES NAME", 5, 0}, //398
        {"MEZE", "MADE", 4, 0}, //399
        {"0NI", "OMNI", 3, 0}, //400
        {"LMT", "LO", 3, 0}, //401 i.e LOOK
        {"THFE", "THERE", 4, 0}, //402
        {"C9S" , "CONS", 3, 0}, //403
        {"LIC" , "LIKE", 3, 17}, //404 rule 17, exact match rule
        {"ZD" , "MID", 2, 0}, //405
        {"AMEAST" , "JUST", 6, 0}, //406
        {"7VT" , "73", 3, 0}, //407
        {"BMQT" , "BOY", 4, 0}, //408
        {"ATX" , "WX", 3, 0}, //409
        {"CRTTYES" , "CROPS", 7, 0}, //410
        {"W9T" , "WONT", 3, 0}, //411
        {"<AR>A" , "CA", 5, 65}, //412
        {"BHT" , "BEST", 3, 0}, //413
        {"LICL" , "LIKEL", 4, 0}, //414
        {"MERO" , "GRO", 4, 48}, //415
        {"TIONT" , "DONT", 5, 0}, //416
        {"RISB" , "RISTS", 6, 72}, //417
        {"RA6O" , "RADIO", 4, 0}, //418
        {"OI0" , "80", 3, 0}, //419
        {"NOVURE" , "NOVICE", 6, 0}, //420
        {"CAVEL" , "TRAVEL", 5, 0}, //421
        {"YFG" , "YING", 3, 0}, //422
        {"16AO" , "161", 3, 0}, //423
        {"COWEY" , "COPY", 5, 0}, //424
        {"EUE" , "EF", 5, 0}, //425 i.e. BAREUEOOT = BAREFOOT
        {"TSET" , "BET", 4, 0}, //426
        {"CTMANY" , "COPY", 6, 0}, //427
        {"EUOLK" , "FOLK", 5, 0}, //428
        {"TMVER" , "OVER", 5, 0}, //429
        {"DETVE" , "DAVE", 5, 0}, //430
        {"SO9", "SOON", 3, 0}, //431
        {"STERT", "VERT", 5, 0}, //432
        {"MT0D", "MOOD", 4, 0}, //433
        {"UEB", "FB", 3, 0}, //434
        {"THMTN", "THON", 5, 0}, //435
        {"WAKT", "WAY", 4, 0}, //436
        {"ITRE", "ICE", 4, 0}, //437
        {"ETST", "AST", 4, 0}, //438
        {"0V", "MOV", 2, 77}, //439
        {"6IN", "THIN", 2, 0}, //440
        {"EDONG", "LONG", 5, 0}, //441
        {"6IN", "THIN", 2, 0}, //442
        {"EMITD", "WUD", 5, 0}, //443
        {"HMTE", "HOME", 4, 0}, //444
        {"JWAE", "JAKE", 4, 0}, //445
        {"JP", "AMP", 2, 0}, //446
        {"D9", "DON", 2, 42}, //447
        {"MKNR", "OWER", 4, 0}, //448  i.e. SHMKNRS = SHOWERS
        {"ARMXND", "AROUND", 6, 0}, //449
        {"MTF", "OF", 3, 0}, //450
        {"IIAVE", "HAVE", 4, 0}, //451
        {"FB9", "FB ON", 3, 0}, //452
        {"REARN", "LEARN", 5, 0}, //453
        {"AEEARN", "LEARN", 6, 0}, //454
        {"FAEOM", "FROM", 7, 0}, //455
        {"1ON", "19", 3, 0}, //456
        {"MANNOKS" , "MANY TNKS", 7, 0}, //457
        {"NTEY", "KEY", 4, 0}, //458
        {"0OTT", "00", 4, 0}, //459
        {"OET", "OA", 4, 53}, //460 i.e. COETX = COAX
        {"I<KN>", "ING", 5, 0}, //461
        {"FLJE", "FLAME", 4, 0}, //462
        {"6RU", "THRU", 3, 0}, //463
        {"UAIL", "ULL", 4, 0}, //464
        {"7SM", "73", 3, 0}, //465
        {"CAG", "CAME", 3, 17}, //466 rule 17, exact match rule
        {"MEHB", "GHB", 3, 0}, //467  i.e. neiMEHBor = NEIGHBOR
        {"LMM", "LOT", 3, 0}, //468
        {"QRED", "QRL", 4, 0}, //469
        {"MEHT", "GHT", 4, 0}, //470
        {"MENR", "METER", 4, 0}, //471
        {"CEGY", "CPY", 4, 0}, //472
        {"PATR", "PWR", 4, 0}, //473
        {"9L", "ONL", 2, 0}, //474   i.e. 9Ly = ONLY
        {"INIR", "FIR", 4, 0}, //475
        {"OI9" , "89", 3, 0}, //476
        {"MEP" , "MEAN", 3, 0}, //477
        {"B2" , "BUT", 2, 50}, //478
        {"IUE" , "IF", 3, 0}, //479
        {"UTI" , "?", 3, 39}, //480 //skip for 'beautiful'
        {"NTN" , "NG", 3, 44}, //481
        {"EMAY" , "WAY", 4, 67}, //482
        {"ZE3" , "73", 3, 0}, //483
        {"ENUN" , "RUN", 4, 0}, //484
        {"NTMAT" , "NOW", 5, 0}, //485
        {"ATNE" , "AGE", 4, 76}, //486
        {"ISTE", "IVE", 4, 19}, //487
        {"WM9", "19", 3, 0}, //488
        {"AIID", "LID", 4, 0}, //489 soAIID = solid
        {"INUN", "FUN", 4, 0}, //490
        {"PNETTY", "PRTTY", 6, 0}, //491
        {"PFG", "PING", 3, 0}, //492
        {"OITS", "8TS", 4, 46}, //493
        {"ANDT", "WAIT", 4, 41}, //494
        {"OITT", "OUT", 4, 0}, //495
        {"6TTO", "60", 4, 0}, //496
        {"VKT", "VY", 3, 0}, //497
        {"ZST", "MIST", 3, 0}, //498
        {"5OI", "58", 3, 0}, //499
        {"TWA", "TAK", 3, 45}, //500
        {"MASB", "QSB", 4, 0}, //501
        {"NETVNM", "NAVY", 6, 0}, //502
        {"RETDITM", "RADIO", 6, 0}, //503
        {"TTAN", "MAN", 6, 43}, //504
        {"PRE", "WERE", 3, 17}, //505
        {"5TOT", "50", 4, 0}, //506
        {"YECD", "YARD", 4, 17}, //507
        {"0RE", "MORE", 3, 0}, //508
        {"S<AS>", "SRI", 5, 0}, //509
        {"HOG", "HOME", 3, 0}, //510
        {"TH2B", "THUMB", 4, 0}, //511
        {"CLDNY", "CLDY", 5, 0}, //512
        {"MITCH", "MUCH", 5, 0}, //513
        {"INRAM", "FRAM", 5, 0}, //514
        {"SITEN", "SUR", 5, 0}, //515
        {"CAEN", "CAR", 4, 0}, //516
        {"PO<AS>", "POLE", 6, 0}, //517
        {"Q<AA>", "QRT", 5, 0}, //518
        {"CEQ", "CANT", 3, 66}, //519
        {"SKY", "SKY(STAY)", 3, 0}, //520
        {"AGREW", "A GREAT", 5, 0}, //521
        {"CMTU", "COU", 4, 0}, //522 //couple
        {"6W", "THAT", 2, 0}, //523
        {"OAIE", "OLE", 4, 0}, //524 //dipOAIE = DIPOLE
        {"KKRD", "YARD", 4, 0}, //525
        {"QAEN", "QRN", 4, 0}, //526
        {"DJN", "DAMN", 3, 0}, //527
        {"GX" , "TNX", 2, 0}, //528
        {"CIOB" , "CUMB", 4, 0}, //529
        {"KEEWE" , "KEEP", 4, 0}, //530
        {"RANNE" , "RACE", 5, 0}, //531
        {"SWEEC" , "SPEC", 5, 0}, //532
        {"RAD2" , "RADIO", 4, 0}, //533
        {"BIAIE" , "BILE", 5, 0}, //534
        {"HEA6" , "HEATH", 4, 0}, //535
        {"CPS" , "TRANS", 3, 0}, //536
        {"BRITCE" , "BRUCE", 6, 0}, //537
        {"OI2" , "82", 3, 0}, //538
        {"JITST" , "JUST", 5, 0}, //539
        {"OATN" , "OWN", 4, 0}, //540
        {"GOT8" , "GOOD", 4, 0}, //541
        {"FJI" , "FAMI", 3, 0}, //542
        {"NNAN" , "CAN", 4, 0}, //543
        {"WILED" , "WILL", 5, 0}, //544
        {"EDD" , "LD", 3, 0}, //545
        {"ENET" , "GET", 4, 217}, //546 //i,e, RAdio
        {"HORMI" , "HORZ", 5, 0}, //547
        {"CULTS" , "CLUB", 5, 0}, //548 //super sloppy bug
        {"EBEEDIEET" , "781", 9, 200}, //549//super sloppy bug
        {"STRETTNEN" , "STRONG", 9, 200}, //550//super sloppy bug
        {"BEETDT" , "BOY", 6, 200}, //551 //super sloppy bug
        {"THEET" , "THEM", 5, 200}, //552 //super sloppy bug
        {"THETT" , "THEM", 5, 200}, //553 //super sloppy bug
        {"EETAT" , "OW", 5, 200}, //554//super sloppy bug
        {"EETIT" , "OW", 5, 200}, //555//super sloppy bug
        {"ETTAT" , "OW", 5, 200}, //556//super sloppy bug
        {"EBEEEET" , "70", 7, 200}, //557//super sloppy bug
        {"CEMRINS" , "CORPS", 7, 255}, //558 //super sloppy bug
        {"TEMWES" , "TEMPS", 6, 0}, //559 //super sloppy bug
        {"ENEN" , "GG", 4, 217}, //560//super sloppy bug
        {"TNE" , "GE", 3, 260}, //561 //super sloppy bug
        {"ENE" , "GE", 3, 254}, //562 //super sloppy bug
        {"UU" , "OO", 2, 200}, //563 //super sloppy bug
        {"EEX" , "OW", 3, 200}, //564 //super sloppy bug
        {"ETE" , "ME", 3, 256}, //565 //super sloppy bug
        {"INEN" , "ING", 4, 200}, //556 //super sloppy bug
        {"THEX" , "THEY", 4, 200}, //567 //super sloppy bug
        {"THEDT" , "THEY", 5, 200}, //568 //super sloppy bug
        {"BODT" , "BOY", 4, 200}, //569 //super sloppy bug
        {"EDEET" , "LO", 5, 200}, //570 //super sloppy bug
        {"LOIT" , "LOW", 4, 200}, //571 //super sloppy bug
        {"FROET" , "FROM", 5, 200}, //572 //super sloppy bug
        {"FRTTTET" , "FROM", 7, 200}, //573 //super sloppy bug
        {"EKRET" , "QRM", 5, 200}, //574 //super sloppy bug
        {"AETP" , "AMP", 4, 200}, //575 //super sloppy bug
        {"AETIN" , "AMP", 5, 200}, //576 //super sloppy bug
        {"AEN" , "AG", 3, 200}, //577 //super sloppy bug
        {"EECS" , "ORS", 4, 200}, //578 //super sloppy bug
        {"EE/" , "OF", 3, 200}, //579 //super sloppy bug
        {"KEEIN" , "KEEP", 5, 200}, //580 //super sloppy bug
        {"KE EIN" , "KEEP", 6, 200}, //581 //super sloppy bug
        {"FETTR" , "FOR", 5, 200}, //582 //super sloppy bug
        {"RETTET" , "ROM", 6, 200}, //583 //super sloppy bug
        {"BKT" , "BY", 3, 258}, //584 //super sloppy bug
        {"LETTETTK" , "LOOK", 8, 200}, //585 //super sloppy bug
        {"EGR" , "OF", 3, 268}, //586 //super sloppy bug
        {"BUMES" , "BUGS", 5, 0}, //587
        {"RPD" , "RAND", 3, 0}, //588
        {"NDDS" , "KIDS", 4, 0}, //589
        {"NETT" , "NO", 4, 278}, //590
        {"MT7NDY" , "MOSTLY", 6, 0}, //591 
        {"SEET" , "SO", 4, 200}, //592
        {"EKSETT" , "QSO", 4, 200}, //593
        {"NNK", "CK", 3, 0}, //594
        {"RIEN", "RIG", 4, 262}, //595
        {"IENH", "IGH", 4, 200}, //596
        {"EBEEO", "70", 5, 200}, //597
        {"EBETD", "78", 5, 200}, //598
        {"ATTT", "1", 4, 200}, //599
        {"EEEN", "9", 4, 200}, //600
        {"TTTRK", "ORK", 5, 200}, //601
        {"MTOP", "OOP", 4, 200}, //602
        {"EB6", "76", 3, 200}, //603
        {"HAET", "HAM", 4, 200}, //604
        {"TTETT", "MO", 5, 200}, //605
        {"SETUWE", "SETUP", 6, 0}, //606
        {"CHEAT", "CHEW", 5, 200}, //607
        {"GEN", "GG", 4, 217}, //608
        {"TETTDADT", "TODAY", 8, 200}, //609
        {"NETT", "NOT", 4, 200}, //610
        {"ICOET", "ICOM", 5, 200}, //611
        {"TETTMETT", "TOMO", 8, 200}, //612
        {"BOKT", "BOY", 4, 200}, //613
        {"RETT", "RO", 4, 271}, //614
        {"ANATH", "PATH", 5, 200}, //615
        {"LOOIN", "LOOP", 5, 200}, //616
        {"ATHILE", "WHILE", 6, 200}, //617
        {"COET", "COM", 4, 200}, //618
        {"EET" , "O", 3, 257}, //619//super sloppy bug
        {"7OI1" , "781", 3, 257}, //620//super sloppy bug
        {"MTMEAN" , "OOP", 6, 257}, //621//super sloppy bug
        {"CMMT" , "COM", 4, 257}, //622//super sloppy bug
        {"ENET" , "RA", 4, 200}, //623 //i,e, RAdio
        {"ENENIN" , "ENGIN", 6, 200}, //624//super sloppy bug
        {"NETTT" , "NOT", 5, 200}, //625//super sloppy bug
        {"SIEN" , "SIG", 4, 217}, //626//super sloppy bug
        {"RI EN" , "RIG", 5, 217}, //627//super sloppy bug
        {"SETT" , "SO", 4, 217}, //628//super sloppy bug
        {"I CEETET" , "ICOM", 8, 200}, //629//super sloppy bug
        {"STRTTTNEN" , "STRONG", 9, 200}, //630//super sloppy bug
        {"ET ETTST LKT" , "MOSTLY", 12, 200}, //631//super sloppy bug
        {"L MTMTP" , "LOOP", 7, 200}, //632//super sloppy bug
        {"XSEMEU" , "XIEGU", 6, 200}, //633//super sloppy bug
        {"2MO" , "20", 3, 200}, //634//super sloppy bug
        {"EKUIET" , "QUIET", 6, 200}, //635//super sloppy bug
        {"WEMRK" , "WORK", 5, 200}, //636//super sloppy bug
        {"AUIN" , "RAIN", 3, 0}, //637//super sloppy bug
        {"TETT" , "UP", 4, 217}, //638//super sloppy bug
        {"TMEAR" , "YEAR", 5, 200}, //639//super sloppy bug
        {"ANLAKTINTN" , "YEAR", 10, 200}, //640//super sloppy bug
        {"ST9" , "STON", 3, 0}, //641
        {"9MME" , "99", 4, 0}, //642
        {"MME0" , "90", 4, 0}, //643
        {"CETTN" , "CON", 5, 200}, //644
        {"CHKT" , "CHY", 4, 200}, //645
        {"CYING" , "TRYING", 5, 200}, //646
        {"ENUD" , "GUD", 4, 200}, //647
        {"BREEMIY" , "BREEZY", 6, 200}, //648
        {"ATT" , "AM", 3, 217}, //649
        {"WTP" , "BREEZY", 3, 217}, //650
        {"H4KT" , "HVY", 4, 200}, //651
        {"ATID" , "WID", 4, 200}, //652
        {"WMT" , "WO", 3, 200}, //653 //wood
        {"SM0" , "30", 3, 200}, //654
        {"OSTER" , "OVER", 3, 200}, //655
        {"UWEL" , "UPL", 4, 200}, //656 //couple
        {"INRI" , "FRI", 4, 200}, //657 //friend
        {"STERY" , "VERY", 5, 200}, //658 
        {"AIIKE" , "LIKE", 5, 200}, //659
        {"ATAS" , "WAS", 4, 0}, //660
        {"7MS" , "77", 3, 0}, //661
        {"VT5" , "35", 3, 0}, //662
        {"MEREAT" , "GREAT", 6, 0}, //663
        {"EM" , "O", 2, 255}, //664 //super sloppy bug
        {"ETU", "MU", 3, 263}, //665 //super sloppy bug
        {"PH9E" , "PHONE", 4, 0}, //666
        {"ATANT" , "WANT", 5, 0}, //667
        {"ATANT" , "WANT", 5, 0}, //668
        {"RPG" , "RANG", 3, 0}, //669
        {"LONTN" , "LONG", 5, 0}, //670
        {"MXR" , "OUR", 3, 0}, //671
        {"5ON9" , "599", 4, 0}, //672
        {"AMEE" , "AGE", 4, 0}, //673
        {"ANM" , "PM", 3, 17}, //674
        {"TITESDAY" , "TUESDAY", 8, 0}, //675
        {"9NCE" , "ONCE", 4, 0}, //676
        {"WPB" , "WANTS", 3, 0}, //677
        {"SYI" , "STATI", 3, 0}, //678
        {"PMTT" , "ANOT", 4, 0}, //679
        {"TTC" , "MC", 3, 0}, //680
        {"P9SER" , "ANOTHER", 5, 0}, //681
        {"NJE" , "NAME", 5, 0}, //682
        {"G8NG" , "GOING", 4, 0}, //683
        {"ATIFE" , "WIFE", 5, 0}, //684
        {"BUME" , "BUG", 4, 0}, //685
        {"NMM" , "9", 3, 0}, //686
        {"II" , "ES", 2, 0}, //687
        {"FRTT" , "FRM", 4, 0}, //688
        {"WEOST" , "POST", 5, 0}, //689
        {"TTIKE" , "MIKE", 2, 0}, //690
        {"SUTTTTER" , "SUMMER", 8, 0}, //691
        {"TDIN" , "ZIN", 4, 0}, //692 //BLAtdinG = BLAZING
        {"LAMIY" , "LAZY", 5, 0}, //693
        {"UIN" , "UP", 3, 274}, //694//super sloppy bug
        {"IEO" , "SO", 3, 0}, //695
        {"WXP" , "JU", 3, 0}, //696 //JuPiter, FL
        {"1TY" , "1TT W", 3, 0}, //697
        {"OK9" , "OK ON", 3, 0}, //698
        {"ACDVITY" , "ACTIVITY", 7, 0}, //699
        {"REZIO" , "RADIO", 5, 0}, //700
        {"NMT" , "NO", 3, 200}, //701
        {"C2ING" , "CUMING", 5, 0}, //702
        {"CANKT" , "CPY", 5, 0}, //703
        {"LOOTA" , "LOOK", 5, 0}, //704
        {"ARTMUND" , "AROUND", 7, 0}, //705
        {"KFG" , "KING", 3, 0}, //706
        {"6IER" , "THIER", 4, 0}, //707
        {"SEPCAN" , "SEPARATE", 6, 0}, //708
        {"UN2" , "UNUM", 3, 0}, //709
        {"TH5" , "THIS", 3, 0}, //710
        {"NO9", "NOON", 3, 0}, //711
        {"HETD", "HAD", 3, 0}, //712
        {"NZR", "NGER", 3, 0}, //713
        {"MIC", "MIKE", 3, 17}, //714
        {"ORIME", "ORIG", 5, 0}, //715
        {"THETK", "THEY", 5, 0}, //716
        {"SIMAN", "SIMP", 5, 0}, //717
        {"STERKT", "VERY", 6, 0}, //718
        {"SOECCE", "SOURCE", 6, 0}, //719
        {"AMEO", "AGO", 4, 0}, //720
        {"KMME", "K9", 4, 0}, //721
        {"NITNS", "NICS", 5, 0}, //722
        {"ALSETT", "ALSO", 6, 200}, //723
        {"MSVT", "73", 4, 200}, //724
        {"KOTE", "K9", 4, 200}, //725
        {"KON", "K9", 3, 200}, //726
        {"KTOE", "K9", 4, 200}, //727
        {"MOW", "K9", 5, 200}, //728
        {"TTSVT", "73", 5, 0}, //729
        {"NMY", "NOW", 3, 200}, //730
        {"N9EY", "MONEY", 4, 0}, //731
        {"HEJ", "HEAT", 3, 0}, //732
        {"T9ITE", "TONITE", 5, 0}, //733
        {"THEKT", "THEY", 5, 0}, //734
        {"6OSE", "THOSE", 4, 0}, //735
        {"CHPCE", "CHANCE", 5, 0}, //736
        {"5RE", "HERE", 3, 0}, //737
        {"GRPITE", "GRANITE", 5, 0}, //738
        {"KEEAN", "KEEP", 5, 0}, //739
        {"CEE", "TREE", 3, 0}, //740
        {"CTK", "CQ", 3, 0}, //741
        {"BEFTMRE", "BEFORE", 7, 0}, //742
        {"CIP", "TRIP", 3, 17}, //743
        {"RITN", "RIG", 4, 0}, //744
        {"CHLR", "CHAIR", 4, 0}, //745
        {"NDND", "KIND", 4, 0}, //746
        {"DME", "TIME", 3, 200}, //747
        {"DG", "TIME", 2, 17}, //748
        {"P8NT", "POINT", 4, 0}, //749
        {"TGR", "OF", 2, 0}, //750
        {"XKKI", "XYL", 2, 200}, //751
        {"ATHAT", "WHAT", 5, 0}, //752
        {"EQIL", "EMAIL", 4, 0}, //753
    };
    
    bool AllDah;
    bool AllDit;
    bool NewSpltVal;
    bool StrchdDah; //used mainly to steer which rules to apply within the Bug1 rule set (when long dahs are detected certain simple rules are bypassed)
    int BugKey;//controls wich parsing rules are used 0 = paddle; 1 = bug; 2 = cootie
    int MaxCntKyUpBcktPtr;

    uint8_t ExitPath[IntrvlBufSize];
    uint8_t ExitPtr; 
	Buckt_t KeyUpBuckts[15];
    Buckt_t KeyDwnBuckts[15];
    uint8_t SameSymblWrdCnt = 0;
    int KeyUpBucktPtr = 0;
    int KeyDwnBucktPtr = 0;
    int TmpUpIntrvlsPtr = 0;
    int StrLength = 0; //MsgBuf indx pointer to character printed as Debug output
    //int wpm =0; //upated from DcodeCW.cpp through this class method EvalTimeData()
    float DahVarPrcnt;
    float MaxDt2DhRatio;
    uint16_t TmpUpIntrvls[IntrvlBufSize];
    uint16_t TmpDwnIntrvls[IntrvlBufSize];
    uint16_t DitDahSplitVal;
    uint16_t AvgDahVal;
    uint16_t NuSpltVal = 0;
    //uint16_t DitIntrvlVal; //used as sanity test/check in 'bug' letterbrk rule set; 20240129 running average of the last 6 dits
    uint16_t WrdBrkVal; // serves in post parser as the value to insert a space in the reconstructed character string
    unsigned int SymbSet;
    unsigned int LstLtrBrkCnt = 0;//track the number of keyevents since last letterbreak.
    //uint16_t UnitIntvrlx2r5; //basic universal symbol interval; i.e. a standard dit X 2.4; used in b1 rule set to find letter breaks
    uint16_t Bg1SplitPt; //bug1 rule set dit/dah decision value; derived from UnitIntvrlx2r5
    char BrkFlg;
    /* Methods */
    bool Tst4LtrBrk(int& n);
    bool PadlRules(int& n);
    bool Bug1Rules(int& n);
    bool Bug2Rules(int& n);
    bool CootyRules(int& n);
    bool Cooty2Rules(int& n);
    bool SloppyBgRules(int& n);
    bool SKRules(int& n); //true straight key(i.e. J38)
    void insertionSort(uint16_t arr[], int n);
	void SetSpltPt(Buckt_t arr[], int n);
    int AdvSrch4Match(int n, unsigned int decodeval, bool DpScan);
    bool SrchAgn(int n);
    int BldCodeVal(int Start, int LtrBrk);
    int FindLtrBrk(int Start, int End);
    void SyncTmpBufA(void);
    void PrintThisChr(void);
    int DitDahBugTst(void); //returns 2 for unknown; 0 for paddle; 1 for bug
    void Dcode4Dahs(int n);
    void FixClassicErrors(void);
    int SrchEsReplace(int MsgBufIndx, int STptr, const char srchTerm[10], const char NuTerm[10]);
    char TmpBufA[MsgbufSize - 5];

public:
	AdvParser(void); //TFT_eSPI *tft_ptr, char *StrdTxt
    //void EvalTimeData(uint16_t KeyUpIntrvls[IntrvlBufSize], uint16_t KeyDwnIntrvls[IntrvlBufSize], int KeyUpPtr, int KeyDwnPtr, int curWPM);
	void EvalTimeData(void);
    char Msgbuf[MsgbufSize];
    char LtrHoldr[30];
    int LtrPtr = 0;
    /*controls debug USB serial print*/
    bool Dbug = false;
    int KeyType = 0;//used for the display's status indicator("S" or "E")
    float AvgSmblDedSpc;
    float wrdbrkFtcr;
    int GetMsgLen(void);
    unsigned long LstGltchEvnt; //time stamp of last detected glitch (detected in/by Geoertzel.cpp)
    uint16_t DitIntrvlVal;//20240521 moved this from privat to public so Goertzel.cpp could see it
    uint16_t KeyUpIntrvls[IntrvlBufSize];
    uint16_t KeyDwnIntrvls[IntrvlBufSize];
    uint16_t UnitIntvrlx2r5; //basic universal symbol interval; i.e. a standard dit X 2.4; used in b1 rule set to find letter breaks
    int KeyUpPtr = 0;
    int KeyDwnPtr = 0;
    int wpm =0; //upated from DcodeCW.cpp
	
};

#endif /* INC_ADVPARSER_H_ */