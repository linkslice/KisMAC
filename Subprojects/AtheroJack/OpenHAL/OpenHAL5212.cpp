/*
 *  OpenHAL5212.cpp
 *  AtheroJack
 *
 *  Created by mick on 30.03.2005.
 *  Copyright 2005 __MyCompanyName__. All rights reserved.
 *
 */

#include "OpenHAL5212.h"
#include "ar5212var.h"
#include "ar5212reg.h"
#include "../WiFiLogger.h"

#define etherbroadcastaddr "\xFF\xFF\xFF\xFF\xFF\xFF"
static const struct ar5k_ini_rf ar5112_rf_tofix[] = AR5K_AR5112_INI_RF;

OSDefineMetaClassAndStructors(OpenHAL5212, OpenHAL);

static const struct ar5k_ar5212_ini ar5212_ini[] =
    AR5K_AR5212_INI;
static const struct ar5k_ar5212_ini_mode ar5212_mode[] =
    AR5K_AR5212_INI_MODE;

	UInt16 firmwareWAG511[] = {
0x0013, 0x168c, 0x0200, 0x0001, 0x0000, 0x5001, 0x0000, 0x4610, 0x1385, 0x1c0a, 0x0100, 0x0000, 0x0002, 0x0002, 0xc606, 0x0001, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1612, 0x5ba0, 0x0009, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x5aa5, 0x0000, 0x0000, 
0x0313, 0x4943, 0x2053, 0x7104, 0x1202, 0x0400, 0x0306, 0x0001, 0x0000, 0x0500, 0x410e, 0x39b1, 0x1eb5, 0x4e2d, 0x3056, 0xffff, 
0xe902, 0x0700, 0x0106, 0x0000, 0x0100, 0x1500, 0x0752, 0x4101, 0x6874, 0x7265, 0x736f, 0x4320, 0x6d6f, 0x756d, 0x696e, 0x6163, 
0x6974, 0x6e6f, 0x2c73, 0x4920, 0x636e, 0x002e, 0x5241, 0x3035, 0x3130, 0x302d, 0x3030, 0x2d30, 0x3030, 0x3030, 0x5700, 0x7269, 
0x6c65, 0x7365, 0x2073, 0x414c, 0x204e, 0x6552, 0x6566, 0x6572, 0x636e, 0x2065, 0x6143, 0x6472, 0x3000, 0x0030, 0x00ff, 0x2100, 
0x0602, 0x2201, 0x0205, 0x8d80, 0x005b, 0x0522, 0x4002, 0x8954, 0x2200, 0x0205, 0x1b00, 0x00b7, 0x0522, 0x8002, 0x12a8, 0x2201, 
0x0205, 0x3600, 0x016e, 0x0522, 0x0002, 0x2551, 0x2202, 0x0205, 0x6c00, 0x02dc, 0x0522, 0x8002, 0x37f9, 0x2203, 0x0205, 0xa200, 
0x044a, 0x0222, 0x0803, 0x0822, 0x0604, 0x0900, 0xa05b, 0x1216, 0x0222, 0x0105, 0x00ff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0003, 0x0000, 
0x27f3, 0x4005, 0x0a07, 0x0202, 0x4205, 0x019b, 0x0302, 0x1801, 0x0003, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x2d3c, 0x0081, 0x0000, 0x0108, 0x0000, 0xe049, 0x2492, 0x020f, 0x000e, 0xb0ca, 0x21a3, 0x4024, 
0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x2398, 0x0091, 0x0410, 0x4148, 0x2082, 0xda22, 0x021c, 0x0007, 0xb0ff, 0x01a3, 0x4012, 0x0001, 0xac70, 0x17b8, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x3198, 0x0091, 0x0410, 
0x4148, 0x2082, 0xe022, 0x021c, 0x000e, 0xb0ff, 0x21a3, 0x4012, 0x0051, 0xac70, 0x1320, 0x17b8, 0x0000, 0x0003, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1013, 0x1112, 0x1440, 0x3031, 0x3234, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x564a, 0x7864, 0xaa8c, 0xcdb9, 0x0000, 0x3113, 0x5041, 0x1155, 0x0000, 0x1900, 0x3214, 0x5042, 0x0d55, 0x0000, 0x1900, 0x3418, 
0x5043, 0x0d35, 0x0000, 0x1900, 0x361b, 0x5047, 0x0955, 0x0000, 0x1900, 0x381c, 0x5045, 0x0d34, 0x0000, 0x1900, 0x4320, 0x5050, 
0x0159, 0x0000, 0x1900, 0x4e22, 0x4e4e, 0x0599, 0x0000, 0x1900, 0x4a24, 0x4a4a, 0x0599, 0x0000, 0x1900, 0x4124, 0x504f, 0x0135, 
0x0000, 0x1900, 0x4225, 0x5050, 0x0115, 0x0000, 0x1900, 0x3d21, 0x5050, 0x0135, 0x0000, 0x1900, 0x3f20, 0x5050, 0x0137, 0x0000, 
0x1900, 0x3a20, 0x5048, 0x0934, 0x0000, 0x1900, 0x3d22, 0x504d, 0x0135, 0x0000, 0x1900, 0x4c92, 0x071a, 0x5092, 0x071a, 0x6892, 
0x071a, 0x8c92, 0x071a, 0xb481, 0xc696, 0xbd81, 0xc696, 0xcd81, 0xc696, 0xd281, 0xc696, 0x70a2, 0x8a28, 0xb8a2, 0x8a28, 0x709a, 
0x481e, 0x939a, 0x481e, 0xac9a, 0x481e, 0x4c58, 0x5c68, 0xbdc1, 0xcd00, 0x2222, 0x2423, 0x2464, 0x2400, 0x5052, 0x5a60, 0x62c0, 
0xc1c9, 0x2262, 0x2224, 0x2423, 0x6323, 0x7075, 0xa200, 0x0000, 0x0000, 0x2868, 0x2800, 0x0000, 0x0000, 0x7075, 0x9da2, 0x0000, 
0x0000, 0x2166, 0x6621, 0x0000, 0x0000, 0x8989, 0x0000, 0x0000, 0x0000, 0x2626, 0x0000, 0x0000, 0x0000, 0x4a56, 0x0000, 0x0000, 
0x0000, 0x3c3c, 0x0000, 0x0000, 0x0000, 0x4c68, 0x8cb4, 0xbdc1, 0xcd00, 0x2424, 0x2424, 0x3c7c, 0x3c00, 0x7075, 0xac00, 0x0000, 
0x0000, 0x2060, 0x2000, 0x0000, 0x0000, 0x7075, 0xac00, 0x0000, 0x0000, 0x1d5d, 0x1d00, 0x0000, 0x0000, 0x8989, 0x0000, 0x0000, 
0x0000, 0x1e1e, 0x0000, 0x0000, 0x0000, 0xea0e, 0xc000, 0x0e07, 0x8042, 0xa204, 0x0002, 0x0e03, 0x8186, 0x9858, 0x0000, 0x0fff, 
0x8225, 0xa214, 0x000c, 0x05ff, 0xc0fd, 0x1001, 0xc8f5, 0x707f, 0xc8fc, 0x1000, 0xdc92, 0x1001, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xcccc, 0xcccc, 0xcccc, 0xcccc, 0xcccc, 0xcccc, 0xcccc, 0xcccc, 0xcccc, 0xcccc, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x0405, 0x0006, 
0xbad3, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
};
	UInt16 firmware[] = {
0x0013, 0x168c, 0x0200, 0x0001, 0x0000, 0x5001, 0x0000, 0xaa40, 0x14b7, 0x1c0a, 0x0100, 0x0000, 0x0002, 0x0002, 0xc606, 0x0001, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x280b, 0xa64d, 0x0020, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x5aa5, 0x0001, 0x0000, 
0x0313, 0x4943, 0x2053, 0x2604, 0x1301, 0x0400, 0x0306, 0x0001, 0x0000, 0x0500, 0x410e, 0x39b1, 0x1eb5, 0x4e2d, 0x3056, 0xffff, 
0xe902, 0x0700, 0x0106, 0x0000, 0x0100, 0x1500, 0x0752, 0x5001, 0x6f72, 0x6978, 0x2c6d, 0x4320, 0x726f, 0x2e70, 0x3800, 0x3230, 
0x312e, 0x6731, 0x5720, 0x7269, 0x6c65, 0x7365, 0x2073, 0x7055, 0x7267, 0x6461, 0x2065, 0x694b, 0x2e74, 0x4300, 0x5349, 0x5220, 
0x5645, 0x3120, 0x322e, 0x4300, 0x706f, 0x7279, 0x6769, 0x7468, 0x3220, 0x3030, 0x0033, 0x00ff, 0x0000, 0x0000, 0x0000, 0x2100, 
0x0602, 0x2201, 0x0205, 0x8d80, 0x005b, 0x0522, 0x4002, 0x8954, 0x2200, 0x0205, 0x1b00, 0x00b7, 0x0522, 0x8002, 0x12a8, 0x2201, 
0x0205, 0x3600, 0x016e, 0x0522, 0x0002, 0x2551, 0x2202, 0x0205, 0x6c00, 0x02dc, 0x0522, 0x8002, 0x37f9, 0x2203, 0x0205, 0xa200, 
0x044a, 0x0222, 0x0803, 0x0822, 0x0604, 0x0020, 0xa64d, 0x280b, 0x0222, 0x0105, 0x00ff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x1070, 0x0101, 0x0000, 0x0000, 0x3033, 0x4d54, 0x3239, 0x3031, 0x3339, 0x3338, 0xffff, 0x0000, 0x0000, 0x0000, 0xffff, 0x0060, 
0x13e6, 0x3004, 0x0ff6, 0x0b06, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x2d2c, 0x0000, 0x0000, 0x0000, 0x0000, 0xe028, 0xa492, 0x1c00, 0x000e, 0xb8ca, 0x01bb, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x2d3c, 0x0118, 0x2100, 0x0850, 0x4400, 0xda22, 0x021c, 0x0007, 0xb0ff, 0x01b7, 0x4012, 0x0001, 0x0170, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x233c, 0x0118, 0x2100, 
0x0850, 0x4400, 0xe022, 0x021c, 0x000e, 0xb0ff, 0x01b7, 0x401b, 0x0079, 0x8e70, 0x0020, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1112, 0x3132, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xa875, 0x5659, 0x865b, 0x71e7, 0xe088, 0xc0e5, 0x5761, 0xa6dd, 0x79f8, 
0x2188, 0xc0e5, 0x5761, 0xa6dd, 0x79f8, 0x2188, 0xd966, 0x5b71, 0xe79f, 0x8208, 0x6288, 0xd966, 0x5b71, 0xe79f, 0x8208, 0x6288, 
0xd945, 0xd969, 0xc71e, 0x79f8, 0x2188, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7079, 0xe79e, 0xb879, 0xe79e, 0x7079, 0xc698, 0x9379, 0xc698, 0xac79, 0xc698, 0x7075, 
0x7a84, 0x8e9d, 0xa2a7, 0x2424, 0x2424, 0x2424, 0x2424, 0x7075, 0x7a84, 0x8e9d, 0xa2a7, 0x1e24, 0x2424, 0x2424, 0x1e24, 0x7075, 
0x7a84, 0x8e9d, 0xa7ac, 0x2424, 0x2424, 0x2424, 0x2424, 0x7075, 0x7a84, 0x8e9d, 0xa7ac, 0x1e1e, 0x1e1e, 0x1e1e, 0x1e1e, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
0xbad1, 0x0205, 0x0013, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 	
};

