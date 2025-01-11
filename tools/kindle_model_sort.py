#!/usr/bin/env python3

import sys
from operator import itemgetter

# NOTE: Pilfered from https://code.activestate.com/recipes/65212/
# FIXME: Crockford's Base32, but with the "L" & "U" re-added in?
# NOTE: In case this ever needs fixing, don't forget to update the horrible regex used in MRPI to parse our verbose output,
#       to avoid a repeat of what r16043 fixed...
def baseN(num, base, numerals="0123456789ABCDEFGHJKLMNPQRSTUVWX"):
	if num == 0:
		return "0"

	if num < 0:
		return '-' + baseN((-1) * num, base, numerals)

	if not 2 <= base <= len(numerals):
		raise ValueError('Base must be between 2 and %d' % len(numerals))

	left_digits = num // base
	if left_digits == 0:
		return numerals[num % base]
	else:
		return baseN(left_digits, base, numerals) + numerals[num % base]

# NOTE: Pilfered from https://stackoverflow.com/questions/1119722/
BASE_LIST = tuple("0123456789ABCDEFGHJKLMNPQRSTUVWX")
BASE_DICT = dict((c, v) for v, c in enumerate(BASE_LIST))
BASE_LEN = len(BASE_LIST)

def devCode(string):
	num = 0
	for char in string:
		num = num * BASE_LEN + BASE_DICT[char]
	return num

# Just do a conversion (SN -> devCode) if we were passed an argument...
if len(sys.argv) > 1:
	print("0x{:03X}".format(devCode(sys.argv[1])))
	exit()

