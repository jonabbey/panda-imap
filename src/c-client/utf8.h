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
 * Last Edited:	26 June 1998
 *
 * Copyright 1998 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made
 * available "as is", and
 * THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
 * NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
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
				/* Baltic */
#define I2CS_ISO8859_13 (I2CS_96 | 0x59)
				/* Vietnamese */
#define I2CS_VSCII (I2CS_96 | 0x5a)
				/* Euro (6/2 may be incorrect) */
#define I2CS_ISO8859_15 (I2CS_96 | 0x62)

/* Miscellaneous definitions */

#define EUC_CS2 0x8e		/* single shift CS2 */
#define EUC_CS3 0x8f		/* single shift CS3 */

#define BITS7 0x7f		/* 7-bit value mask */
#define BIT8 0x80		/* 8th bit mask */

				/* UCS2 codepoints */
#define UCS2_POUNDSTERLING 0x00a3
#define UCS2_YEN 0x00a5
#define UCS2_OVERLINE 0x203e
#define UCS2_KATAKANA 0xff61
#define BOGON 0xfffd

				/* hankaku katakana parameters */
#define MIN_KANA_7 0x21
#define MAX_KANA_7 0x5f
#define KANA_7 (UCS2_KATAKANA - MIN_KANA_7)
#define MIN_KANA_8 (MIN_KANA_7 | BIT8)
#define MAX_KANA_8 (MAX_KANA_7 | BIT8)
#define KANA_8 (UCS2_KATAKANA - MIN_KANA_8)


/* Character set table support */

typedef void (*cstext_t) (SIZEDTEXT *text,SIZEDTEXT *ret,void *tab);

struct utf8_csent {
  char *name;			/* character set name */
  cstext_t dsp;			/* text conversion dispatch */
  void *tab;			/* optional additional data */
};


struct utf8_eucparam {
  unsigned int base_ku : 8;	/* base row */
  unsigned int base_ten : 8;	/* base column */
  unsigned int max_ku : 8;	/* maximum row */
  unsigned int max_ten : 8;	/* maximum column */
  void *tab;			/* conversion table */
};

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
void utf8_searchpgm (SEARCHPGM *pgm,char *charset);
void utf8_stringlist (STRINGLIST *st,char *charset);
long utf8_mime2text (SIZEDTEXT *src,SIZEDTEXT *dst);
unsigned char *mime2_token (unsigned char *s,unsigned char *se,
			    unsigned char **t);
unsigned char *mime2_text (unsigned char *s,unsigned char *se,
			   unsigned char **t);
long mime2_decode (unsigned char *e,unsigned char *t,unsigned char *te,
		   SIZEDTEXT *txt);