//place holder
HAL_BOOL OpenHAL5212::nic_ath_hal_attach(u_int16_t device, HAL_BUS_TAG st, HAL_BUS_HANDLE sh, HAL_STATUS *status) {
	u_int8_t mac[IEEE80211_ADDR_LEN];
	u_int32_t srev;

	ah_magic = AR5K_AR5212_MAGIC;
	
	/* Bring device out of sleep and reset it's units */
	if (ar5k_ar5212_nic_wakeup(AR5K_INIT_MODE) != AH_TRUE)
		return (AH_FALSE);

	/* Get MAC, PHY and RADIO revisions */
	srev = AR5K_REG_READ(AR5K_AR5212_SREV) & AR5K_AR5212_SREV_M;
	ah_mac_version = srev & AR5K_AR5212_SREV_VERSION;
	ah_mac_revision = srev & AR5K_AR5212_SREV_REVISION;
	ah_phy_revision = AR5K_REG_READ(AR5K_AR5212_PHY_CHIP_ID) &
	    0x00ffffffff;

	ah_radio_5ghz_revision =
	    ar5k_ar5212_radio_revision(HAL_CHIP_5GHZ);

	/* Get the 2GHz radio revision if it's supported */
	if (ah_mac_version >= AR5K_SREV_VER_AR5211)
		ah_radio_2ghz_revision =
		    ar5k_ar5212_radio_revision(HAL_CHIP_2GHZ);

	/* Identify the chipset (this has to be done in an early step) */
	ah_version = AR5K_AR5212;
	ah_radio = ah_radio_5ghz_revision < AR5K_SREV_RAD_5112 ?
	    AR5K_AR5111 : AR5K_AR5112;
	ah_phy = AR5K_AR5212_PHY(0);

	bcopy(etherbroadcastaddr, mac, IEEE80211_ADDR_LEN);
	nic_writeAssocid(mac, 0, 0);
	nic_getMacAddress(mac);
	nic_setPCUConfig();

	return AH_TRUE;
}

HAL_BOOL OpenHAL5212::ar5k_ar5212_nic_reset(u_int32_t val) {
	HAL_BOOL ret = AH_FALSE;
	u_int32_t mask = val ? val : 0xFFFFFFFF;

	/* Read-and-clear */
	AR5K_REG_READ(AR5K_AR5212_RXDP);

	/*
	 * Reset the device and wait until success
	 */
	AR5K_REG_WRITE(AR5K_AR5212_RC, val);

	/* Wait at least 128 PCI clocks */
	AR5K_DELAY(15);

	val &=
	    AR5K_AR5212_RC_PCU | AR5K_AR5212_RC_BB;

	mask &=
	    AR5K_AR5212_RC_PCU | AR5K_AR5212_RC_BB;

	ret = ar5k_register_timeout(AR5K_AR5212_RC, mask, val, AH_FALSE);

	/*
	 * Reset configuration register
	 */
	if ((val & AR5K_AR5212_RC_PCU) == 0)
		AR5K_REG_WRITE(AR5K_AR5212_CFG, AR5K_AR5212_INIT_CFG);

	return (ret);
}

HAL_BOOL OpenHAL5212::ar5k_ar5212_nic_wakeup(u_int16_t flags) {
	u_int32_t turbo, mode, clock;

	turbo = 0;
	mode = 0;
	clock = 0;

	/*
	 * Get channel mode flags
	 */

	if (ah_radio >= AR5K_AR5112) {
		mode = AR5K_AR5212_PHY_MODE_RAD_AR5112;
		clock = AR5K_AR5212_PHY_PLL_AR5112;
	} else {
		mode = AR5K_AR5212_PHY_MODE_RAD_AR5111;
		clock = AR5K_AR5212_PHY_PLL_AR5111;
	}

	if (flags & IEEE80211_CHAN_2GHZ) {
		mode |= AR5K_AR5212_PHY_MODE_FREQ_2GHZ;
		clock |= AR5K_AR5212_PHY_PLL_44MHZ;
	} else if (flags & IEEE80211_CHAN_5GHZ) {
		mode |= AR5K_AR5212_PHY_MODE_FREQ_5GHZ;
		clock |= AR5K_AR5212_PHY_PLL_40MHZ;
	} else {
		AR5K_PRINT("invalid radio frequency mode\n");
		return (AH_FALSE);
	}

	if (flags & IEEE80211_CHAN_CCK) {
		mode |= AR5K_AR5212_PHY_MODE_MOD_CCK;
	} else if (flags & IEEE80211_CHAN_OFDM) {
		mode |= AR5K_AR5212_PHY_MODE_MOD_OFDM;
	} else if (flags & IEEE80211_CHAN_DYN) {
		mode |= AR5K_AR5212_PHY_MODE_MOD_DYN;
	} else {
		AR5K_PRINT("invalid radio frequency mode\n");
		return (AH_FALSE);
	}

	if (flags & IEEE80211_CHAN_TURBO) {
		turbo = AR5K_AR5212_PHY_TURBO_MODE |
		    AR5K_AR5212_PHY_TURBO_SHORT;
	}

	/*
	 * Reset and wakeup the device
	 */

	/* ...reset chipset and PCI device */
	if (ar5k_ar5212_nic_reset(AR5K_AR5212_RC_CHIP | AR5K_AR5212_RC_PCI) == AH_FALSE) {
		AR5K_PRINT("failed to reset the AR5212 + PCI chipset\n");
		return (AH_FALSE);
	}

	/* ...wakeup */
	if (nic_setPowerMode(HAL_PM_AWAKE, AH_TRUE, 0) == AH_FALSE) {
		AR5K_PRINT("failed to resume the AR5212 (again)\n");
		return (AH_FALSE);
	}

	/* ...final warm reset */
	if (ar5k_ar5212_nic_reset(0) == AH_FALSE) {
		AR5K_PRINT("failed to warm reset the AR5212\n");
		return (AH_FALSE);
	}

	/* ...set the PHY operating mode */
	AR5K_REG_WRITE(AR5K_AR5212_PHY_PLL, clock);
	AR5K_DELAY(300);

	AR5K_REG_WRITE(AR5K_AR5212_PHY_MODE, mode);
	AR5K_REG_WRITE(AR5K_AR5212_PHY_TURBO, turbo);

	return (AH_TRUE);
}

u_int16_t OpenHAL5212::ar5k_ar5212_radio_revision(HAL_CHIP chip) {
	int i;
	u_int32_t srev;
	u_int16_t ret;

	/*
	 * Set the radio chip access register
	 */
	switch (chip) {
	case HAL_CHIP_2GHZ:
		AR5K_REG_WRITE(AR5K_AR5212_PHY(0), AR5K_AR5212_PHY_SHIFT_2GHZ);
		break;
	case HAL_CHIP_5GHZ:
		AR5K_REG_WRITE(AR5K_AR5212_PHY(0), AR5K_AR5212_PHY_SHIFT_5GHZ);
		break;
	default:
		return (0);
	}

	AR5K_DELAY(2000);

	/* ...wait until PHY is ready and read the selected radio revision */
	AR5K_REG_WRITE(AR5K_AR5212_PHY(0x34), 0x00001c16);

	for (i = 0; i < 8; i++)
		AR5K_REG_WRITE(AR5K_AR5212_PHY(0x20), 0x00010000);
	srev = (AR5K_REG_READ(AR5K_AR5212_PHY(0x100)) >> 24) & 0xff;

	ret = ar5k_bitswap(((srev & 0xf0) >> 4) | ((srev & 0x0f) << 4), 8);

	/* Reset to the 5GHz mode */
	AR5K_REG_WRITE(AR5K_AR5212_PHY(0), AR5K_AR5212_PHY_SHIFT_5GHZ);

	return (ret);
}

const HAL_RATE_TABLE * OpenHAL5212::nic_getRateTable(u_int mode) {
	switch (mode) {
	case HAL_MODE_11A:
		return (&ah_rt_11a);
	case HAL_MODE_TURBO:
		return (&ah_rt_turbo);
	case HAL_MODE_11B:
		return (&ah_rt_11b);
	case HAL_MODE_11G:
	case HAL_MODE_PUREG:
		return (&ah_rt_11g);
	case HAL_MODE_XR:
		return (&ah_rt_xr);
	default:
		return (NULL);
	}

	return (NULL);
}

void OpenHAL5212::nic_detach() {
	if (ah_rf_banks != NULL)
		IOFree(ah_rf_banks, sizeof(ar5112_rf_tofix));
}