model_tuples = [
	('Kindle1',						0x01,	'ATVPDKIKX0DER'),
	('Kindle2US',						0x02,	'A3UN6WX5RRO2AG'),
	('Kindle2International',				0x03,	'A1F83G8C2ARO7P'),
	('KindleDXUS',						0x04,	'A1PA6795UKMFR9'),
	('KindleDXInternational',				0x05,	'A13V1IB3VIYZZH'),
	('ValidKindleUnknown_0x07',				0x07,	'A2EUQ1WTGCTBG2'),
	('Kindle3WiFi3G',					0x06,	'A1VC38T7YXB528'),
	('Kindle3WiFi',						0x08,	'A3AEGXETSR30VB'),
	('KindleDXGraphite',					0x09,	'A3P5ROKL5A1OLE'),
	('Kindle3WiFi3GEurope',					0x0A,	'A3JWKAKR8XB7XF'),
	('ValidKindleUnknown_0x0B',				0x0B,	'A1X6FK5RDHNB96'),
	('ValidKindleUnknown_0x0C',				0x0C,	'AN1VRQENFRJN5'),
	('ValidKindleUnknown_0x0D',				0x0D,	'A3DWYIK6Y9EEQB'),
	('Kindle4NonTouch',					0x0E,	'A3R76HOPU0Z2CB'),
	('Kindle5TouchWiFi3G',					0x0F,	'A1IM4EOPHS76S7'),
	('Kindle5TouchWiFi3GEurope',				0x10,	'A138L1TOL8PIJT'),
	('Kindle5TouchWiFi',					0x11,	'A3T4TT2Z381HKD'),
	('Kindle5TouchUnknown',					0x12,	'A3LJ5WMKNRFKQS'),
	('KindlePaperWhiteWiFi3G',				0x1B,	'A1JYRMDPD0WRC1'),
	('KindlePaperWhiteWiFi3GCanada',			0x1C,	'A1U5RCOVU0NYF2'),
	('KindlePaperWhiteWiFi3GEurope',			0x1D,	'A1I7TFXKDRQDZL'),
	('KindlePaperWhiteWiFi3GJapan',				0x1F,	'A1K21FY43GMZF8'),
	('KindlePaperWhiteWiFi3GBrazil',			0x20,	'A3RN7G7QC5MWSZ'),
	('Kindle4NonTouchBlack',				0x23,	'AMMK0LS9EDNM8'),
	('KindlePaperWhiteWiFi',				0x24,	'A3VSAZHKW7EWVH'),
	('KindlePaperWhite2WiFiJapan',				0x5A,	'A1XFE4LQM16OSW'),
	('KindlePaperWhite2WiFi',				0xD4,	'A2X1JOFWQIYV75'),
	('KindlePaperWhite2WiFi3G',				0xD5,	'A2LTUGSV2JQ93O'),
	('KindlePaperWhite2WiFi3GCanada',			0xD6,	'A3CG2RMGG8NQEJ'),
	('KindlePaperWhite2WiFi3GEurope',			0xD7,	'A2RWEQK36M6DUE'),
	('KindlePaperWhite2WiFi3GRussia',			0xD8,	'A3DM9ZTSZGUSMW'),
	('KindlePaperWhite2WiFi3GJapan',			0xF2,	'A36L7QE2V0XKCZ'),
	('KindlePaperWhite2WiFi4GBInternational',		0x17,	'A3I3CR3NPZFVHY'),
	('KindlePaperWhite2WiFi3G4GBCanada',			0x5F,	'A16EMENY0O3Z2H'),
	('KindlePaperWhite2WiFi3G4GBEurope',			0x60,	'A3D1N3J5SXSYPF'),
	('KindlePaperWhite2WiFi3G4GBBrazil',			0x61,	'A3NRQ2KXEO33BF'),
	('KindlePaperWhite2WiFi3G4GB',				0x62,	'A3QT0UFVNUDPAE'),
	('KindlePaperWhite2Unknown_0xF4',			0xF4,	'A3JI3C11GUW6OM'),
	('KindlePaperWhite2Unknown_0xF9',			0xF9,	'A148QFVDZ3MQ8V'),
	('KindleVoyageWiFi',					0x13,	'A3FE7AD5N5R11'),
	('KindleVoyageWiFi3G',					0x54,	'A1VHVRSIVA49BF'),
	('KindleVoyageWiFi3GJapan',				0x2A,	'A2KSI370ME58SV'),
	('KindleVoyageWiFi3G_0x4F',					0x4F,	'AEK24W3B90XSI'),
	('KindleVoyageWiFi3GMexico',				0x52,	'A66ZTOXC8UWFP'),
	('KindleVoyageWiFi3GEurope',				0x53,	'A26JMGYIXWMKGL'),
	('KindleBasic',						0xC6,	'A2TNPB8EVLW5FA'),
	('ValidKindleUnknown_0x99',				0x99,	'A2I96HKA5TK143'),
	('KindleBasicKiwi',					0xDD,	'A9N06WOIL49CA'),
	('ValidKindleUnknown_0x16',				0x16),
	('ValidKindleUnknown_0x21',				0x21),
	('KindlePaperWhite3WiFi',				0x201,	'A21RY355YUXQAF'),	# 0G1
	('KindlePaperWhite3WiFi3G',				0x202,	'A6S0KGW65V1TV'),	# 0G2
	('KindlePaperWhite3WiFi3GMexico',			0x204,	'A3P87LH4DLAKE2'),	# 0G4
	('KindlePaperWhite3WiFi3GEurope',			0x205,	'A3OLIINW419WLP'),	# 0G5
	('KindlePaperWhite3WiFi3GCanada',			0x206,	'AOPKCG97868D2'),	# 0G6
	('KindlePaperWhite3WiFi3GJapan',			0x207,	'A3MTNJ7FDYZOPO'),	# 0G7
	('KindlePaperWhite3WhiteWiFi',				0x26B,	'A21RY355YUXQAF'),	# 0KB
	('KindlePaperWhite3WhiteWiFi3GJapan',			0x26C,	'A3MTNJ7FDYZOPO'),	# 0KC
	('KindlePW3WhiteUnknown_0KD',				0x26D,	'AOPKCG97868D2'),	# 0KD?
	('KindlePaperWhite3WhiteWiFi3GInternational',		0x26E,	'A3OLIINW419WLP'),	# 0KE
	('KindlePaperWhite3WhiteWiFi3GInternationalBis',	0x26F,	'A6S0KGW65V1TV'),	# 0KF
	('KindlePW3WhiteUnknown_0KG',				0x270,	'A3P87LH4DLAKE2'),	# 0KG?
	('KindlePaperWhite3WiFi32GBJapanBlack',			0x293,	'A2T9E09EBKRBWU'),	# 0LK
	('KindlePaperWhite3WiFi32GBJapanWhite',			0x294,	'A2T9E09EBKRBWU'),	# 0LL
	('KindlePW3Unknown_TTT',				0x6F7B,	'A21RY355YUXQAF'),	# TTT?
	('KindleOasisWiFi',					0x20C,	'A2NP90AR02CXEG'),	# 0GC
	('KindleOasisWiFi3G',					0x20D,	'A370DV3BFIHFD3'),	# 0GD
	('KindleOasisWiFi3GInternational',			0x219,	'A21R12JDS0I7HR'),	# 0GR
	('KindleOasisUnknown_0GS',				0x21A,	'A2G9XCYZJMNLQK'),	# 0GS?
	('KindleOasisWiFi3GChina',				0x21B,	'AIOUHGSC1FXK5'),	# 0GT
	('KindleOasisWiFi3GEurope',				0x21C,	'A1VYPQEAEVB479'),	# 0GU
	('KindleBasic2Unknown_0DU',				0x1BC),				# 0DU?
	('KindleBasic2',					0x269,	'A363JBKK6AP29Q'),	# 0K9
	('KindleBasic2White',					0x26A,	'A363JBKK6AP29Q'),	# 0KA
	('KindleOasis2Unknown_0LM',				0x295,	'A2AVNKP6ZINL5'),	# 0LM?
	('KindleOasis2Unknown_0LN',				0x296,	'A1SZ6LXIZK7826'),	# 0LN?
	('KindleOasis2Unknown_0LP',				0x297,	'A3M646A6GS49CA'),	# 0LP?
	('KindleOasis2Unknown_0LQ',				0x298,	'A39S6AFBERWZOH'),	# 0LQ?
	('KindleOasis2WiFi32GBChampagne',			0x2E1,	'A1SZ6LXIZK7826'),	# 0P1
	('KindleOasis2Unknown_0P2',				0x2E2,	'A2AVNKP6ZINL5'),	# 0P2?
	('KindleOasis2Unknown_0P6',				0x2E6,	'A3M646A6GS49CA'),	# 0P6
	('KindleOasis2Unknown_0P7',				0x2E7,	'A39S6AFBERWZOH'),	# 0P7?
	('KindleOasis2WiFi8GB',					0x2E8,	'A1SZ6LXIZK7826'),	# 0P8
	('KindleOasis2WiFi3G32GB',				0x341,	'A2AVNKP6ZINL5'),	# 0S1
	('KindleOasis2WiFi3G32GBEurope',			0x342,	'A3M646A6GS49CA'),	# 0S2
	('KindleOasis2Unknown_0S3',				0x343,	'A39S6AFBERWZOH'),	# 0S3?
	('KindleOasis2Unknown_0S4',				0x344,	'A1SZ6LXIZK7826'),	# 0S4?
	('KindleOasis2Unknown_0S7',				0x347,	'A1SZ6LXIZK7826'),	# 0S7?
	('KindleOasis2WiFi32GB',				0x34A,	'A1SZ6LXIZK7826'),	# 0SA
	('KindlePaperWhite4WiFi8GB',				0x2F7,	'AJRLVDTOPT1LE'),	# 0PP
	('KindlePaperWhite4WiFi4G32GB',				0x361,	'A3IT5K46YEJ8DG'),	# 0T1
	('KindlePaperWhite4WiFi4G32GBEurope',			0x362,	'A2J0U8ZY7AYQWV'),	# 0T2
	('KindlePaperWhite4WiFi4G32GBJapan',			0x363,	'AV9Q59KU8EJQE'),	# 0T3
	('KindlePaperWhite4Unknown_0T4',			0x364,	'A27ME72Q2PS699'),	# 0T4?
	('KindlePaperWhite4Unknown_0T5',			0x365,	'A3IT5K46YEJ8DG'),	# 0T5?
	('KindlePaperWhite4WiFi32GB',				0x366,	'AJRLVDTOPT1LE'),	# 0T6
	('KindlePaperWhite4Unknown_0T7',			0x367,	'AJRLVDTOPT1LE'),	# 0T7?
	('KindlePaperWhite4Unknown_0TJ',			0x372,	'AJRLVDTOPT1LE'),	# 0TJ?
	('KindlePaperWhite4Unknown_0TK',			0x373,	'AJRLVDTOPT1LE'),	# 0TK?
	('KindlePaperWhite4Unknown_0TL',			0x374,	'A2J0U8ZY7AYQWV'),	# 0TL?
	('KindlePaperWhite4Unknown_0TM',			0x375,	'AV9Q59KU8EJQE'),	# 0TM?
	('KindlePaperWhite4Unknown_0TN',			0x376,	'A27ME72Q2PS699'),	# 0TN?
	('KindlePaperWhite4WiFi8GBIndia',			0x402,	'AJRLVDTOPT1LE'),	# 102
	('KindlePaperWhite4WiFi32GBIndia',			0x403,	'A2J0U8ZY7AYQWV'),	# 103
	('KindlePaperWhite4WiFi32GBBlue',			0x4D8,	'AJRLVDTOPT1LE'),	# 16Q
	('KindlePaperWhite4WiFi32GBPlum',			0x4D9,	'AJRLVDTOPT1LE'),	# 16R
	('KindlePaperWhite4WiFi32GBSage',			0x4DA,	'AJRLVDTOPT1LE'),	# 16S
	('KindlePaperWhite4WiFi8GBBlue',			0x4DB,	'AJRLVDTOPT1LE'),	# 16T
	('KindlePaperWhite4WiFi8GBPlum',			0x4DC,	'AJRLVDTOPT1LE'),	# 16U
	('KindlePaperWhite4WiFi8GBSage',			0x4DD,	'AJRLVDTOPT1LE'),	# 16V
	('KindlePW4Unknown_0PL',				0x2F4,	'A3IT5K46YEJ8DG'),	# 0PL?
	('KindleBasic3',					0x414,	'AHU5VU98ZZYIL'),	# 10L
	('KindleBasic3White8GB',				0x3CF,	'AHU5VU98ZZYIL'),	# 0WF
	('KindleBasic3Unknown_0WG',				0x3D0,	'AHU5VU98ZZYIL'),	# 0WG?
	('KindleBasic3White',					0x3D1,	'AHU5VU98ZZYIL'),	# 0WH
	('KindleBasic3Unknown_0WJ',				0x3D2,	'AHU5VU98ZZYIL'),	# 0WJ?
	('KindleBasic3KidsEdition',				0x3AB,	'AHU5VU98ZZYIL'),	# 0VB
	('KindleOasis3WiFi32GBChampagne',			0x434,	'A2NW3VDYR5P8Z0'),	# 11L
	('KindleOasis3WiFi4G32GBJapan',				0x3D8,	'A28MDQJEP7D12S'),	# 0WQ
	('KindleOasis3WiFi4G32GBIndia',				0x3D7,	'A2M7UZTFTYKRHM'),	# 0WP
	('KindleOasis3WiFi4G32GB',				0x3D6,	'AB6KN53ZYVL6D'),	# 0WN
	('KindleOasis3WiFi32GB',				0x3D5,	'A2NW3VDYR5P8Z0'),	# 0WM
	('KindleOasis3WiFi8GB',					0x3D4,	'A2NW3VDYR5P8Z0'),	# 0WL
	('KindlePaperWhite5SignatureEdition',			0x690,	'A328XUBPG464LQ'),	# 1LG
	('KindlePaperWhite5Unknown_1Q0',			0x700,	'A328XUBPG464LQ'),	# 1Q0?
	('KindlePaperWhite5',					0x6FF,	'A328XUBPG464LQ'),	# 1PX
	('KindlePaperWhite5Unknown_1VD',			0x7AD,	'A328XUBPG464LQ'),	# 1VD?
	('KindlePaperWhite5SE_219',				0x829,	'A328XUBPG464LQ'),	# 219
	('KindlePaperWhite5_21A',				0x82A,	'A328XUBPG464LQ'),	# 21A
	('KindlePaperWhite5SE_2BH',				0x971,	'A328XUBPG464LQ'),	# 2BH
	('KindlePaperWhite5Unknown_2BJ',			0x972,	'A328XUBPG464LQ'),	# 2BJ?
	('KindlePaperWhite5_2DK',				0x9B3,	'A328XUBPG464LQ'),	# 2DK
	('KindleBasic4Unknown_22D',				0x84D,	'A1S35GJCTB6VUN'),	# 22D?
	('KindleBasic4Unknown_25T',				0x8BB,	'A1S35GJCTB6VUN'),	# 25T?
	('KindleBasic4Unknown_23A',				0x86A,	'A1S35GJCTB6VUN'),	# 23A?
	('KindleBasic4_2AQ',					0x958,	'A1S35GJCTB6VUN'),	# 2AQ
	('KindleBasic4_2AP',					0x957,	'A1S35GJCTB6VUN'),	# 2AP
	('KindleBasic4Unknown_1XH',				0x7F1,	'A1S35GJCTB6VUN'),	# 1XH?
	('KindleBasic4Unknown_22C',				0x84C,	'A1S35GJCTB6VUN'),	# 22C?
	('KindleScribeUnknown_27J',				0x8F2,	'A12KI9K1KHHBVF'),	# 27J?
	('KindleScribeUnknown_2BL',				0x974,	'A12KI9K1KHHBVF'),	# 2BL?
	('KindleScribeUnknown_263',				0x8C3,	'A12KI9K1KHHBVF'),	# 263?
	('KindleScribe16GB_227',				0x847,	'A12KI9K1KHHBVF'),	# 227
	('KindleScribeUnknown_2BM',				0x975,	'A12KI9K1KHHBVF'),	# 2BM?
	('KindleScribe_23L',					0x874,	'A12KI9K1KHHBVF'),	# 23L
	('KindleScribe64GB_23M',				0x875,	'A12KI9K1KHHBVF'),	# 23M
	('KindleScribeUnknown_270',				0x8E0,	'A12KI9K1KHHBVF'),	# 270?
	('KindleBasic5Unknown_3L5',				0xE85,	'A2AJ1N357FEMTV'),	# 3L5?
	('KindleBasic5Unknown_3L6',				0xE86,	'A2AJ1N357FEMTV'),	# 3L6?
	('KindleBasic5Unknown_3L4',				0xE84,	'A2AJ1N357FEMTV'),	# 3L4?
	('KindleBasic5Unknown_3L3',				0xE83,	'A2AJ1N357FEMTV'),	# 3L3?
	('KindleBasic5Unknown_A89',				0x2909,	'A2AJ1N357FEMTV'),	# A89?
	('KindleBasic5Unknown_3L2',				0xE82,	'A2AJ1N357FEMTV'),	# 3L2?
	('KindleBasic5Unknown_3KM',				0xE75,	'A2AJ1N357FEMTV'),	# 3KM
	('KindlePaperWhite6Unknown_349',			0xC89,	'A1BF5SA90HOYO2'),	# 349?
	('KindlePaperWhite6Unknown_346',			0xC86,	'A1BF5SA90HOYO2'),	# 346?
	('KindlePaperWhite6Unknown_33X',			0xC7F,	'A1BF5SA90HOYO2'),	# 33X
	('KindlePaperWhite6Unknown_33W',			0xC7E,	'A1BF5SA90HOYO2'),	# 33W?
	('KindlePaperWhite6Unknown_3HA',			0xE2A,	'A1BF5SA90HOYO2'),	# 3HA?
	('KindlePaperWhite6Unknown_3H5',			0xE25,	'A1BF5SA90HOYO2'),	# 3H5?
	('KindlePaperWhite6Unknown_3H3',			0xE23,	'A1BF5SA90HOYO2'),	# 3H3?
	('KindlePaperWhite6Unknown_3H8',			0xE28,	'A1BF5SA90HOYO2'),	# 3H8?
	('KindlePaperWhite6Unknown_3J5',			0xE45,	'A1BF5SA90HOYO2'),	# 3J5?
	('KindlePaperWhite6Unknown_3JS',			0xE5A,	'A1BF5SA90HOYO2'),	# 3JS?
	('KindleScribe2Unknown_3V0',				0xFA0,	'A3TY6T3X94EBV6'),  # 3V0?
	('KindleScribe2Unknown_3V1',				0xFA1,	'A3TY6T3X94EBV6'),  # 3V1?
	('KindleScribe2Unknown_3X5',				0xFE5,	'A3TY6T3X94EBV6'),  # 3X5?
	('KindleScribe2Unknown_3UV',				0xF9D,	'A3TY6T3X94EBV6'),  # 3UV?
	('KindleScribe2Unknown_3X4',				0xFE4,	'A3TY6T3X94EBV6'),  # 3X4?
	('KindleScribe2Unknown_3X3',				0xFE3,	'A3TY6T3X94EBV6'),  # 3X3?
	('KindleScribe2Unknown_41E',				0x102E,	'A3TY6T3X94EBV6'),  # 41E?
	('KindleScribe2Unknown_41D',				0x102D,	'A3TY6T3X94EBV6'),  # 41D?
	('KindleColorSoftUnknown_3H9',				0xE29,	'A2CU9ZQDNZFID4'),	# 3H9?
	('KindleColorSoftUnknown_3H4',				0xE24,	'A2CU9ZQDNZFID4'),	# 3H4?
	('KindleColorSoftUnknown_3HB',				0xE2B,	'A2CU9ZQDNZFID4'),	# 3HB?
	('KindleColorSoftUnknown_3H6',				0xE26,	'A2CU9ZQDNZFID4'),	# 3H6?
	('KindleColorSoftUnknown_3H2',				0xE22,	'A2CU9ZQDNZFID4'),	# 3H2?
	('KindleColorSoftUnknown_34X',				0xC9F,	'A2CU9ZQDNZFID4'),	# 34X?
	('KindleColorSoftUnknown_3H7',				0xE27,	'A2CU9ZQDNZFID4'),	# 3H7
	('KindleColorSoftUnknown_3JT',				0xE5B,	'A2CU9ZQDNZFID4'),	# 3JT?
	('KindleColorSoftUnknown_3J6',				0xE46,	'A2CU9ZQDNZFID4'),	# 3J6?
	('KindleColorSoftUnknown_456',				0x10A6,	'A2CU9ZQDNZFID4'),	# 456?
	('KindleUnknown', 0x00)
]

