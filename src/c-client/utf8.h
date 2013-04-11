/*
 * Program:	UTF-8 routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	11 June 1997
 * Last Edited:	10 January 2001
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2001 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

/* UTF-8 size and conversion routines from UCS-2 values.  This will need to
 * be changed if UTF-16 data (surrogate pairs) are ever an issue.
 */

#define UTF8_SIZE(c) ((c & 0xff80) ? ((c & 0xf800) ? 3 : 2) : 1)

#define UTF8_PUT(b,c) {					\
  if (c & 0xff80) {		/* non-ASCII? */	\
    if (c & 0xf800) {		/* three byte code */	\
      *b++ = 0xe0 | (c >> 12);				\
      *b++ = 0x80 | ((c >> 6) & 0x3f);			\
    }							\
    else *b++ = 0xc0 | ((c >> 6) & 0x3f);		\
    *b++ = 0x80 | (c & 0x3f); 				\
  }							\
  else *b++ = c;					\
}

/* ISO-2022 engine states */

#define I2S_CHAR 0		/* character */
#define I2S_ESC 1		/* previous character was ESC */
#define I2S_MUL 2		/* previous character was multi-byte code */
#define I2S_INT 3		/* previous character was intermediate */


/* ISO-2022 Gn selections */

#define I2C_G0 0		/* G0 */
#define I2C_G1 1		/* G1 */
#define I2C_G2 2		/* G2 */
#define I2C_G3 3		/* G3 */
#define I2C_SG2 (2 << 2)	/* single shift G2 */
#define I2C_SG3 (3 << 2)	/* single shift G2 */


/* ISO-2022 octet definitions */

#define I2C_ESC 0x1b		/* ESCape */

	/* Intermediate character */
#define I2C_STRUCTURE 0x20	/* announce code structure */
#define I2C_C0 0x21		/* C0 */
#define I2C_C1 0x22		/* C1 */
#define I2C_CONTROL 0x23	/* single control function */
#define I2C_MULTI 0x24		/* multi-byte character set */
#define I2C_OTHER 0x25		/* other coding system */
#define I2C_REVISED 0x26	/* revised registration */
#define I2C_G0_94 0x28		/* G0 94-character set */
#define I2C_G1_94 0x29		/* G1 94-character set */
#define I2C_G2_94 0x2A		/* G2 94-character set */
#define I2C_G3_94 0x2B		/* G3 94-character set */
#define I2C_G0_96 0x2C		/* (not in ISO-2022) G0 96-character set */
#define I2C_G1_96 0x2D		/* G1 96-character set */
#define I2C_G2_96 0x2E		/* G2 96-character set */
#define I2C_G3_96 0x2F		/* G3 96-character set */

	/* Locking shifts */
#define I2C_SI 0x0f		/* lock shift to G0 (Shift In) */
#define I2C_SO 0x0e		/* lock shift to G1 (Shift Out) */
	/* prefixed by ESC */
#define I2C_LS2 0x6e		/* lock shift to G2 */
#define I2C_LS3 0x6f		/* lock shift to G3 */
#define I2C_LS1R 0x7e		/* lock shift GR to G1 */
#define I2C_LS2R 0x7d		/* lock shift GR to G2 */
#define I2C_LS3R 0x7c		/* lock shift GR to G3 */

	/* Single shifts */
#define I2C_SS2_ALT 0x8e	/* single shift to G2 (SS2) */
#define I2C_SS3_ALT 0x8f	/* single shift to G3 (SS3) */
#define I2C_SS2_ALT_7 0x19	/* single shift to G2 (SS2) */
#define I2C_SS3_ALT_7 0x1d	/* single shift to G3 (SS3) */
	/* prefixed by ESC */
#define I2C_SS2 0x4e		/* single shift to G2 (SS2) */
#define I2C_SS3 0x4f		/* single shift to G3 (SS3) */

/* Types of character sets */

#define I2CS_94 0x000		/* 94 character set */
#define I2CS_96 0x100		/* 96 character set */
#define I2CS_MUL 0x200		/* multi-byte */
#define I2CS_94x94 (I2CS_MUL | I2CS_94)
#define I2CS_96x96 (I2CS_MUL | I2CS_96)


/* 94 character sets */

				/* British localized ASCII */
#define I2CS_BRITISH (I2CS_94 | 0x41)
				/* ASCII */