HAL_BOOL OpenHAL5212::nic_reset(HAL_OPMODE op_mode, HAL_CHANNEL *channel, HAL_BOOL change_channel, HAL_STATUS *status) {
	struct ar5k_eeprom_info *ee = &ah_capabilities.cap_eeprom;
	u_int8_t mac[IEEE80211_ADDR_LEN];
	u_int32_t data, s_seq, s_ant, s_led[3];
	u_int i, phy, mode, freq, off, ee_mode, ant[2];
	const HAL_RATE_TABLE *rt;

	/*
	 * Save some registers before a reset
	 */
	if (change_channel == AH_TRUE) {
		s_seq = AR5K_REG_READ(AR5K_AR5212_DCU_SEQNUM(0));
		s_ant = AR5K_REG_READ(AR5K_AR5212_DEFAULT_ANTENNA);
	} else {
		s_seq = 0;
		s_ant = 1;
	}

	s_led[0] = AR5K_REG_READ(AR5K_AR5212_PCICFG) &
	    AR5K_AR5212_PCICFG_LEDSTATE;
	s_led[1] = AR5K_REG_READ(AR5K_AR5212_GPIOCR);
	s_led[2] = AR5K_REG_READ(AR5K_AR5212_GPIODO);

	if (change_channel == AH_TRUE && ah_rf_banks != NULL)
		nic_getRfGain();

	if (ar5k_ar5212_nic_wakeup(channel->c_channel_flags) == AH_FALSE)
		return (AH_FALSE);

	/*
	 * Initialize operating mode
	 */
	ah_op_mode = op_mode;

	if (ah_radio == AR5K_AR5111) {
		phy = AR5K_INI_PHY_5111;
	} else if (ah_radio == AR5K_AR5112) {
		phy = AR5K_INI_PHY_5112;
	} else {
		AR5K_PRINTF("invalid phy radio: %u\n", ah_radio);
		return (AH_FALSE);
	}

	if (channel->c_channel_flags & IEEE80211_CHAN_A) {
		mode = AR5K_INI_VAL_11A;
		freq = AR5K_INI_RFGAIN_5GHZ;
		ee_mode = AR5K_EEPROM_MODE_11A;
	} else if (channel->c_channel_flags & IEEE80211_CHAN_T) {
		mode = AR5K_INI_VAL_11A_TURBO;
		freq = AR5K_INI_RFGAIN_5GHZ;
		ee_mode = AR5K_EEPROM_MODE_11A;
	} else if (channel->c_channel_flags & IEEE80211_CHAN_B) {
		mode = AR5K_INI_VAL_11B;
		freq = AR5K_INI_RFGAIN_2GHZ;
		ee_mode = AR5K_EEPROM_MODE_11B;
	} else if (channel->c_channel_flags & IEEE80211_CHAN_G) {
		mode = AR5K_INI_VAL_11G;
		freq = AR5K_INI_RFGAIN_2GHZ;
		ee_mode = AR5K_EEPROM_MODE_11G;
	} else if (channel->c_channel_flags & CHANNEL_TG) {
		mode = AR5K_INI_VAL_11G_TURBO;
		freq = AR5K_INI_RFGAIN_2GHZ;
		ee_mode = AR5K_EEPROM_MODE_11G;
	} else if (channel->c_channel_flags & CHANNEL_XR) {
		mode = AR5K_INI_VAL_XR;
		freq = AR5K_INI_RFGAIN_5GHZ;
		ee_mode = AR5K_EEPROM_MODE_11A;
	} else {
		AR5K_PRINTF("invalid channel: %d\n", channel->c_channel);
		return (AH_FALSE);
	}

	/* PHY access enable */
	AR5K_REG_WRITE(AR5K_AR5212_PHY(0), AR5K_AR5212_PHY_SHIFT_5GHZ);

	/*
	 * Write initial mode settings
	 */
	for (i = 0; i < AR5K_ELEMENTS(ar5212_mode); i++) {
		if (ar5212_mode[i].mode_flags == AR5K_INI_FLAG_511X)
			off = AR5K_INI_PHY_511X;
		else if (ar5212_mode[i].mode_flags & AR5K_INI_FLAG_5111 &&
		    ah_radio == AR5K_AR5111)
			off = AR5K_INI_PHY_5111;
		else if (ar5212_mode[i].mode_flags & AR5K_INI_FLAG_5112 &&
		    ah_radio == AR5K_AR5112)
			off = AR5K_INI_PHY_5112;
		else
			continue;

		AR5K_REG_WAIT(i);
		AR5K_REG_WRITE((u_int32_t)ar5212_mode[i].mode_register,
		    ar5212_mode[i].mode_value[off][mode]);
	}

	/*
	 * Write initial register settings
	 */
	for (i = 0; i < AR5K_ELEMENTS(ar5212_ini); i++) {
		if (change_channel == AH_TRUE &&
		    ar5212_ini[i].ini_register >= AR5K_AR5212_PCU_MIN &&
		    ar5212_ini[i].ini_register <= AR5K_AR5212_PCU_MAX)
			continue;

		if ((ah_radio == AR5K_AR5111 &&
		    ar5212_ini[i].ini_flags & AR5K_INI_FLAG_5111) ||
		    (ah_radio == AR5K_AR5112 &&
		    ar5212_ini[i].ini_flags & AR5K_INI_FLAG_5112)) {
			AR5K_REG_WAIT(i);
			AR5K_REG_WRITE((u_int32_t)ar5212_ini[i].ini_register,
			    ar5212_ini[i].ini_value);
		}
	}

	/*
	 * Write initial RF gain settings
	 */
	if (ar5k_rfgain(phy, freq) == AH_FALSE)
		return (AH_FALSE);

	AR5K_DELAY(1000);

	/*
	 * Set rate duration table
	 */
	rt = nic_getRateTable(
	    channel->c_channel_flags & IEEE80211_CHAN_TURBO ?
	    HAL_MODE_TURBO : HAL_MODE_XR);

	for (i = 0; i < rt->rt_rate_count; i++) {
		AR5K_REG_WRITE(AR5K_AR5212_RATE_DUR(rt->rt_info[i].r_rate_code),
		    ath_hal_computetxtime(rt, 14,
		    rt->rt_info[i].r_control_rate, AH_FALSE));
	}

	if (!(channel->c_channel_flags & IEEE80211_CHAN_TURBO)) {
		rt = nic_getRateTable(HAL_MODE_11B);
		for (i = 0; i < rt->rt_rate_count; i++) {
			data = AR5K_AR5212_RATE_DUR(rt->rt_info[i].r_rate_code);
			AR5K_REG_WRITE(data,
			    ath_hal_computetxtime(rt, 14,
			    rt->rt_info[i].r_control_rate, AH_FALSE));
			if (rt->rt_info[i].r_short_preamble) {
				AR5K_REG_WRITE(data +
				    (rt->rt_info[i].r_short_preamble << 2),
				    ath_hal_computetxtime(rt, 14,
				    rt->rt_info[i].r_control_rate, AH_FALSE));
			}
		}
	}

	/*
	 * Set TX power (XXX use txpower from net80211)
	 */
	if (ar5k_ar5212_txpower(channel,
		AR5K_TUNE_DEFAULT_TXPOWER) == AH_FALSE)
		return (AH_FALSE);

	/*
	 * Write RF registers
	 */
	if (ar5k_rfregs(channel, mode) == AH_FALSE)
		return (AH_FALSE);

	/*
	 * Configure additional registers
	 */

	/* OFDM timings */
	if (channel->c_channel_flags & IEEE80211_CHAN_OFDM) {
		u_int32_t coef_scaled, coef_exp, coef_man, ds_coef_exp,
		    ds_coef_man, clock;

		clock = channel->c_channel_flags & IEEE80211_CHAN_T ? 80 : 40;
		coef_scaled = ((5 * (clock << 24)) / 2) / channel->c_channel;

		for (coef_exp = 31; coef_exp > 0; coef_exp--)
			if ((coef_scaled >> coef_exp) & 0x1)
				break;

		if (!coef_exp)
			return (AH_FALSE);

		coef_exp = 14 - (coef_exp - 24);
		coef_man = coef_scaled + (1 << (24 - coef_exp - 1));
		ds_coef_man = coef_man >> (24 - coef_exp);
		ds_coef_exp = coef_exp - 16;

		AR5K_REG_WRITE_BITS(AR5K_AR5212_PHY_TIMING_3,
		    AR5K_AR5212_PHY_TIMING_3_DSC_MAN, ds_coef_man);
		AR5K_REG_WRITE_BITS(AR5K_AR5212_PHY_TIMING_3,
		    AR5K_AR5212_PHY_TIMING_3_DSC_EXP, ds_coef_exp);
	}

	/* Set antenna mode */
	AR5K_REG_MASKED_BITS(AR5K_AR5212_PHY(0x44),
	    ah_antenna[ee_mode][0], 0xfffffc06);

	ant[0] = HAL_ANT_FIXED_A;
	ant[1] = HAL_ANT_FIXED_B;

	if (ah_ant_diversity == AH_FALSE) {
		if (freq == AR5K_INI_RFGAIN_2GHZ)
			ant[0] = HAL_ANT_FIXED_B;
		else	
			ant[1] = HAL_ANT_FIXED_A;
	}

	AR5K_REG_WRITE(AR5K_AR5212_PHY_ANT_SWITCH_TABLE_0,
	    ah_antenna[ee_mode][ant[0]]);
	AR5K_REG_WRITE(AR5K_AR5212_PHY_ANT_SWITCH_TABLE_1,
	    ah_antenna[ee_mode][ant[1]]);

	/* Commit values from EEPROM */
	if (ah_radio == AR5K_AR5111)
		AR5K_REG_WRITE_BITS(AR5K_AR5212_PHY_FC,
		    AR5K_AR5212_PHY_FC_TX_CLIP, ee->ee_tx_clip);

	AR5K_REG_WRITE(AR5K_AR5212_PHY(0x5a),
	    AR5K_AR5212_PHY_NF_SVAL(ee->ee_noise_floor_thr[ee_mode]));

	AR5K_REG_MASKED_BITS(AR5K_AR5212_PHY(0x11),
	    (ee->ee_switch_settling[ee_mode] << 7) & 0x3f80, 0xffffc07f);
	AR5K_REG_MASKED_BITS(AR5K_AR5212_PHY(0x12),
	    (ee->ee_ant_tx_rx[ee_mode] << 12) & 0x3f000, 0xfffc0fff);
	AR5K_REG_MASKED_BITS(AR5K_AR5212_PHY(0x14),
	    (ee->ee_adc_desired_size[ee_mode] & 0x00ff) |
	    ((ee->ee_pga_desired_size[ee_mode] << 8) & 0xff00), 0xffff0000);

	AR5K_REG_WRITE(AR5K_AR5212_PHY(0x0d),
	    (ee->ee_tx_end2xpa_disable[ee_mode] << 24) |
	    (ee->ee_tx_end2xpa_disable[ee_mode] << 16) |
	    (ee->ee_tx_frm2xpa_enable[ee_mode] << 8) |
	    (ee->ee_tx_frm2xpa_enable[ee_mode]));

	AR5K_REG_MASKED_BITS(AR5K_AR5212_PHY(0x0a),
	    ee->ee_tx_end2xlna_enable[ee_mode] << 8, 0xffff00ff);
	AR5K_REG_MASKED_BITS(AR5K_AR5212_PHY(0x19),
	    (ee->ee_thr_62[ee_mode] << 12) & 0x7f000, 0xfff80fff);
	AR5K_REG_MASKED_BITS(AR5K_AR5212_PHY(0x49), 4, 0xffffff01);

	AR5K_REG_ENABLE_BITS(AR5K_AR5212_PHY_IQ,
	    AR5K_AR5212_PHY_IQ_CORR_ENABLE |
	    (ee->ee_i_cal[ee_mode] << AR5K_AR5212_PHY_IQ_CORR_Q_I_COFF_S) |
	    ee->ee_q_cal[ee_mode]);

	if (ah_ee_version >= AR5K_EEPROM_VERSION_4_1) {
		AR5K_REG_WRITE_BITS(AR5K_AR5212_PHY_GAIN_2GHZ,
		    AR5K_AR5212_PHY_GAIN_2GHZ_MARGIN_TXRX,
		    ee->ee_margin_tx_rx[ee_mode]);
	}

	/*
	 * Restore saved values
	 */
	AR5K_REG_WRITE(AR5K_AR5212_DCU_SEQNUM(0), s_seq);
	AR5K_REG_WRITE(AR5K_AR5212_DEFAULT_ANTENNA, s_ant);
	AR5K_REG_ENABLE_BITS(AR5K_AR5212_PCICFG, s_led[0]);
	AR5K_REG_WRITE(AR5K_AR5212_GPIOCR, s_led[1]);
	AR5K_REG_WRITE(AR5K_AR5212_GPIODO, s_led[2]);

	/*
	 * Misc
	 */
	bcopy(etherbroadcastaddr, mac, IEEE80211_ADDR_LEN);
	nic_writeAssocid(mac, 0, 0);
	nic_setPCUConfig();
	AR5K_REG_WRITE(AR5K_AR5212_PISR, 0xffffffff);
	AR5K_REG_WRITE(AR5K_AR5212_RSSI_THR, AR5K_TUNE_RSSI_THRES);

	/*
	 * Set channel and calibrate the PHY
	 */
	if (ar5k_channel(channel) == AH_FALSE)
		return (AH_FALSE);

	/*
	 * Enable the PHY and wait until completion
	 */
	AR5K_REG_WRITE(AR5K_AR5212_PHY_ACTIVE, AR5K_AR5212_PHY_ENABLE);

	data = AR5K_REG_READ(AR5K_AR5212_PHY_RX_DELAY) &
	    AR5K_AR5212_PHY_RX_DELAY_M;
	data = (channel->c_channel_flags & IEEE80211_CHAN_CCK) ?
	    ((data << 2) / 22) : (data / 10);

	AR5K_DELAY(100 + data);

	/*
	 * Start calibration
	 */
	AR5K_REG_ENABLE_BITS(AR5K_AR5212_PHY_AGCCTL,
	    AR5K_AR5212_PHY_AGCCTL_NF |
	    AR5K_AR5212_PHY_AGCCTL_CAL);

	if (channel->c_channel_flags & IEEE80211_CHAN_B) {
		ah_calibration = AH_FALSE;
	} else {
		ah_calibration = AH_TRUE;
		AR5K_REG_WRITE_BITS(AR5K_AR5212_PHY_IQ,
		    AR5K_AR5212_PHY_IQ_CAL_NUM_LOG_MAX, 15);
		AR5K_REG_ENABLE_BITS(AR5K_AR5212_PHY_IQ,
		    AR5K_AR5212_PHY_IQ_RUN);
	}

	/*
	 * Reset queues and start beacon timers at the end of the reset routine
	 */
	for (i = 0; i < ah_capabilities.cap_queues.q_tx_num; i++) {
		AR5K_REG_WRITE_Q(AR5K_AR5212_DCU_QCUMASK(i), i);
		if (nic_resetTxQueue(i) == AH_FALSE) {
			AR5K_PRINTF("failed to reset TX queue #%d\n", i);
			return (AH_FALSE);
		}
	}

	/* Pre-enable interrupts */
	nic_setInterrupts(HAL_INT_RX | HAL_INT_TX | HAL_INT_FATAL);

	/*
	 * Set RF kill flags if supported by the device (read from the EEPROM)
	 */
	if (AR5K_EEPROM_HDR_RFKILL(ah_capabilities.cap_eeprom.ee_header)) {
		WLLogDebug("RF Kill supported!");
		nic_gpioCfgInput(0);
		if ((ah_gpio[0] = nic_gpioGet(0)) == 0)
			nic_gpioSetIntr(0, 1);
		else
			nic_gpioSetIntr(0, 0);
	}

	/*
	 * Set the 32MHz reference clock
	 */
	AR5K_REG_WRITE(AR5K_AR5212_PHY_SCR, AR5K_AR5212_PHY_SCR_32MHZ);
	AR5K_REG_WRITE(AR5K_AR5212_PHY_SLMT, AR5K_AR5212_PHY_SLMT_32MHZ);
	AR5K_REG_WRITE(AR5K_AR5212_PHY_SCAL, AR5K_AR5212_PHY_SCAL_32MHZ);
	AR5K_REG_WRITE(AR5K_AR5212_PHY_SCLOCK, AR5K_AR5212_PHY_SCLOCK_32MHZ);
	AR5K_REG_WRITE(AR5K_AR5212_PHY_SDELAY, AR5K_AR5212_PHY_SDELAY_32MHZ);
	AR5K_REG_WRITE(AR5K_AR5212_PHY_SPENDING, ah_radio == AR5K_AR5111 ?
	    AR5K_AR5212_PHY_SPENDING_AR5111 : AR5K_AR5212_PHY_SPENDING_AR5112);

	/* 
	 * Disable beacons and reset the register
	 */
	AR5K_REG_DISABLE_BITS(AR5K_AR5212_BEACON,
	    AR5K_AR5212_BEACON_ENABLE | AR5K_AR5212_BEACON_RESET_TSF);

	return (AH_TRUE);
}