# We need the ID of a few very specific cutoff models...
wario_cutoff_id = 0
for i, v in enumerate(model_tuples):
	if v[0] == 'KindleVoyageWiFi3GJapan':
		wario_cutoff_id = v[1]


print('Kindle models sorted by device code\n')
for t in sorted(model_tuples, key=itemgetter(1)):
	# Handle the base32hex device IDs in a dedicated manner...
	if t[1] > 0xFF:
		print("{:<45} 0x{:03X} ({:0>3}) {:4} {:<14}".format(t[0], t[1], baseN(t[1], 32), '', t[2] if len(t) == 3 else ''))
	else:
		print("{:<45} 0x{:02X} {:11} {:<14}".format(t[0], t[1], '', t[2] if len(t) == 3 else ''))

print('\nKindle models >= KindleVoyageWiFi3GJapan (i.e., Platform >= Wario)\n')
for t in model_tuples:
	if t[1] >= wario_cutoff_id:
		if t[1] > 0xFF:
			print("{:<45} 0x{:03X} ({:0>3})".format(t[0], t[1], baseN(t[1], 32)))
		else:
			print("{:<45} 0x{:02X}".format(t[0], t[1]))
#	# That's to double-check that everything's sane for KindleTool's info command...
#	else:
#		print("!!{:<44}!!".format(t[0]))

print('\nKindle models with new device code decoding (i.e., >= PW3)\n')
for t in model_tuples:
	if t[1] >= wario_cutoff_id:
		if t[1] > 0xFF:
			print("{:<45} 0x{:03X} ({:0>3} <-> 0x{:03X})".format(t[0], t[1], baseN(t[1], 32), devCode(baseN(t[1], 32))))