#define I2CS_ASCII (I2CS_94 | 0x42)
				/* some buggy software does this */
#define I2CS_JIS_BUGROM (I2CS_94 | 0x48)
				/* JIS X 0201-1976 right half */
#define I2CS_JIS_KANA (I2CS_94 | 0x49)
				/* JIS X 0201-1976 left half */
#define I2CS_JIS_ROMAN (I2CS_94 | 0x4a)
				/* JIS X 0208-1978 */
#define I2CS_JIS_OLD (I2CS_94x94 | 0x40)
				/* GB 2312 */
#define I2CS_GB (I2CS_94x94 | 0x41)
				/* JIS X 0208-1983 */
#define I2CS_JIS_NEW (I2CS_94x94 | 0x42)
				/* KSC 5601 */
#define I2CS_KSC (I2CS_94x94 | 0x43)
				/* JIS X 0212-1990 */
#define I2CS_JIS_EXT (I2CS_94x94 | 0x44)
				/* CNS 11643 plane 1 */
#define I2CS_CNS1 (I2CS_94x94 | 0x47)
				/* CNS 11643 plane 2 */
#define I2CS_CNS2 (I2CS_94x94 | 0x48)
				/* CNS 11643 plane 3 */
#define I2CS_CNS3 (I2CS_94x94 | 0x49)
				/* CNS 11643 plane 4 */
#define I2CS_CNS4 (I2CS_94x94 | 0x4a)
				/* CNS 11643 plane 5 */
#define I2CS_CNS5 (I2CS_94x94 | 0x4b)
				/* CNS 11643 plane 6 */
#define I2CS_CNS6 (I2CS_94x94 | 0x4c)
				/* CNS 11643 plane 7 */
#define I2CS_CNS7 (I2CS_94x94 | 0x4d)

/* 96 character sets */
				/* Latin-1 (Western Europe) */
#define I2CS_ISO8859_1 (I2CS_96 | 0x41)
				/* Latin-2 (Czech, Slovak) */
#define I2CS_ISO8859_2 (I2CS_96 | 0x42)
				/* Latin-3 (Dutch, Turkish) */
#define I2CS_ISO8859_3 (I2CS_96 | 0x43)
				/* Latin-4 (Scandinavian) */
#define I2CS_ISO8859_4 (I2CS_96 | 0x44)
				/* Greek */
#define I2CS_ISO8859_7 (I2CS_96 | 0x46)
				/* Arabic */
#define I2CS_ISO8859_6 (I2CS_96 | 0x47)
				/* Hebrew */
#define I2CS_ISO8859_8 (I2CS_96 | 0x48)
				/* Cyrillic */
#define I2CS_ISO8859_5 (I2CS_96 | 0x4c)
				/* Latin-5 (Finnish, Portuguese) */
#define I2CS_ISO8859_9 (I2CS_96 | 0x4d)
				/* TIS 620 */
#define I2CS_TIS620 (I2CS_96 | 0x54)
				/* Latin-6 (Northern Europe) */
#define I2CS_ISO8859_10 (I2CS_96 | 0x56)
				/* Latin-7 (Baltic) */
#define I2CS_ISO8859_13 (I2CS_96 | 0x59)
				/* Vietnamese */
#define I2CS_VSCII (I2CS_96 | 0x5a)
				/* Latin-8 (Celtic) */
#define I2CS_ISO8859_14 (I2CS_96 | 0x5c)
				/* Euro (6/2 may be incorrect) */
#define I2CS_ISO8859_15 (I2CS_96 | 0x62)

/* Miscellaneous ISO 2022 definitions */

#define EUC_CS2 0x8e		/* single shift CS2 */
#define EUC_CS3 0x8f		/* single shift CS3 */

#define BITS7 0x7f		/* 7-bit value mask */
#define BIT8 0x80		/* 8th bit mask */

				/* UCS2 codepoints */
#define UCS2_POUNDSTERLING 0x00a3
#define UCS2_YEN 0x00a5
#define UCS2_OVERLINE 0x203e
#define UCS2_KATAKANA 0xff61
#define UBOGON 0xfffd		/* replacement character */

				/* hankaku katakana parameters */
#define MIN_KANA_7 0x21
#define MAX_KANA_7 0x5f
#define KANA_7 (UCS2_KATAKANA - MIN_KANA_7)
#define MIN_KANA_8 (MIN_KANA_7 | BIT8)
#define MAX_KANA_8 (MAX_KANA_7 | BIT8)
#define KANA_8 (UCS2_KATAKANA - MIN_KANA_8)