void OpenHAL5212::nic_setPCUConfig() {
	u_int32_t pcu_reg, low_id, high_id;

	pcu_reg = 0;

	switch (ah_op_mode) {
	case IEEE80211_M_IBSS:
		pcu_reg |= AR5K_AR5212_STA_ID1_ADHOC |
		    AR5K_AR5212_STA_ID1_DESC_ANTENNA;
		break;

	case IEEE80211_M_HOSTAP:
		pcu_reg |= AR5K_AR5212_STA_ID1_AP |
		    AR5K_AR5212_STA_ID1_RTS_DEFAULT_ANTENNA;
		break;

	case IEEE80211_M_STA:
	case IEEE80211_M_MONITOR:
		pcu_reg |= AR5K_AR5212_STA_ID1_DEFAULT_ANTENNA;
		break;

	default:
		return;
	}

	/*
	 * Set PCU registers
	 */
		bcopy(&(ah_sta_id[0]), &low_id, 4);
        bcopy(&(ah_sta_id[4]), &high_id, 2);
        AR5K_REG_WRITE(AR5K_AR5212_STA_ID0, low_id);
        AR5K_REG_WRITE(AR5K_AR5212_STA_ID1, pcu_reg | high_id);
		
	//low_id = OSSwapHostToLittleInt32(*((UInt32*)  &ah_sta_id[0]));
	//high_id = OSSwapHostToLittleInt16(*((UInt16*) &ah_sta_id[4]));
	
	//AR5K_REG_WRITE(AR5K_AR5212_STA_ID0, low_id);
	//AR5K_REG_WRITE(AR5K_AR5212_STA_ID1, pcu_reg | high_id);

	return;
}


HAL_BOOL OpenHAL5212::nic_perCalibration(HAL_CHANNEL *channel) {
	u_int32_t i_pwr, q_pwr;
	int32_t iq_corr, i_coff, i_coffd, q_coff, q_coffd;

	if (ah_calibration == AH_FALSE ||
	    AR5K_REG_READ(AR5K_AR5212_PHY_IQ) & AR5K_AR5212_PHY_IQ_RUN)
		goto done;

	ah_calibration = AH_FALSE;

	iq_corr = AR5K_REG_READ(AR5K_AR5212_PHY_IQRES_CAL_CORR);
	i_pwr = AR5K_REG_READ(AR5K_AR5212_PHY_IQRES_CAL_PWR_I);
	q_pwr = AR5K_REG_READ(AR5K_AR5212_PHY_IQRES_CAL_PWR_Q);
	i_coffd = ((i_pwr >> 1) + (q_pwr >> 1)) >> 7;
	q_coffd = q_pwr >> 6;

	if (i_coffd == 0 || q_coffd == 0)
		goto done;

	i_coff = ((-iq_corr) / i_coffd) & 0x3f;
	q_coff = (((int32_t)i_pwr / q_coffd) - 64) & 0x1f;

	/* Commit new IQ value */
	AR5K_REG_ENABLE_BITS(AR5K_AR5212_PHY_IQ,
	    AR5K_AR5212_PHY_IQ_CORR_ENABLE |
	    ((u_int32_t)q_coff) |
	    ((u_int32_t)i_coff << AR5K_AR5212_PHY_IQ_CORR_Q_I_COFF_S));

 done:
	/* Start noise floor calibration */
	AR5K_REG_ENABLE_BITS(AR5K_AR5212_PHY_AGCCTL,
	    AR5K_AR5212_PHY_AGCCTL_NF);

	/* Request RF gain */
	if (channel->c_channel_flags & IEEE80211_CHAN_5GHZ) {
		AR5K_REG_WRITE(AR5K_AR5212_PHY_PAPD_PROBE,
		    AR5K_REG_SM(ah_txpower.txp_max,
		    AR5K_AR5212_PHY_PAPD_PROBE_TXPOWER) |
		    AR5K_AR5212_PHY_PAPD_PROBE_TX_NEXT);
		ah_rf_gain = HAL_RFGAIN_READ_REQUESTED;
	}

	return (AH_TRUE);
}

#pragma mark -

