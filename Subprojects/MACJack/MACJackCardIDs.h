/*
        
        File:			MACJackCardIDs.h
        Program:		MACJack
		Author:			Michael Rossberg
						mick@binaervarianz.de
		Description:	KisMAC is a wireless stumbler for MacOS X.
                
        This file is part of KisMAC.
        
        This file is taken from the OpenBSD wi driver and AirJack

    KisMAC is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    KisMAC is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with KisMAC; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#define	_TX_CFPOLL			(0x1000)
#define	_TX_PRST                        (0x0800)
#define	_TX_MACPORT			(0x0700)
#define	_TX_NOENCRYPT			(0x0080)
#define	_TX_RETRYSTRAT			(0x0060)
#define	_TX_STRUCTYPE			(0x0018)
#define	_TX_TXEX                        (0x0004)
#define	_TX_TXOK                        (0x0001)
#define	_TX_SET(v,m,s)			((((UInt16)(v))<<((UInt16)(s)))&((UInt16)(m)))
#define	_TX_RETRYSTRAT_SET(v)		_TX_SET(v, _TX_RETRYSTRAT, 5)
#define	_TX_CFPOLL_SET(v)		_TX_SET(v, _TX_CFPOLL,12)
#define	_TX_MACPORT_SET(v)		_TX_SET(v, _TX_MACPORT, 8)
#define	_TX_TXEX_SET(v)			_TX_SET(v, _TX_TXEX, 2)
#define	_TX_TXOK_SET(v)			_TX_SET(v, _TX_TXOK, 1)

/* Firmware types */
#define	WI_NOTYPE	0
#define	WI_LUCENT	1
#define	WI_INTERSIL	2
#define	WI_SYMBOL	3

/* Card identities */
#define	WI_NIC_LUCENT		0x0001
#define	WI_NIC_SONY		0x0002
#define	WI_NIC_LUCENT_EMB	0x0005
#define	WI_NIC_EVB2		0x8000
#define	WI_NIC_HWB3763		0x8001
#define	WI_NIC_HWB3163		0x8002
#define	WI_NIC_HWB3163B		0x8003
#define	WI_NIC_EVB3		0x8004
#define	WI_NIC_HWB1153		0x8007
#define	WI_NIC_P2_SST		0x8008	/* Prism2 with SST flush */
#define	WI_NIC_EVB2_SST		0x8009
#define	WI_NIC_3842_EVA		0x800A	/* 3842 Evaluation Board */

#define	WI_NIC_3842_PCMCIA_AMD	0x800B	/* Prism2.5 PCMCIA */
#define	WI_NIC_3842_PCMCIA_SST	0x800C
#define	WI_NIC_3842_PCMCIA_ATL	0x800D
#define	WI_NIC_3842_PCMCIA_ATS	0x800E

#define	WI_NIC_3842_MINI_AMD	0x8012	/* Prism2.5 Mini-PCI */
#define	WI_NIC_3842_MINI_SST	0x8013
#define	WI_NIC_3842_MINI_ATL	0x8014
#define	WI_NIC_3842_MINI_ATS	0x8015

#define	WI_NIC_3842_PCI_AMD	0x8016	/* Prism2.5 PCI-bridge */
#define	WI_NIC_3842_PCI_SST	0x8017
#define	WI_NIC_3842_PCI_ATL	0x8018
#define	WI_NIC_3842_PCI_ATS	0x8019

#define	WI_NIC_P3_PCMCIA_AMD	0x801A	/* Prism3 PCMCIA */
#define	WI_NIC_P3_PCMCIA_SST	0x801B
#define	WI_NIC_P3_PCMCIA_ATL	0x801C
#define	WI_NIC_P3_PCMCIA_ATS	0x801D

#define	WI_NIC_P3_MINI_AMD	0x8021	/* Prism3 Mini-PCI */
#define	WI_NIC_P3_MINI_SST	0x8022
#define	WI_NIC_P3_MINI_ATL	0x8023
#define	WI_NIC_P3_MINI_ATS	0x8024

#define WI_CARD_IDS							\
	{								\
		WI_NIC_LUCENT,						\
		"Lucent WaveLAN/IEEE",					\
		WI_LUCENT						\
	}, {								\
		WI_NIC_SONY,						\
		"Sony WaveLAN/IEEE",					\
		WI_LUCENT						\
	}, {								\
		WI_NIC_LUCENT_EMB,					\
		"Lucent Embedded WaveLAN/IEEE",				\
		WI_LUCENT						\
	}, {								\
		WI_NIC_EVB2,						\
		"PRISM2 HFA3841(EVB2)",					\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_HWB3763,						\
		"PRISM2 HWB3763 rev.B",					\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_HWB3163,						\
		"PRISM2 HWB3163 rev.A",					\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_HWB3163B,					\
		"PRISM2 HWB3163 rev.B",					\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_EVB3,						\
		"PRISM2 HFA3842(EVB3)",					\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_HWB1153,						\
		"PRISM1 HWB1153",					\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_P2_SST,						\
		"PRISM2 HWB3163 SST-flash",				\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_EVB2_SST,					\
		"PRISM2 HWB3163(EVB2) SST-flash",			\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_3842_EVA,					\
		"PRISM2 HFA3842(EVAL)",					\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_3842_PCMCIA_AMD,					\
		"PRISM2.5 ISL3873",					\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_3842_PCMCIA_SST,					\
		"PRISM2.5 ISL3873",					\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_3842_PCMCIA_ATL,					\
		"PRISM2.5 ISL3873",					\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_3842_PCMCIA_ATS,					\
		"PRISM2.5 ISL3873",					\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_3842_MINI_AMD,					\
		"PRISM2.5 ISL3874A(Mini-PCI)",				\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_3842_MINI_SST,					\
		"PRISM2.5 ISL3874A(Mini-PCI)",				\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_3842_MINI_ATL,					\
		"PRISM2.5 ISL3874A(Mini-PCI)",				\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_3842_MINI_ATS,					\
		"PRISM2.5 ISL3874A(Mini-PCI)",				\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_3842_PCI_AMD,					\
		"PRISM2.5 ISL3874A(PCI-bridge)",			\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_3842_PCI_SST,					\
		"PRISM2.5 ISL3874A(PCI-bridge)",			\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_3842_PCI_ATS,					\
		"PRISM2.5 ISL3874A(PCI-bridge)",			\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_3842_PCI_ATL,					\
		"PRISM2.5 ISL3874A(PCI-bridge)",			\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_P3_PCMCIA_AMD,					\
		"PRISM3 ISL37300P",					\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_P3_PCMCIA_SST,					\
		"PRISM3 ISL37300P",					\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_P3_PCMCIA_ATL,					\
		"PRISM3 ISL37300P",					\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_P3_PCMCIA_ATS,					\
		"PRISM3 ISL37300P",					\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_P3_MINI_AMD,					\
		"PRISM3 ISL37300P(PCI)",				\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_P3_MINI_SST,					\
		"PRISM3 ISL37300P(PCI)",				\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_P3_MINI_ATL,					\
		"PRISM3 ISL37300P(PCI)",				\
		WI_INTERSIL						\
	}, {								\
		WI_NIC_P3_MINI_ATS,					\
		"PRISM3 ISL37300P(PCI)",				\
		WI_INTERSIL						\
	}, {								\
		0,							\
		NULL,							\
		WI_NOTYPE						\
	}