/* Charset scripts */

/*  The term "script" is used here in a very loose sense, enough to make
 * purists cringe.  Basically, the idea is to give the main program some
 * idea of how it should treat the characters of text in a charset with
 * respect to font, drawing routines, etc.
 *
 *  In some cases, "script" is associated with a charset; in other cases,
 * it's more closely tied to a language.
 */

#define SC_UNICODE 0x1		/* UNICODE */
	/* ISO 8859 scripts */
#define SC_LATIN_1 0x10		/* Western Europe */
#define SC_LATIN_2 0x20		/* Eastern Europe */
#define SC_LATIN_3 0x40		/* Southern Europe */
#define SC_LATIN_4 0x80		/* Northern Europe */
#define SC_LATIN_5 0x100	/* Turkish */
#define SC_LATIN_6 0x200	/* Nordic */
#define SC_LATIN_7 0x400	/* Baltic */
#define SC_LATIN_8 0x800	/* Celtic */
#define SC_LATIN_9 0x1000	/* Euro */
#define SC_LATIN_0 SC_LATIN_9	/* colloquial name for Latin-9 */
#define SC_ARABIC 0x2000
#define SC_CYRILLIC 0x4000
#define SC_GREEK 0x8000
#define SC_HEBREW 0x10000
#define SC_THAI 0x20000
#define SC_UKRANIAN 0x40000
	/* East Asian scripts */
#define SC_CHINESE_SIMPLIFIED 0x100000
#define SC_CHINESE_TRADITIONAL 0x200000
#define SC_JAPANESE 0x400000
#define SC_KOREAN 0x800000
#define SC_VIETNAMESE 0x1000000

/* Character set table support */

typedef void (*cstext_t) (SIZEDTEXT *text,SIZEDTEXT *ret,void *tab);

struct utf8_csent {
  char *name;			/* charset name */
  cstext_t dsp;			/* text conversion dispatch */
  void *tab;			/* optional additional data */
  unsigned long script;		/* script(s) implemented by this charset */
  char *preferred;		/* preferred charset over this one */
};


struct utf8_eucparam {
  unsigned int base_ku : 8;	/* base row */
  unsigned int base_ten : 8;	/* base column */
  unsigned int max_ku : 8;	/* maximum row */
  unsigned int max_ten : 8;	/* maximum column */
  void *tab;			/* conversion table */
};


/* UTF-7 engine states */

#define U7_ASCII 0		/* ASCII character */
#define U7_PLUS 1		/* plus seen */
#define U7_UNICODE 2		/* Unicode characters */
#define U7_MINUS 3		/* absorbed minus seen */

/* Function prototypes */

long utf8_text (SIZEDTEXT *text,char *charset,SIZEDTEXT *ret,long flags);
void utf8_text_8859_1 (SIZEDTEXT *text,SIZEDTEXT *ret,void *tab);
void utf8_text_1byte (SIZEDTEXT *text,SIZEDTEXT *ret,void *tab);
void utf8_text_1byte8 (SIZEDTEXT *text,SIZEDTEXT *ret,void *tab);
void utf8_text_euc (SIZEDTEXT *text,SIZEDTEXT *ret,void *tab);
void utf8_text_dbyte (SIZEDTEXT *text,SIZEDTEXT *ret,void *tab);
void utf8_text_dbyte2 (SIZEDTEXT *text,SIZEDTEXT *ret,void *tab);
void utf8_text_sjis (SIZEDTEXT *text,SIZEDTEXT *ret,void *tab);
void utf8_text_2022 (SIZEDTEXT *text,SIZEDTEXT *ret,void *tab);
void utf8_text_utf7 (SIZEDTEXT *text,SIZEDTEXT *ret,void *tab);
void utf8_searchpgm (SEARCHPGM *pgm,char *charset);
void utf8_stringlist (STRINGLIST *st,char *charset);
long utf8_mime2text (SIZEDTEXT *src,SIZEDTEXT *dst);
unsigned char *mime2_token (unsigned char *s,unsigned char *se,
			    unsigned char **t);
unsigned char *mime2_text (unsigned char *s,unsigned char *se,
			   unsigned char **t);
long mime2_decode (unsigned char *e,unsigned char *t,unsigned char *te,
		   SIZEDTEXT *txt);