HAL_BOOL OpenHAL5212::nic_resetTxQueue(u_int queue) {
	u_int32_t cw_min, cw_max, retry_lg, retry_sh;
	struct ieee80211_channel *channel = (struct ieee80211_channel*)
	    &ah_current_channel;
	HAL_TXQ_INFO *tq;

	AR5K_ASSERT_ENTRY(queue, ah_capabilities.cap_queues.q_tx_num);

	tq = &ah_txq[queue];

	if (tq->tqi_type == HAL_TX_QUEUE_INACTIVE)
		return (AH_TRUE);

	/*
	 * Set registers by channel mode
	 */
	if (IEEE80211_IS_CHAN_XR(channel)) {
		ah_cw_min = AR5K_TUNE_CWMIN_XR;
		cw_max = ah_cw_max = AR5K_TUNE_CWMAX_XR;
		ah_aifs = AR5K_TUNE_AIFS_XR;
	} else if (IEEE80211_IS_CHAN_B(channel)) {
		ah_cw_min = AR5K_TUNE_CWMIN_11B;
		cw_max = ah_cw_max = AR5K_TUNE_CWMAX_11B;
		ah_aifs = AR5K_TUNE_AIFS_11B;
	} else {
		ah_cw_min = AR5K_TUNE_CWMIN;
		ah_cw_max = AR5K_TUNE_CWMAX;
		ah_aifs = AR5K_TUNE_AIFS;
	}

	/*
	 * Set retry limits
	 */
	if (ah_software_retry == AH_TRUE) {
		/* XXX Need to test this */
		retry_lg = ah_limit_tx_retries;
		retry_sh = retry_lg =
		    retry_lg > AR5K_AR5212_DCU_RETRY_LMT_SH_RETRY ?
		    AR5K_AR5212_DCU_RETRY_LMT_SH_RETRY : retry_lg;
	} else {
		retry_lg = AR5K_INIT_LG_RETRY;
		retry_sh = AR5K_INIT_SH_RETRY;
	}

	AR5K_REG_WRITE(AR5K_AR5212_DCU_RETRY_LMT(queue),
	    AR5K_REG_SM(AR5K_INIT_SLG_RETRY,
	    AR5K_AR5212_DCU_RETRY_LMT_SLG_RETRY) |
	    AR5K_REG_SM(AR5K_INIT_SSH_RETRY,
	    AR5K_AR5212_DCU_RETRY_LMT_SSH_RETRY) |
	    AR5K_REG_SM(retry_lg, AR5K_AR5212_DCU_RETRY_LMT_LG_RETRY) |
	    AR5K_REG_SM(retry_sh, AR5K_AR5212_DCU_RETRY_LMT_SH_RETRY));

	/*
	 * Set initial content window (cw_min/cw_max)
	 */
	cw_min = 1;
	while (cw_min < ah_cw_min)
		cw_min = (cw_min << 1) | 1;

	cw_min = tq->tqi_cw_min < 0 ?
	    (cw_min >> (-tq->tqi_cw_min)) :
	    ((cw_min << tq->tqi_cw_min) + (1 << tq->tqi_cw_min) - 1);
	cw_max = tq->tqi_cw_max < 0 ?
	    (cw_max >> (-tq->tqi_cw_max)) :
	    ((cw_max << tq->tqi_cw_max) + (1 << tq->tqi_cw_max) - 1);

	AR5K_REG_WRITE(AR5K_AR5212_DCU_LCL_IFS(queue),
	    AR5K_REG_SM(cw_min, AR5K_AR5212_DCU_LCL_IFS_CW_MIN) |
	    AR5K_REG_SM(cw_max, AR5K_AR5212_DCU_LCL_IFS_CW_MAX) |
	    AR5K_REG_SM(ah_aifs + tq->tqi_aifs,
	    AR5K_AR5212_DCU_LCL_IFS_AIFS));

	/*
	 * Set misc registers
	 */
	AR5K_REG_WRITE(AR5K_AR5212_QCU_MISC(queue),
	    AR5K_AR5212_QCU_MISC_DCU_EARLY);

	if (tq->tqi_cbr_period) {
		AR5K_REG_WRITE(AR5K_AR5212_QCU_CBRCFG(queue),
		    AR5K_REG_SM(tq->tqi_cbr_period,
		    AR5K_AR5212_QCU_CBRCFG_INTVAL) |
		    AR5K_REG_SM(tq->tqi_cbr_overflow_limit,
		    AR5K_AR5212_QCU_CBRCFG_ORN_THRES));
		AR5K_REG_ENABLE_BITS(AR5K_AR5212_QCU_MISC(queue),
		    AR5K_AR5212_QCU_MISC_FRSHED_CBR);
		if (tq->tqi_cbr_overflow_limit)
			AR5K_REG_ENABLE_BITS(AR5K_AR5212_QCU_MISC(queue),
			    AR5K_AR5212_QCU_MISC_CBR_THRES_ENABLE);
	}

	if (tq->tqi_ready_time) {
		AR5K_REG_WRITE(AR5K_AR5212_QCU_RDYTIMECFG(queue),
		    AR5K_REG_SM(tq->tqi_ready_time,
		    AR5K_AR5212_QCU_RDYTIMECFG_INTVAL) |
		    AR5K_AR5212_QCU_RDYTIMECFG_ENABLE);
	}

	if (tq->tqi_burst_time) {
		AR5K_REG_WRITE(AR5K_AR5212_DCU_CHAN_TIME(queue),
		    AR5K_REG_SM(tq->tqi_burst_time,
		    AR5K_AR5212_DCU_CHAN_TIME_DUR) |
		    AR5K_AR5212_DCU_CHAN_TIME_ENABLE);

		if (tq->tqi_flags & AR5K_TXQ_FLAG_RDYTIME_EXP_POLICY_ENABLE) {
			AR5K_REG_ENABLE_BITS(AR5K_AR5212_QCU_MISC(queue),
			    AR5K_AR5212_QCU_MISC_TXE);
		}
	}

	if (tq->tqi_flags & AR5K_TXQ_FLAG_BACKOFF_DISABLE) {
		AR5K_REG_WRITE(AR5K_AR5212_DCU_MISC(queue),
		    AR5K_AR5212_DCU_MISC_POST_FR_BKOFF_DIS);
	}

	if (tq->tqi_flags & AR5K_TXQ_FLAG_FRAG_BURST_BACKOFF_ENABLE) {
		AR5K_REG_WRITE(AR5K_AR5212_DCU_MISC(queue),
		    AR5K_AR5212_DCU_MISC_BACKOFF_FRAG);
	}

	/*
	 * Set registers by queue type
	 */
	switch (tq->tqi_type) {
	case HAL_TX_QUEUE_BEACON:
		AR5K_REG_ENABLE_BITS(AR5K_AR5212_QCU_MISC(queue),
		    AR5K_AR5212_QCU_MISC_FRSHED_DBA_GT |
		    AR5K_AR5212_QCU_MISC_CBREXP_BCN |
		    AR5K_AR5212_QCU_MISC_BCN_ENABLE);

		AR5K_REG_ENABLE_BITS(AR5K_AR5212_DCU_MISC(queue),
		    (AR5K_AR5212_DCU_MISC_ARBLOCK_CTL_GLOBAL <<
		    AR5K_AR5212_DCU_MISC_ARBLOCK_CTL_GLOBAL) |
		    AR5K_AR5212_DCU_MISC_POST_FR_BKOFF_DIS |
		    AR5K_AR5212_DCU_MISC_BCN_ENABLE);

		AR5K_REG_WRITE(AR5K_AR5212_QCU_RDYTIMECFG(queue),
		    ((AR5K_TUNE_BEACON_INTERVAL -
		    (AR5K_TUNE_SW_BEACON_RESP - AR5K_TUNE_DMA_BEACON_RESP) -
		    AR5K_TUNE_ADDITIONAL_SWBA_BACKOFF) * 1024) |
		    AR5K_AR5212_QCU_RDYTIMECFG_ENABLE);
		break;

	case HAL_TX_QUEUE_CAB:
		AR5K_REG_ENABLE_BITS(AR5K_AR5212_QCU_MISC(queue),
		    AR5K_AR5212_QCU_MISC_FRSHED_DBA_GT |
		    AR5K_AR5212_QCU_MISC_CBREXP |
		    AR5K_AR5212_QCU_MISC_CBREXP_BCN);

		AR5K_REG_ENABLE_BITS(AR5K_AR5212_DCU_MISC(queue),
		    (AR5K_AR5212_DCU_MISC_ARBLOCK_CTL_GLOBAL <<
		    AR5K_AR5212_DCU_MISC_ARBLOCK_CTL_GLOBAL));
		break;

	case HAL_TX_QUEUE_PSPOLL:
		AR5K_REG_ENABLE_BITS(AR5K_AR5212_QCU_MISC(queue),
		    AR5K_AR5212_QCU_MISC_CBREXP);
		break;

	case HAL_TX_QUEUE_DATA:
	default:
		break;
	}

	/*
	 * Enable tx queue in the secondary interrupt mask registers
	 */
	AR5K_REG_WRITE(AR5K_AR5212_SIMR0,
	    AR5K_REG_SM(ah_txq_interrupts, AR5K_AR5212_SIMR0_QCU_TXOK) |
	    AR5K_REG_SM(ah_txq_interrupts, AR5K_AR5212_SIMR0_QCU_TXDESC));
	AR5K_REG_WRITE(AR5K_AR5212_SIMR1,
	    AR5K_REG_SM(ah_txq_interrupts, AR5K_AR5212_SIMR1_QCU_TXERR));
	AR5K_REG_WRITE(AR5K_AR5212_SIMR2,
	    AR5K_REG_SM(ah_txq_interrupts, AR5K_AR5212_SIMR2_QCU_TXURN));

	return (AH_TRUE);
}

#pragma mark -

u_int32_t OpenHAL5212::nic_getRxDP() {
	return (AR5K_REG_READ(AR5K_AR5212_RXDP));
}

void OpenHAL5212::nic_setRxDP(u_int32_t phys_addr) {
	AR5K_REG_WRITE(AR5K_AR5212_RXDP, phys_addr);
}

void OpenHAL5212::nic_enableReceive() {
	AR5K_REG_WRITE(AR5K_AR5212_CR, AR5K_AR5212_CR_RXE);
}

HAL_BOOL OpenHAL5212::nic_stopDmaReceive() {
	int i;

	AR5K_REG_WRITE(AR5K_AR5212_CR, AR5K_AR5212_CR_RXD);

	/*
	 * It may take some time to disable the DMA receive unit
	 */
	for (i = 2000;
	     i > 0 && (AR5K_REG_READ(AR5K_AR5212_CR) & AR5K_AR5212_CR_RXE) != 0;
	     i--)
		AR5K_DELAY(10);

	return (i > 0 ? AH_TRUE : AH_FALSE);
}

void OpenHAL5212::nic_startPcuReceive() {
	AR5K_REG_DISABLE_BITS(AR5K_AR5212_DIAG_SW, AR5K_AR5212_DIAG_SW_DIS_RX);
}

void OpenHAL5212::nic_stopPcuReceive() {
	AR5K_REG_ENABLE_BITS(AR5K_AR5212_DIAG_SW, AR5K_AR5212_DIAG_SW_DIS_RX);
}

u_int32_t OpenHAL5212::nic_getRxFilter() {
	u_int32_t data, filter = 0;

	filter = AR5K_REG_READ(AR5K_AR5212_RX_FILTER);
	data = AR5K_REG_READ(AR5K_AR5212_PHY_ERR_FIL);

	if (data & AR5K_AR5212_PHY_ERR_FIL_RADAR)
		filter |= HAL_RX_FILTER_PHYRADAR;
	if (data & (AR5K_AR5212_PHY_ERR_FIL_OFDM |
	    AR5K_AR5212_PHY_ERR_FIL_CCK))
		filter |= HAL_RX_FILTER_PHYERR;

	return (filter);
}

void OpenHAL5212::nic_setRxFilter(u_int32_t filter) {
	u_int32_t data = 0;

	if (filter & HAL_RX_FILTER_PHYRADAR)
		data |= AR5K_AR5212_PHY_ERR_FIL_RADAR;
	if (filter & HAL_RX_FILTER_PHYERR)
		data |= AR5K_AR5212_PHY_ERR_FIL_OFDM |
		    AR5K_AR5212_PHY_ERR_FIL_CCK;

	if (data) {
		AR5K_REG_ENABLE_BITS(AR5K_AR5212_RXCFG,
		    AR5K_AR5212_RXCFG_ZLFDMA);
	} else {
		AR5K_REG_DISABLE_BITS(AR5K_AR5212_RXCFG,
		    AR5K_AR5212_RXCFG_ZLFDMA);
	}

	AR5K_REG_WRITE(AR5K_AR5212_RX_FILTER, filter & 0xff);
	AR5K_REG_WRITE(AR5K_AR5212_PHY_ERR_FIL, data);
}

HAL_BOOL OpenHAL5212::nic_setupRxDesc(struct ath_desc *desc, u_int32_t size, u_int flags) {
	struct ar5k_ar5212_rx_desc *rx_desc;

	/* Reset descriptor */
	desc->ds_ctl0 = 0;
	desc->ds_ctl1 = 0;
	bzero((void*)&desc->ds_hw[0], sizeof(struct ar5k_ar5212_rx_status));

	rx_desc = (struct ar5k_ar5212_rx_desc*)&desc->ds_ctl0;

	if ((rx_desc->rx_control_1 = (size &
	    AR5K_AR5212_DESC_RX_CTL1_BUF_LEN)) != (size))
		return (AH_FALSE);

	if (flags & HAL_RXDESC_INTREQ)
		rx_desc->rx_control_1 |= (AR5K_AR5212_DESC_RX_CTL1_INTREQ);

	return (AH_TRUE);
}

HAL_STATUS OpenHAL5212::nic_procRxDesc(struct ath_desc *desc, u_int32_t phys_addr, struct ath_desc *next) {
	struct ar5k_ar5212_rx_status *rx_status;
	struct ar5k_ar5212_rx_error *rx_err;
	
	rx_status = (struct ar5k_ar5212_rx_status*)&desc->ds_hw[0];

	/* Overlay on error */
	rx_err = (struct ar5k_ar5212_rx_error*)&desc->ds_hw[0];

	/* No frame received / not ready */
	if ((rx_status->rx_status_1 & AR5K_AR5212_DESC_RX_STATUS1_DONE) == 0)
		return (HAL_EINPROGRESS);

	/*
	 * Frame receive status
	 */
	desc->ds_us.rx.rs_datalen = rx_status->rx_status_0 &
	    AR5K_AR5212_DESC_RX_STATUS0_DATA_LEN;
	desc->ds_us.rx.rs_rssi =
	    AR5K_REG_MS(rx_status->rx_status_0,
	    AR5K_AR5212_DESC_RX_STATUS0_RECEIVE_SIGNAL);
	desc->ds_us.rx.rs_rate =
	    AR5K_REG_MS(rx_status->rx_status_0,
	    AR5K_AR5212_DESC_RX_STATUS0_RECEIVE_RATE);
	desc->ds_us.rx.rs_antenna = rx_status->rx_status_0 &
	    AR5K_AR5212_DESC_RX_STATUS0_RECEIVE_ANTENNA;
	desc->ds_us.rx.rs_more = rx_status->rx_status_0 &
	    AR5K_AR5212_DESC_RX_STATUS0_MORE;
	desc->ds_us.rx.rs_tstamp =
	    AR5K_REG_MS(rx_status->rx_status_1,
	    AR5K_AR5212_DESC_RX_STATUS1_RECEIVE_TIMESTAMP);
	desc->ds_us.rx.rs_status = 0;

	/*
	 * Key table status
	 */
	if (rx_status->rx_status_1 &
	    AR5K_AR5212_DESC_RX_STATUS1_KEY_INDEX_VALID) {
		desc->ds_us.rx.rs_keyix =
		    AR5K_REG_MS(rx_status->rx_status_1,
		    AR5K_AR5212_DESC_RX_STATUS1_KEY_INDEX);
	} else {
		desc->ds_us.rx.rs_keyix = HAL_RXKEYIX_INVALID;
	}

	/*
	 * Receive/descriptor errors
	 */
	if ((rx_status->rx_status_1 &
	    AR5K_AR5212_DESC_RX_STATUS1_FRAME_RECEIVE_OK) == 0) {
		if (rx_status->rx_status_1 &
		    AR5K_AR5212_DESC_RX_STATUS1_CRC_ERROR)
			desc->ds_us.rx.rs_status |= HAL_RXERR_CRC;

		if (rx_status->rx_status_1 &
		    AR5K_AR5212_DESC_RX_STATUS1_PHY_ERROR) {
			desc->ds_us.rx.rs_status |= HAL_RXERR_PHY;
			desc->ds_us.rx.rs_phyerr =
			    AR5K_REG_MS(rx_err->rx_error_1,
			    AR5K_AR5212_DESC_RX_ERROR1_PHY_ERROR_CODE);
		}

		if (rx_status->rx_status_1 &
		    AR5K_AR5212_DESC_RX_STATUS1_DECRYPT_CRC_ERROR)
			desc->ds_us.rx.rs_status |= HAL_RXERR_DECRYPT;

		if (rx_status->rx_status_1 &
		    AR5K_AR5212_DESC_RX_STATUS1_MIC_ERROR)
			desc->ds_us.rx.rs_status |= HAL_RXERR_MIC;
	}

	return (HAL_OK);
}

void OpenHAL5212::nic_rxMonitor() {
	AR5K_REG_ENABLE_BITS(AR5K_AR5212_RX_FILTER,
	    AR5K_AR5212_RX_FILTER_PROMISC);
}

#pragma mark -

void OpenHAL5212::dumpState() {
#define AR5K_PRINT_REGISTER(_x)						\
	IOLog("(%s: %08x) ", #_x, AR5K_REG_READ(AR5K_AR5212_##_x));\
	IOSleep(100);

	IOLog("MAC registers:\n");
	AR5K_PRINT_REGISTER(CR);
	AR5K_PRINT_REGISTER(CFG);
	AR5K_PRINT_REGISTER(IER);
	AR5K_PRINT_REGISTER(TXCFG);
	AR5K_PRINT_REGISTER(RXCFG);
	AR5K_PRINT_REGISTER(MIBC);
	AR5K_PRINT_REGISTER(TOPS);
	AR5K_PRINT_REGISTER(RXNOFRM);
	AR5K_PRINT_REGISTER(RPGTO);
	AR5K_PRINT_REGISTER(RFCNT);
	AR5K_PRINT_REGISTER(MISC);
	AR5K_PRINT_REGISTER(PISR);
	AR5K_PRINT_REGISTER(SISR0);
	AR5K_PRINT_REGISTER(SISR1);
	AR5K_PRINT_REGISTER(SISR3);
	AR5K_PRINT_REGISTER(SISR4);
	AR5K_PRINT_REGISTER(DCM_ADDR);
	AR5K_PRINT_REGISTER(DCM_DATA);
	AR5K_PRINT_REGISTER(DCCFG);
	AR5K_PRINT_REGISTER(CCFG);
	AR5K_PRINT_REGISTER(CCFG_CUP);
	AR5K_PRINT_REGISTER(CPC0);
	AR5K_PRINT_REGISTER(CPC1);
	AR5K_PRINT_REGISTER(CPC2);
	AR5K_PRINT_REGISTER(CPCORN);
	AR5K_PRINT_REGISTER(QCU_TXE);
	AR5K_PRINT_REGISTER(QCU_TXD);
	AR5K_PRINT_REGISTER(DCU_GBL_IFS_SIFS);
	AR5K_PRINT_REGISTER(DCU_GBL_IFS_SLOT);
	AR5K_PRINT_REGISTER(DCU_FP);
	AR5K_PRINT_REGISTER(DCU_TXP);
	AR5K_PRINT_REGISTER(DCU_TX_FILTER);
	AR5K_PRINT_REGISTER(RC);
	AR5K_PRINT_REGISTER(SCR);
	AR5K_PRINT_REGISTER(INTPEND);
	AR5K_PRINT_REGISTER(PCICFG);
	AR5K_PRINT_REGISTER(GPIOCR);
	AR5K_PRINT_REGISTER(GPIODO);
	AR5K_PRINT_REGISTER(SREV);
	AR5K_PRINT_REGISTER(EEPROM_BASE);
	AR5K_PRINT_REGISTER(EEPROM_DATA);
	AR5K_PRINT_REGISTER(EEPROM_CMD);
	AR5K_PRINT_REGISTER(EEPROM_CFG);
	AR5K_PRINT_REGISTER(PCU_MIN);
	AR5K_PRINT_REGISTER(STA_ID0);
	AR5K_PRINT_REGISTER(STA_ID1);
	AR5K_PRINT_REGISTER(BSS_ID0);
	AR5K_PRINT_REGISTER(SLOT_TIME);
	AR5K_PRINT_REGISTER(TIME_OUT);
	AR5K_PRINT_REGISTER(RSSI_THR);
	AR5K_PRINT_REGISTER(BEACON);
	AR5K_PRINT_REGISTER(CFP_PERIOD);
	AR5K_PRINT_REGISTER(TIMER0);
	AR5K_PRINT_REGISTER(TIMER2);
	AR5K_PRINT_REGISTER(TIMER3);
	AR5K_PRINT_REGISTER(CFP_DUR);
	AR5K_PRINT_REGISTER(MCAST_FIL0);
	AR5K_PRINT_REGISTER(MCAST_FIL1);
	AR5K_PRINT_REGISTER(DIAG_SW);
	AR5K_PRINT_REGISTER(TSF_U32);
	AR5K_PRINT_REGISTER(ADDAC_TEST);
	AR5K_PRINT_REGISTER(DEFAULT_ANTENNA);
	AR5K_PRINT_REGISTER(LAST_TSTP);
	AR5K_PRINT_REGISTER(NAV);
	AR5K_PRINT_REGISTER(RTS_OK);
	AR5K_PRINT_REGISTER(ACK_FAIL);
	AR5K_PRINT_REGISTER(FCS_FAIL);
	AR5K_PRINT_REGISTER(BEACON_CNT);
	AR5K_PRINT_REGISTER(TSF_PARM);
	AR5K_PRINT_REGISTER(RATE_DUR_0);
	AR5K_PRINT_REGISTER(KEYTABLE_0);
	IOLog("\n");

	IOLog("PHY registers:\n");
	AR5K_PRINT_REGISTER(PHY_TURBO);
	AR5K_PRINT_REGISTER(PHY_AGC);
	AR5K_PRINT_REGISTER(PHY_TIMING_3);
	AR5K_PRINT_REGISTER(PHY_CHIP_ID);
	AR5K_PRINT_REGISTER(PHY_AGCCTL);
	AR5K_PRINT_REGISTER(PHY_NF);
	AR5K_PRINT_REGISTER(PHY_SCR);
	AR5K_PRINT_REGISTER(PHY_SLMT);
	AR5K_PRINT_REGISTER(PHY_SCAL);
	AR5K_PRINT_REGISTER(PHY_RX_DELAY);
	AR5K_PRINT_REGISTER(PHY_IQ);
	AR5K_PRINT_REGISTER(PHY_PAPD_PROBE);
	AR5K_PRINT_REGISTER(PHY_TXPOWER_RATE1);
	AR5K_PRINT_REGISTER(PHY_TXPOWER_RATE2);
	AR5K_PRINT_REGISTER(PHY_FC);
	AR5K_PRINT_REGISTER(PHY_RADAR);
	AR5K_PRINT_REGISTER(PHY_ANT_SWITCH_TABLE_0);
	AR5K_PRINT_REGISTER(PHY_ANT_SWITCH_TABLE_1);
	IOLog("\n");
}

void OpenHAL5212::nic_getMacAddress(u_int8_t *mac) {
	bcopy(ah_sta_id, mac, IEEE80211_ADDR_LEN);
}

HAL_BOOL OpenHAL5212::nic_setMacAddress(const u_int8_t *mac) {
	u_int32_t low_id, high_id;

	/* Set new station ID */
	bcopy(mac, ah_sta_id, IEEE80211_ADDR_LEN);

	bcopy(mac, &low_id, 4);
	bcopy(mac + 4, &high_id, 2);
	high_id = 0x0000ffff & OSSwapHostToLittleInt(high_id);

	AR5K_REG_WRITE(AR5K_AR5212_STA_ID0, low_id);
	AR5K_REG_WRITE(AR5K_AR5212_STA_ID1, OSSwapLittleToHostInt(high_id));

	return (AH_TRUE);
}


HAL_BOOL OpenHAL5212::nic_setRegulatoryDomain(u_int16_t regdomain, HAL_STATUS *status) {
	ieee80211_regdomain_t ieee_regdomain;

	ieee_regdomain = ar5k_regdomain_to_ieee(regdomain);

	if (ar5k_eeprom_regulation_domain(AH_TRUE, &ieee_regdomain) == AH_TRUE) {
		*status = HAL_OK;
		return (AH_TRUE);
	}

	*status = HAL_EIO;

	return (AH_FALSE);
}

void OpenHAL5212::nic_setLedState(HAL_LED_STATE state) {
	u_int32_t led;

	AR5K_REG_DISABLE_BITS(AR5K_AR5212_PCICFG,
	    AR5K_AR5212_PCICFG_LEDMODE |  AR5K_AR5212_PCICFG_LED);

	/*
	 * Some blinking values, define at your wish
	 */
	switch (state) {
	case IEEE80211_S_SCAN:
	case IEEE80211_S_AUTH:
		led = AR5K_AR5212_PCICFG_LEDMODE_PROP |
		    AR5K_AR5212_PCICFG_LED_PEND;
		break;

	case IEEE80211_S_INIT:
		led = AR5K_AR5212_PCICFG_LEDMODE_PROP |
		    AR5K_AR5212_PCICFG_LED_NONE;
		break;

	case IEEE80211_S_ASSOC:
	case IEEE80211_S_RUN:
		led = AR5K_AR5212_PCICFG_LEDMODE_PROP |
		    AR5K_AR5212_PCICFG_LED_ASSOC;
		break;

	default:
		led = AR5K_AR5212_PCICFG_LEDMODE_PROM |
		    AR5K_AR5212_PCICFG_LED_NONE;
		break;
	}

	AR5K_REG_ENABLE_BITS(AR5K_AR5212_PCICFG, led);
}

void OpenHAL5212::nic_writeAssocid(const u_int8_t *bssid, u_int16_t assoc_id, u_int16_t tim_offset) {
	u_int32_t low_id, high_id;

	/*
	 * Set simple BSSID mask
	 */
	AR5K_REG_WRITE(AR5K_AR5212_BSS_IDM0, 0xfffffff);
	AR5K_REG_WRITE(AR5K_AR5212_BSS_IDM1, 0xfffffff);

	/*
	 * Set BSSID which triggers the "SME Join" operation
	 */
	
	bcopy(bssid, &low_id, 4);
	bcopy(bssid + 4, &high_id, 2);
	AR5K_REG_WRITE(AR5K_AR5212_BSS_ID0, OSSwapHostToLittleInt32(low_id));
	AR5K_REG_WRITE(AR5K_AR5212_BSS_ID1, OSSwapHostToLittleInt32(high_id) |
		((assoc_id & 0x3fff) << AR5K_AR5212_BSS_ID1_AID_S));

	/*low_id = OSSwapHostToLittleInt32(*((UInt32*)  &bssid[0]));
	high_id = OSSwapHostToLittleInt16(*((UInt16*) &bssid[4]));
	
	AR5K_REG_WRITE(AR5K_AR5212_BSS_ID0, low_id);
	AR5K_REG_WRITE(AR5K_AR5212_BSS_ID1, high_id |
	    ((assoc_id & 0x3fff) << AR5K_AR5212_BSS_ID1_AID_S));
	*/
	
	bcopy(bssid, &ah_bssid, IEEE80211_ADDR_LEN);

	if (assoc_id == 0) {
		nic_disablePSPoll();
		return;
	}

	AR5K_REG_WRITE(AR5K_AR5212_BEACON,
	    (AR5K_REG_READ(AR5K_AR5212_BEACON) &
	    ~AR5K_AR5212_BEACON_TIM) |
	    (((tim_offset ? tim_offset + 4 : 0) <<
	    AR5K_AR5212_BEACON_TIM_S) &
	    AR5K_AR5212_BEACON_TIM));

	nic_enablePSPoll(NULL, 0);
}

HAL_BOOL OpenHAL5212::nic_gpioCfgInput(u_int32_t gpio) {
	if (gpio > AR5K_AR5212_NUM_GPIO)
		return (AH_FALSE);

	AR5K_REG_WRITE(AR5K_AR5212_GPIOCR,
	    (AR5K_REG_READ(AR5K_AR5212_GPIOCR) &~ AR5K_AR5212_GPIOCR_ALL(gpio))
	    | AR5K_AR5212_GPIOCR_NONE(gpio));

	return (AH_TRUE);
}

u_int32_t OpenHAL5212::nic_gpioGet(u_int32_t gpio) {
	if (gpio > AR5K_AR5212_NUM_GPIO)
		return (0xffffffff);

	/* GPIO input magic */
	return (((AR5K_REG_READ(AR5K_AR5212_GPIODI) &
	    AR5K_AR5212_GPIODI_M) >> gpio) & 0x1);
}

void OpenHAL5212::nic_gpioSetIntr(u_int gpio, u_int32_t interrupt_level) {
	u_int32_t data;

	if (gpio > AR5K_AR5212_NUM_GPIO)
		return;

	/*
	 * Set the GPIO interrupt
	 */
	data = (AR5K_REG_READ(AR5K_AR5212_GPIOCR) &
	    ~(AR5K_AR5212_GPIOCR_INT_SEL(gpio) | AR5K_AR5212_GPIOCR_INT_SELH |
	    AR5K_AR5212_GPIOCR_INT_ENA | AR5K_AR5212_GPIOCR_ALL(gpio))) |
	    (AR5K_AR5212_GPIOCR_INT_SEL(gpio) | AR5K_AR5212_GPIOCR_INT_ENA);

	AR5K_REG_WRITE(AR5K_AR5212_GPIOCR,
	    interrupt_level ? data : (data | AR5K_AR5212_GPIOCR_INT_SELH));

	ah_imr |= AR5K_AR5212_PIMR_GPIO;

	/* Enable GPIO interrupts */
	AR5K_REG_ENABLE_BITS(AR5K_AR5212_PIMR, AR5K_AR5212_PIMR_GPIO);
}

HAL_RFGAIN OpenHAL5212::nic_getRfGain() {
	u_int32_t data, type;

	if ((ah_rf_banks == NULL) || (!ah_gain.g_active))
		return (HAL_RFGAIN_INACTIVE);

	if (ah_rf_gain != HAL_RFGAIN_READ_REQUESTED)
		goto done;

	data = AR5K_REG_READ(AR5K_AR5212_PHY_PAPD_PROBE);

	if (!(data & AR5K_AR5212_PHY_PAPD_PROBE_TX_NEXT)) {
		ah_gain.g_current =
		    data >> AR5K_AR5212_PHY_PAPD_PROBE_GAINF_S;
		type = AR5K_REG_MS(data, AR5K_AR5212_PHY_PAPD_PROBE_TYPE);

		if (type == AR5K_AR5212_PHY_PAPD_PROBE_TYPE_CCK)
			ah_gain.g_current += AR5K_GAIN_CCK_PROBE_CORR;

		if (ah_radio == AR5K_AR5112) {
			ar5k_rfregs_gainf_corr();
			ah_gain.g_current =
			    ah_gain.g_current >= ah_gain.g_f_corr ?
			    (ah_gain.g_current - ah_gain.g_f_corr) :
			    0;
		}

		if (ar5k_rfregs_gain_readback() &&
		    AR5K_GAIN_CHECK_ADJUST(&ah_gain) &&
		    ar5k_rfregs_gain_adjust())
			ah_rf_gain = HAL_RFGAIN_NEED_CHANGE;
	}

 done:
	return (ah_rf_gain);
}

#pragma mark -

HAL_BOOL OpenHAL5212::nic_resetKeyCacheEntry(u_int16_t entry) {
	int i;

	AR5K_ASSERT_ENTRY(entry, AR5K_AR5212_KEYTABLE_SIZE);

	for (i = 0; i < AR5K_AR5212_KEYCACHE_SIZE; i++)
		AR5K_REG_WRITE(AR5K_AR5212_KEYTABLE_OFF(entry, i), 0);

	/* Set NULL encryption */
	AR5K_REG_WRITE(AR5K_AR5212_KEYTABLE_TYPE(entry),
	    AR5K_AR5212_KEYTABLE_TYPE_NULL);

	return (AH_FALSE);
}

#pragma mark -

HAL_BOOL OpenHAL5212::nic_setPowerMode(HAL_POWER_MODE mode, HAL_BOOL set_chip, u_int16_t sleep_duration) {
	int i;

	switch (mode) {
	case HAL_PM_AUTO:
		if (set_chip == AH_TRUE) {
			AR5K_REG_WRITE(AR5K_AR5212_SCR,
			    AR5K_AR5212_SCR_SLE | sleep_duration);
		}
		break;

	case HAL_PM_FULL_SLEEP:
		if (set_chip == AH_TRUE) {
			AR5K_REG_WRITE(AR5K_AR5212_SCR,
			    AR5K_AR5212_SCR_SLE_SLP);
		}
		break;

	case HAL_PM_AWAKE:
		if (set_chip == AH_FALSE)
			goto commit;

		AR5K_REG_WRITE(AR5K_AR5212_SCR, AR5K_AR5212_SCR_SLE_WAKE);

		for (i = 5000; i > 0; i--) {
			/* Check if the AR5212 did wake up */
			if ((AR5K_REG_READ(AR5K_AR5212_PCICFG) &
			    AR5K_AR5212_PCICFG_SPWR_DN) == 0)
				break;

			/* Wait a bit and retry */
			AR5K_DELAY(200);
			AR5K_REG_WRITE(AR5K_AR5212_SCR,
			    AR5K_AR5212_SCR_SLE_WAKE);
		}

		/* Fail if the AR5212 didn't wake up */
		if (i <= 0)
			return (AH_FALSE);
		break;

	case HAL_PM_NETWORK_SLEEP:
	case HAL_PM_UNDEFINED:
	default:
		return (AH_FALSE);
	}

 commit:
	ah_power_mode = mode;

	AR5K_REG_DISABLE_BITS(AR5K_AR5212_STA_ID1,
	    AR5K_AR5212_STA_ID1_DEFAULT_ANTENNA);
	AR5K_REG_ENABLE_BITS(AR5K_AR5212_STA_ID1,
	    AR5K_AR5212_STA_ID1_PWR_SV);

	return (AH_TRUE);
}

#pragma mark -

HAL_BOOL OpenHAL5212::nic_isInterruptPending() {
	return (AR5K_REG_READ(AR5K_AR5212_INTPEND) == 0 ? AH_FALSE : AH_TRUE);
}

HAL_BOOL OpenHAL5212::nic_getPendingInterrupts(u_int32_t *interrupt_mask) {
	u_int32_t data;

	/*
	 * Read interrupt status from the Read-And-Clear shadow register
	 */
	data = AR5K_REG_READ(AR5K_AR5212_RAC_PISR);

	/*
	 * Get abstract interrupt mask (HAL-compatible)
	 */
	*interrupt_mask = (data & HAL_INT_COMMON) & ah_imr;

	if (data == HAL_INT_NOCARD)
		return (AH_FALSE);

	if (data & (AR5K_AR5212_PISR_RXOK | AR5K_AR5212_PISR_RXERR))
		*interrupt_mask |= HAL_INT_RX;

	if (data & (AR5K_AR5212_PISR_TXOK | AR5K_AR5212_PISR_TXERR))
		*interrupt_mask |= HAL_INT_TX;

	if (data & (AR5K_AR5212_PISR_HIUERR))
		*interrupt_mask |= HAL_INT_FATAL;

	/*
	 * Special interrupt handling (not catched by the driver)
	 */
	//WE ARE A PASSIVE DRIVER implement in an active driver!!!
	//if (((*interrupt_mask) & AR5K_AR5212_PISR_RXPHY) && ah_radar.r_enabled == AH_TRUE)
	//	ar5k_radar_alert();

	return (AH_TRUE);
}

u_int32_t OpenHAL5212::nic_getInterrupts() {
	/* Return the interrupt mask stored previously */
	return (ah_imr);
}

HAL_INT OpenHAL5212::nic_setInterrupts(HAL_INT new_mask) {
	HAL_INT old_mask, int_mask;

	/*
	 * Disable card interrupts to prevent any race conditions
	 * (they will be re-enabled afterwards).
	 */
	AR5K_REG_WRITE(AR5K_AR5212_IER, AR5K_AR5212_IER_DISABLE);

	old_mask = ah_imr;

	/*
	 * Add additional, chipset-dependent interrupt mask flags
	 * and write them to the IMR (interrupt mask register).
	 */
	int_mask = new_mask & HAL_INT_COMMON;

	if (new_mask & HAL_INT_RX)
		int_mask |=
		    AR5K_AR5212_PIMR_RXOK |
		    AR5K_AR5212_PIMR_RXERR |
		    AR5K_AR5212_PIMR_RXORN |
		    AR5K_AR5212_PIMR_RXDESC;

	if (new_mask & HAL_INT_TX)
		int_mask |=
		    AR5K_AR5212_PIMR_TXOK |
		    AR5K_AR5212_PIMR_TXERR |
		    AR5K_AR5212_PIMR_TXDESC |
		    AR5K_AR5212_PIMR_TXURN;

	if (new_mask & HAL_INT_FATAL) {
		int_mask |= AR5K_AR5212_PIMR_HIUERR;
		AR5K_REG_ENABLE_BITS(AR5K_AR5212_SIMR2,
		    AR5K_AR5212_SIMR2_MCABT |
		    AR5K_AR5212_SIMR2_SSERR |
		    AR5K_AR5212_SIMR2_DPERR);
	}

	if (ah_op_mode & HAL_M_HOSTAP) {
		int_mask |= AR5K_AR5212_PIMR_MIB;
	} else {
		int_mask &= ~AR5K_AR5212_PIMR_MIB;
	}

	AR5K_REG_WRITE(AR5K_AR5212_PIMR, int_mask);

	/* Store new interrupt mask */
	ah_imr = new_mask;

	/* ..re-enable interrupts */
	AR5K_REG_WRITE(AR5K_AR5212_IER, AR5K_AR5212_IER_ENABLE);

	return (old_mask);
}

#pragma mark -

HAL_BOOL OpenHAL5212::nic_get_capabilities()
{
	u_int16_t ee_header;

	/* Capabilities stored in the EEPROM */
	ee_header = ah_capabilities.cap_eeprom.ee_header;

	/*
	 * XXX The AR5212 tranceiver supports frequencies from 4920 to 6100GHz
	 * XXX and from 2312 to 2732GHz. There are problems with the current
	 * XXX ieee80211 implementation because the IEEE channel mapping
	 * XXX does not support negative channel numbers (2312MHz is channel
	 * XXX -19). Of course, this doesn't matter because these channels
	 * XXX are out of range but some regulation domains like MKK (Japan)
	 * XXX will support frequencies somewhere around 4.8GHz.
	 */

	/*
	 * Set radio capabilities
	 */

	if (AR5K_EEPROM_HDR_11A(ee_header)) {
		ah_capabilities.cap_range.range_5ghz_min = 5005; /* 4920 */
		ah_capabilities.cap_range.range_5ghz_max = 6100;

		/* Set supported modes */
		ah_capabilities.cap_mode =
		    HAL_MODE_11A | HAL_MODE_TURBO | HAL_MODE_XR;
	}

	/* This chip will support 802.11b if the 2GHz radio is connected */
	if (AR5K_EEPROM_HDR_11B(ee_header) || AR5K_EEPROM_HDR_11G(ee_header)) {
		ah_capabilities.cap_range.range_2ghz_min = 2412; /* 2312 */
		ah_capabilities.cap_range.range_2ghz_max = 2732;
		ah_capabilities.cap_mode |= HAL_MODE_11B;

		if (AR5K_EEPROM_HDR_11B(ee_header))
			ah_capabilities.cap_mode |= HAL_MODE_11B;
		if (AR5K_EEPROM_HDR_11G(ee_header))
			ah_capabilities.cap_mode |= HAL_MODE_11G;
	}

	/* GPIO */
	ah_gpio_npins = AR5K_AR5212_NUM_GPIO;

	/* Set number of supported TX queues */
	ah_capabilities.cap_queues.q_tx_num = AR5K_AR5212_TX_NUM_QUEUES;

	return (AH_TRUE);
}

#pragma mark -

HAL_BOOL OpenHAL5212::nic_eeprom_is_busy() {
	return (AR5K_REG_READ(AR5K_AR5212_CFG) & AR5K_AR5212_CFG_EEBS ?
	    AH_TRUE : AH_FALSE);
}

HAL_STATUS OpenHAL5212::nic_eeprom_read(u_int32_t offset, u_int16_t *data)
{
	u_int32_t status, i;
	u_int32_t o = offset;
	/*
	 * Initialize EEPROM access
	 */
	 
	if (ah_device == 0xff13) {
		if (offset >= 0x400) {
			AR5K_PRINTF("EEPROM access out of bounds (offset = 0x%x)", offset);
			return HAL_EIO;
		}
		*data = firmware[offset];
		return HAL_OK;
	}
	
	AR5K_REG_WRITE(AR5K_AR5212_EEPROM_BASE, o);
	AR5K_REG_ENABLE_BITS(AR5K_AR5212_EEPROM_CMD,
	    AR5K_AR5212_EEPROM_CMD_READ);

	for (i = AR5K_TUNE_REGISTER_TIMEOUT; i > 0; i--) {
		status = AR5K_REG_READ(AR5K_AR5212_EEPROM_STATUS);
		if (status & AR5K_AR5212_EEPROM_STAT_RDDONE) {
			if (status & AR5K_AR5212_EEPROM_STAT_RDERR)
				return (HAL_EIO);
			*data = (u_int16_t)
			    (AR5K_REG_READ(AR5K_AR5212_EEPROM_DATA) & 0xffff);
			return (HAL_OK);
		}
		AR5K_DELAY(15);
	}
	
	AR5K_PRINTF("Could not read EEPROM at pos 0x%x. Timeout. Status 0x%x\n", offset, status);
	return (HAL_EEREAD);
}

HAL_STATUS OpenHAL5212::nic_eeprom_write(u_int32_t offset, u_int16_t data)
{
	u_int32_t status, timeout;

	/*
	 * Prime write pump
	 */
	AR5K_REG_WRITE(AR5K_AR5212_EEPROM_BASE, offset);
	AR5K_REG_WRITE(AR5K_AR5212_EEPROM_DATA, data);
	AR5K_REG_WRITE(AR5K_AR5212_EEPROM_CMD, AR5K_AR5212_EEPROM_CMD_WRITE);

	for (timeout = 10000; timeout > 0; timeout--) {
		AR5K_DELAY(1);
		status = AR5K_REG_READ(AR5K_AR5212_EEPROM_STATUS);
		if (status & AR5K_AR5212_EEPROM_STAT_WRDONE) {
			if (status & AR5K_AR5212_EEPROM_STAT_WRERR)
				return (HAL_EIO);
			return (HAL_OK);
		}
	}

	return (HAL_EEWRITE);
}

#pragma mark -

HAL_BOOL OpenHAL5212::ar5k_ar5212_txpower(HAL_CHANNEL *channel, u_int txpower) {
	HAL_BOOL tpc = ah_txpower.txp_tpc;
	int i;

	if (txpower > AR5K_TUNE_MAX_TXPOWER) {
		AR5K_PRINTF("invalid tx power: %u\n", txpower);
		return (AH_FALSE);
	}

	/* Reset TX power values */
	bzero(&ah_txpower, sizeof(ah_txpower));
	ah_txpower.txp_tpc = tpc;

	/* Initialize TX power table */
	ar5k_txpower_table(channel, txpower);

	/* 
	 * Write TX power values
	 */
	for (i = 0; i < (AR5K_EEPROM_POWER_TABLE_SIZE / 2); i++) {
		AR5K_REG_WRITE(AR5K_AR5212_PHY_PCDAC_TXPOWER(i),
		    ((((ah_txpower.txp_pcdac[(i << 1) + 1] << 8) | 0xff) &
		    0xffff) << 16) | (((ah_txpower.txp_pcdac[i << 1] << 8)
		    | 0xff) & 0xffff));
	}

	AR5K_REG_WRITE(AR5K_AR5212_PHY_TXPOWER_RATE1,
	    AR5K_TXPOWER_OFDM(3, 24) | AR5K_TXPOWER_OFDM(2, 16)
	    | AR5K_TXPOWER_OFDM(1, 8) | AR5K_TXPOWER_OFDM(0, 0));

	AR5K_REG_WRITE(AR5K_AR5212_PHY_TXPOWER_RATE2,
	    AR5K_TXPOWER_OFDM(7, 24) | AR5K_TXPOWER_OFDM(6, 16)
	    | AR5K_TXPOWER_OFDM(5, 8) | AR5K_TXPOWER_OFDM(4, 0));

	AR5K_REG_WRITE(AR5K_AR5212_PHY_TXPOWER_RATE3,
	    AR5K_TXPOWER_CCK(10, 24) | AR5K_TXPOWER_CCK(9, 16)
	    | AR5K_TXPOWER_CCK(15, 8) | AR5K_TXPOWER_CCK(8, 0));

	AR5K_REG_WRITE(AR5K_AR5212_PHY_TXPOWER_RATE4,
	    AR5K_TXPOWER_CCK(14, 24) | AR5K_TXPOWER_CCK(13, 16)
	    | AR5K_TXPOWER_CCK(12, 8) | AR5K_TXPOWER_CCK(11, 0));

	if (ah_txpower.txp_tpc == AH_TRUE) {
		AR5K_REG_WRITE(AR5K_AR5212_PHY_TXPOWER_RATE_MAX,
		    AR5K_AR5212_PHY_TXPOWER_RATE_MAX_TPC_ENABLE |
		    AR5K_TUNE_MAX_TXPOWER);
	} else {
		AR5K_REG_WRITE(AR5K_AR5212_PHY_TXPOWER_RATE_MAX,
		    AR5K_AR5212_PHY_TXPOWER_RATE_MAX |
		    AR5K_TUNE_MAX_TXPOWER);
	}

	return (AH_TRUE);
}

#pragma mark -

HAL_BOOL OpenHAL5212::nic_channel(HAL_CHANNEL *channel) {
	u_int32_t data, data0, data1, data2;
	u_int16_t c;

	data = data0 = data1 = data2 = 0;
	c = channel->c_channel;

	/*
	 * Set the channel on the AR5112 or newer
	 */
	if (c < 4800) {
		if (!((c - 2224) % 5)) {
			data0 = ((2 * (c - 704)) - 3040) / 10;
			data1 = 1;
		} else if (!((c - 2192) % 5)) {
			data0 = ((2 * (c - 672)) - 3040) / 10;
			data1 = 0;
		} else
			return (AH_FALSE);

		data0 = ar5k_bitswap((data0 << 2) & 0xff, 8);
	} else {
		if (!(c % 20) && c >= 5120) {
			data0 = ar5k_bitswap(((c - 4800) / 20 << 2), 8);
			data2 = ar5k_bitswap(3, 2);
		} else if (!(c % 10)) {
			data0 = ar5k_bitswap(((c - 4800) / 10 << 1), 8);
			data2 = ar5k_bitswap(2, 2);
		} else if (!(c % 5)) {
			data0 = ar5k_bitswap((c - 4800) / 5, 8);
			data2 = ar5k_bitswap(1, 2);
		} else
			return (AH_FALSE);
	}

	data = (data0 << 4) | (data1 << 1) | (data2 << 2) | 0x1001;

	AR5K_PHY_WRITE(0x27, data & 0xff);
	AR5K_PHY_WRITE(0x36, (data >> 8) & 0x7f);

	return (AH_TRUE);
}