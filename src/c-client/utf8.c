/* ========================================================================
 * Copyright 1988-2008 University of Washington
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * 
 * ========================================================================
 */

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
 * Last Edited:	17 January 2008
 */


#include <stdio.h>
#include <ctype.h>
#include "c-client.h"

/*	*** IMPORTANT ***
 *
 *  There is a very important difference between "character set" and "charset",
 * and the comments in this file reflect these differences.  A "character set"
 * (also known as "coded character set") is a mapping between codepoints and
 * characters.  A "charset" is as defined in MIME, and incorporates one or more
 * coded character sets in a character encoding scheme.  See RFC 2130 for more
 * details.
 */


/* Character set conversion tables */

#include "iso_8859.c"		/* 8-bit single-byte coded graphic */
#include "koi8_r.c"		/* Cyrillic - Russia */
#include "koi8_u.c"		/* Cyrillic - Ukraine */
#include "tis_620.c"		/* Thai */
#include "viscii.c"		/* Vietnamese */
#include "windows.c"		/* Windows */
#include "ibm.c"		/* IBM */
#include "gb_2312.c"		/* Chinese (PRC) - simplified */
#include "gb_12345.c"		/* Chinese (PRC) - traditional */
#include "jis_0208.c"		/* Japanese - basic */
#include "jis_0212.c"		/* Japanese - supplementary */
#include "ksc_5601.c"		/* Korean */
#include "big5.c"		/* Taiwanese (ROC) - industrial standard */
#include "cns11643.c"		/* Taiwanese (ROC) - national standard */


#include "widths.c"		/* Unicode character widths */
#include "tmap.c"		/* Unicode titlecase mapping */
#include "decomtab.c"		/* Unicode decomposions */

/* EUC parameters */

#ifdef GBTOUNICODE		/* PRC simplified Chinese */
static const struct utf8_eucparam gb_param = {
  BASE_GB2312_KU,BASE_GB2312_TEN,MAX_GB2312_KU,MAX_GB2312_TEN,
  (void *) gb2312tab};
#endif


#ifdef GB12345TOUNICODE		/* PRC traditional Chinese */
static const struct utf8_eucparam gbt_param = {
  BASE_GB12345_KU,BASE_GB12345_TEN,MAX_GB12345_KU,MAX_GB12345_TEN,
  (void *) gb12345tab};
#endif


#ifdef BIG5TOUNICODE		/* ROC traditional Chinese */
static const struct utf8_eucparam big5_param[] = {
  {BASE_BIG5_KU,BASE_BIG5_TEN_0,MAX_BIG5_KU,MAX_BIG5_TEN_0,(void *) big5tab},
  {BASE_BIG5_KU,BASE_BIG5_TEN_1,MAX_BIG5_KU,MAX_BIG5_TEN_1,NIL}
};
#endif


#ifdef JISTOUNICODE		/* Japanese */
static const struct utf8_eucparam jis_param[] = {
  {BASE_JIS0208_KU,BASE_JIS0208_TEN,MAX_JIS0208_KU,MAX_JIS0208_TEN,
     (void *) jis0208tab},
  {MIN_KANA_8,0,MAX_KANA_8,0,(void *) KANA_8},
#ifdef JIS0212TOUNICODE		/* Japanese extended */
  {BASE_JIS0212_KU,BASE_JIS0212_TEN,MAX_JIS0212_KU,MAX_JIS0212_TEN,
     (void *) jis0212tab}
#else
  {0,0,0,0,NIL}
#endif
};
#endif


#ifdef KSCTOUNICODE		/* Korean */
static const struct utf8_eucparam ksc_param = {
  BASE_KSC5601_KU,BASE_KSC5601_TEN,MAX_KSC5601_KU,MAX_KSC5601_TEN,
  (void *) ksc5601tab};
#endif

/* List of supported charsets */

static const CHARSET utf8_csvalid[] = {
  {"US-ASCII",CT_ASCII,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   NIL,NIL,NIL},
  {"UTF-8",CT_UTF8,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   NIL,SC_UNICODE,NIL},
  {"UTF-7",CT_UTF7,CF_PRIMARY | CF_POSTING | CF_UNSUPRT,
   NIL,SC_UNICODE,"UTF-8"},
  {"ISO-8859-1",CT_1BYTE0,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   NIL,SC_LATIN_1,NIL},
  {"ISO-8859-2",CT_1BYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   (void *) iso8859_2tab,SC_LATIN_2,NIL},
  {"ISO-8859-3",CT_1BYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   (void *) iso8859_3tab,SC_LATIN_3,NIL},
  {"ISO-8859-4",CT_1BYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   (void *) iso8859_4tab,SC_LATIN_4,NIL},
  {"ISO-8859-5",CT_1BYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   (void *) iso8859_5tab,SC_CYRILLIC,"KOI8-R"},
  {"ISO-8859-6",CT_1BYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   (void *) iso8859_6tab,SC_ARABIC,NIL},
  {"ISO-8859-7",CT_1BYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   (void *) iso8859_7tab,SC_GREEK,NIL},
  {"ISO-8859-8",CT_1BYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   (void *) iso8859_8tab,SC_HEBREW,NIL},
  {"ISO-8859-9",CT_1BYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   (void *) iso8859_9tab,SC_LATIN_5,NIL},
  {"ISO-8859-10",CT_1BYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   (void *) iso8859_10tab,SC_LATIN_6,NIL},
  {"ISO-8859-11",CT_1BYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   (void *) iso8859_11tab,SC_THAI,NIL},
#if 0				/* ISO 8859-12 reserved for ISCII(?) */
  {"ISO-8859-12",CT_1BYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   (void *) iso8859_12tab,NIL,NIL},
#endif
  {"ISO-8859-13",CT_1BYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   (void *) iso8859_13tab,SC_LATIN_7,NIL},
  {"ISO-8859-14",CT_1BYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   (void *) iso8859_14tab,SC_LATIN_8,NIL},
  {"ISO-8859-15",CT_1BYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   (void *) iso8859_15tab,SC_LATIN_9,NIL},
  {"ISO-8859-16",CT_1BYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   (void *) iso8859_16tab,SC_LATIN_10,NIL},
  {"KOI8-R",CT_1BYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   (void *) koi8rtab,SC_CYRILLIC,NIL},
  {"KOI8-U",CT_1BYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   (void *) koi8utab,SC_CYRILLIC | SC_UKRANIAN,NIL},
  {"KOI8-RU",CT_1BYTE,CF_DISPLAY,
   (void *) koi8utab,SC_CYRILLIC | SC_UKRANIAN,"KOI8-U"},
  {"TIS-620",CT_1BYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   (void *) tis620tab,SC_THAI,"ISO-8859-11"},
  {"VISCII",CT_1BYTE8,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   (void *) visciitab,SC_VIETNAMESE,NIL},

#ifdef GBTOUNICODE
  {"GBK",CT_DBYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
     (void *) &gb_param,SC_CHINESE_SIMPLIFIED,NIL},
  {"GB2312",CT_DBYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
   (void *) &gb_param,SC_CHINESE_SIMPLIFIED,"GBK"},
  {"CN-GB",CT_DBYTE,CF_DISPLAY,
     (void *) &gb_param,SC_CHINESE_SIMPLIFIED,"GBK"},
#ifdef CNS1TOUNICODE
  {"ISO-2022-CN",CT_2022,CF_PRIMARY | CF_UNSUPRT,
     NIL,SC_CHINESE_SIMPLIFIED | SC_CHINESE_TRADITIONAL,
   NIL},
#endif
#endif
#ifdef GB12345TOUNICODE
  {"CN-GB-12345",CT_DBYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) &gbt_param,SC_CHINESE_TRADITIONAL,"BIG5"},
#endif
#ifdef BIG5TOUNICODE
  {"BIG5",CT_DBYTE2,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
     (void *) big5_param,SC_CHINESE_TRADITIONAL,NIL},
  {"CN-BIG5",CT_DBYTE2,CF_DISPLAY,
     (void *) big5_param,SC_CHINESE_TRADITIONAL,"BIG5"},
  {"BIG-5",CT_DBYTE2,CF_DISPLAY,
     (void *) big5_param,SC_CHINESE_TRADITIONAL,"BIG5"},
#endif
#ifdef JISTOUNICODE
  {"ISO-2022-JP",CT_2022,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
     NIL,SC_JAPANESE,NIL},
  {"EUC-JP",CT_EUC,CF_PRIMARY | CF_DISPLAY,
     (void *) jis_param,SC_JAPANESE,"ISO-2022-JP"},
  {"SHIFT_JIS",CT_SJIS,CF_PRIMARY | CF_DISPLAY,
     NIL,SC_JAPANESE,"ISO-2022-JP"},
  {"SHIFT-JIS",CT_SJIS,CF_PRIMARY | CF_DISPLAY,
     NIL,SC_JAPANESE,"ISO-2022-JP"},
#ifdef JIS0212TOUNICODE
  {"ISO-2022-JP-1",CT_2022,CF_UNSUPRT,
     NIL,SC_JAPANESE,"ISO-2022-JP"},
#ifdef GBTOUNICODE
#ifdef KSCTOUNICODE
  {"ISO-2022-JP-2",CT_2022,CF_UNSUPRT,
     NIL,
     SC_LATIN_1 | SC_LATIN_2 | SC_LATIN_3 | SC_LATIN_4 | SC_LATIN_5 |
       SC_LATIN_6 | SC_LATIN_7 | SC_LATIN_8 | SC_LATIN_9 | SC_LATIN_10 |
	 SC_ARABIC | SC_CYRILLIC | SC_GREEK | SC_HEBREW | SC_THAI |
	   SC_VIETNAMESE | SC_CHINESE_SIMPLIFIED | SC_JAPANESE | SC_KOREAN
#ifdef CNS1TOUNICODE
	     | SC_CHINESE_TRADITIONAL
#endif
	       ,"UTF-8"},
#endif
#endif
#endif
#endif

#ifdef KSCTOUNICODE
  {"ISO-2022-KR",CT_2022,CF_PRIMARY | CF_DISPLAY | CF_UNSUPRT,
     NIL,SC_KOREAN,"EUC-KR"},
  {"EUC-KR",CT_DBYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
     (void *) &ksc_param,SC_KOREAN,NIL},
  {"KSC5601",CT_DBYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) &ksc_param,SC_KOREAN,"EUC-KR"},
  {"KSC_5601",CT_DBYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) &ksc_param,SC_KOREAN,"EUC-KR"},
  {"KS_C_5601-1987",CT_DBYTE,CF_DISPLAY,
     (void *) &ksc_param,SC_KOREAN,"EUC-KR"},
  {"KS_C_5601-1989",CT_DBYTE,CF_DISPLAY,
     (void *) &ksc_param,SC_KOREAN,"EUC-KR"},
  {"KS_C_5601-1992",CT_DBYTE,CF_DISPLAY,
     (void *) &ksc_param,SC_KOREAN,"EUC-KR"},
  {"KS_C_5601-1997",CT_DBYTE,CF_DISPLAY,
     (void *) &ksc_param,SC_KOREAN,"EUC-KR"},
#endif

				/* deep sigh */
  {"WINDOWS-874",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) windows_874tab,SC_THAI,"ISO-8859-11"},
  {"CP874",CT_1BYTE,CF_DISPLAY,
     (void *) windows_874tab,SC_THAI,"ISO-8859-11"},
#ifdef GBTOUNICODE
  {"WINDOWS-936",CT_DBYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) &gb_param,SC_CHINESE_SIMPLIFIED,"GBK"},
  {"CP936",CT_DBYTE,CF_DISPLAY,
     (void *) &gb_param,SC_CHINESE_SIMPLIFIED,"GBK"},
#endif
#ifdef KSCTOUNICODE
  {"WINDOWS-949",CT_DBYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) &ksc_param,SC_KOREAN,"EUC-KR"},
  {"CP949",CT_DBYTE,CF_DISPLAY,
     (void *) &ksc_param,SC_KOREAN,"EUC-KR"},
  {"X-WINDOWS-949",CT_DBYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) &ksc_param,SC_KOREAN,"EUC-KR"},
#endif
  {"WINDOWS-1250",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) windows_1250tab,SC_LATIN_2,"ISO-8859-2"},
  {"CP1250",CT_1BYTE,CF_DISPLAY,
     (void *) windows_1250tab,SC_LATIN_2,"ISO-8859-2"},
  {"WINDOWS-1251",CT_1BYTE,CF_PRIMARY | CF_DISPLAY | CF_POSTING,
     (void *) windows_1251tab,SC_CYRILLIC,"KOI8-R"},
  {"CP1251",CT_1BYTE,CF_DISPLAY,
     (void *) windows_1251tab,SC_CYRILLIC,"KOI8-R"},
  {"WINDOWS-1252",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) windows_1252tab,SC_LATIN_1,"ISO-8859-1"},
  {"CP1252",CT_1BYTE,CF_DISPLAY,
     (void *) windows_1252tab,SC_LATIN_1,"ISO-8859-1"},
  {"WINDOWS-1253",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) windows_1253tab,SC_GREEK,"ISO-8859-7"},
  {"CP1253",CT_1BYTE,CF_DISPLAY,
     (void *) windows_1253tab,SC_GREEK,"ISO-8859-7"},
  {"WINDOWS-1254",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) windows_1254tab,SC_LATIN_5,"ISO-8859-9"},
  {"CP1254",CT_1BYTE,CF_DISPLAY,
     (void *) windows_1254tab,SC_LATIN_5,"ISO-8859-9"},
  {"WINDOWS-1255",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) windows_1255tab,SC_HEBREW,"ISO-8859-8"},
  {"CP1255",CT_1BYTE,CF_DISPLAY,
     (void *) windows_1255tab,SC_HEBREW,"ISO-8859-8"},
  {"WINDOWS-1256",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) windows_1256tab,SC_ARABIC,"ISO-8859-6"},
  {"CP1256",CT_1BYTE,CF_DISPLAY,
     (void *) windows_1256tab,SC_ARABIC,"ISO-8859-6"},
  {"WINDOWS-1257",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) windows_1257tab,SC_LATIN_7,"ISO-8859-13"},
  {"CP1257",CT_1BYTE,CF_DISPLAY,
     (void *) windows_1257tab,SC_LATIN_7,"ISO-8859-13"},
  {"WINDOWS-1258",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) windows_1258tab,SC_VIETNAMESE,"VISCII"},
  {"CP1258",CT_1BYTE,CF_DISPLAY,
     (void *) windows_1258tab,SC_VIETNAMESE,"VISCII"},

				/* deeper sigh */
  {"IBM367",CT_ASCII,CF_PRIMARY | CF_DISPLAY,
     NIL,NIL,"US-ASCII"},
  {"IBM437",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) ibm_437tab,SC_LATIN_1,"ISO-8859-1"},
  {"IBM737",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) ibm_737tab,SC_GREEK,"ISO-8859-7"},
  {"IBM775",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) ibm_775tab,SC_LATIN_7,"ISO-8859-13"},
  {"IBM850",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) ibm_850tab,SC_LATIN_1,"ISO-8859-1"},
  {"IBM852",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) ibm_852tab,SC_LATIN_2,"ISO-8859-2"},
  {"IBM855",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) ibm_855tab,SC_CYRILLIC,"ISO-8859-5"},
  {"IBM857",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) ibm_857tab,SC_LATIN_5,"ISO-8859-9"},
  {"IBM860",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) ibm_860tab,SC_LATIN_1,"ISO-8859-1"},
  {"IBM861",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) ibm_861tab,SC_LATIN_6,"ISO-8859-10"},
  {"IBM862",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) ibm_862tab,SC_HEBREW,"ISO-8859-8"},
  {"IBM863",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) ibm_863tab,SC_LATIN_1,"ISO-8859-1"},
  {"IBM864",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) ibm_864tab,SC_ARABIC,"ISO-8859-6"},
  {"IBM865",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) ibm_865tab,SC_LATIN_6,"ISO-8859-10"},
  {"IBM866",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) ibm_866tab,SC_CYRILLIC,"KOI8-R"},
  {"IBM869",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) ibm_869tab,SC_GREEK,"ISO-8859-7"},
  {"IBM874",CT_1BYTE,CF_PRIMARY | CF_DISPLAY,
     (void *) ibm_874tab,SC_THAI,"ISO-8859-11"},
				/* deepest sigh */
  {"ANSI_X3.4-1968",CT_ASCII,CF_DISPLAY,
     NIL,NIL,"US-ASCII"},
  {"UNICODE-1-1-UTF-7",CT_UTF7,CF_UNSUPRT,
     NIL,SC_UNICODE,"UTF-8"},
				/* these should never appear in email */
  {"UCS-2",CT_UCS2,CF_PRIMARY | CF_DISPLAY | CF_NOEMAIL,
     NIL,SC_UNICODE,"UTF-8"},
  {"UCS-4",CT_UCS4,CF_PRIMARY | CF_DISPLAY | CF_NOEMAIL,
     NIL,SC_UNICODE,"UTF-8"},
  {"UTF-16",CT_UTF16,CF_PRIMARY | CF_DISPLAY | CF_NOEMAIL,
     NIL,SC_UNICODE,"UTF-8"},
  NIL
};

/* Non-Unicode Script table */

static const SCRIPT utf8_scvalid[] = {
  {"Arabic",NIL,SC_ARABIC},
  {"Chinese Simplified","China, Singapore",SC_CHINESE_SIMPLIFIED},
  {"Chinese Traditional","Taiwan, Hong Kong, Macao",SC_CHINESE_TRADITIONAL},
  {"Cyrillic",NIL,SC_CYRILLIC},
  {"Cyrillic Ukranian",NIL,SC_UKRANIAN},
  {"Greek",NIL,SC_GREEK},
  {"Hebrew",NIL,SC_HEBREW},
  {"Japanese",NIL,SC_JAPANESE},
  {"Korean",NIL,SC_KOREAN},
  {"Latin-1","Western Europe",SC_LATIN_1},
  {"Latin-2","Eastern Europe",SC_LATIN_2},
  {"Latin-3","Southern Europe",SC_LATIN_3},
  {"Latin-4","Northern Europe",SC_LATIN_4},
  {"Latin-5","Turkish",SC_LATIN_5},
  {"Latin-6","Nordic",SC_LATIN_6},
  {"Latin-7","Baltic",SC_LATIN_7},
  {"Latin-8","Celtic",SC_LATIN_8},
  {"Latin-9","Euro",SC_LATIN_9},
  {"Latin-10","Balkan",SC_LATIN_10},
  {"Thai",NIL,SC_THAI},
  {"Vietnamese",NIL,SC_VIETNAMESE},
  NIL
};

/* Look up script name or return entire table
 * Accepts: script name or NIL
 * Returns: pointer to script table entry or NIL if unknown
 */

SCRIPT *utf8_script (char *script)
{
  unsigned long i;
  if (!script) return (SCRIPT *) &utf8_scvalid[0];
  else if (*script && (strlen (script) < 128))
    for (i = 0; utf8_scvalid[i].name; i++)
      if (!compare_cstring (script,utf8_scvalid[i].name))
	return (SCRIPT *) &utf8_scvalid[i];
  return NIL;			/* failed */
}


/* Look up charset name or return entire table
 * Accepts: charset name or NIL
 * Returns: charset table entry or NIL if unknown
 */

const CHARSET *utf8_charset (char *charset)
{
  unsigned long i;
  if (!charset) return (CHARSET *) &utf8_csvalid[0];
  else if (*charset && (strlen (charset) < 128))
    for (i = 0; utf8_csvalid[i].name; i++)
      if (!compare_cstring (charset,utf8_csvalid[i].name))
	return (CHARSET *) &utf8_csvalid[i];
  return NIL;			/* failed */
}

/* Validate charset and generate error message if invalid
 * Accepts: bad character set
 * Returns: NIL if good charset, else error message string
 */

#define BADCSS "[BADCHARSET ("
#define BADCSE ")] Unknown charset: "

char *utf8_badcharset (char *charset)
{
  char *msg = NIL;
  if (!utf8_charset (charset)) {
    char *s,*t;
    unsigned long i,j;
				/* calculate size of header, trailer, and bad
				 * charset plus charset names */
    for (i = 0, j = sizeof (BADCSS) + sizeof (BADCSE) + strlen (charset) - 2;
	 utf8_csvalid[i].name; i++)
      j += strlen (utf8_csvalid[i].name) + 1;
				/* not built right */
    if (!i) fatal ("No valid charsets!");
				/* header */
    for (s = msg = (char *) fs_get (j), t = BADCSS; *t; *s++ = *t++);
				/* each charset */
    for (i = 0; utf8_csvalid[i].name; *s++ = ' ', i++)
      for (t = utf8_csvalid[i].name; *t; *s++ = *t++);
				/* back over last space, trailer */
    for (t = BADCSE, --s; *t; *s++ = *t++);
				/* finally bogus charset */
    for (t = charset; *t; *s++ = *t++);
    *s++ = '\0';		/* finally tie off string */
    if (s != (msg + j)) fatal ("charset msg botch");
  }
  return msg;
}

/* Convert charset labelled sized text to UTF-8
 * Accepts: source sized text
 *	    charset
 *	    pointer to returned sized text if non-NIL
 *	    flags
 * Returns: T if successful, NIL if failure
 */

long utf8_text (SIZEDTEXT *text,char *charset,SIZEDTEXT *ret,long flags)
{
  ucs4cn_t cv = (flags & U8T_CASECANON) ? ucs4_titlecase : NIL;
  ucs4de_t de = (flags & U8T_DECOMPOSE) ? ucs4_decompose_recursive : NIL;
  const CHARSET *cs = (charset && *charset) ?
    utf8_charset (charset) : utf8_infercharset (text);
  if (cs) return (text && ret) ? utf8_text_cs (text,cs,ret,cv,de) : LONGT;
  if (ret) {			/* no conversion possible */
    ret->data = text->data;	/* so return source */
    ret->size = text->size;
  }
  return NIL;			/* failure */
}


/* Operations used in converting data */

#define UTF8_COUNT_BMP(count,c,cv,de) {		\
  void *more = NIL;				\
  if (cv) c = (*cv) (c);			\
  if (de) c = (*de) (c,&more);			\
  do count += UTF8_SIZE_BMP(c);			\
  while (more && (c = (*de) (U8G_ERROR,&more)));\
}

#define UTF8_WRITE_BMP(b,c,cv,de) {		\
  void *more = NIL;				\
  if (cv) c = (*cv) (c);			\
  if (de) c = (*de) (c,&more);			\
  do UTF8_PUT_BMP (b,c)				\
  while (more && (c = (*de) (U8G_ERROR,&more)));\
}

#define UTF8_COUNT(count,c,cv,de) {		\
  void *more = NIL;				\
  if (cv) c = (*cv) (c);			\
  if (de) c = (*de) (c,&more);			\
  do count += utf8_size (c);			\
  while (more && (c = (*de) (U8G_ERROR,&more)));\
}

#define UTF8_WRITE(b,c,cv,de) {			\
  void *more = NIL;				\
  if (cv) c = (*cv) (c);			\
  if (de) c = (*de) (c,&more);			\
  do b = utf8_put (b,c);			\
  while (more && (c = (*de) (U8G_ERROR,&more)));\
}

/* Convert sized text to UTF-8 given CHARSET block
 * Accepts: source sized text
 *	    CHARSET block
 *	    pointer to returned sized text 
 *	    canonicalization function
 *	    decomposition function
 * Returns: T if successful, NIL if failure
 */

long utf8_text_cs (SIZEDTEXT *text,const CHARSET *cs,SIZEDTEXT *ret,
		   ucs4cn_t cv,ucs4de_t de)
{
  ret->data = text->data;	/* default to source */
  ret->size = text->size;
  switch (cs->type) {		/* convert if type known */
  case CT_ASCII:		/* 7-bit ASCII no table */
  case CT_UTF8:			/* variable UTF-8 encoded Unicode no table */
    if (cv || de) utf8_text_utf8 (text,ret,cv,de);
    break;
  case CT_1BYTE0:		/* 1 byte no table */
    utf8_text_1byte0 (text,ret,cv,de);
    break;
  case CT_1BYTE:		/* 1 byte ASCII + table 0x80-0xff */
    utf8_text_1byte (text,ret,cs->tab,cv,de);
    break;
  case CT_1BYTE8:		/* 1 byte table 0x00 - 0xff */
    utf8_text_1byte8 (text,ret,cs->tab,cv,de);
    break;
  case CT_EUC:			/* 2 byte ASCII + utf8_eucparam base/CS2/CS3 */
    utf8_text_euc (text,ret,cs->tab,cv,de);
    break;
  case CT_DBYTE:		/* 2 byte ASCII + utf8_eucparam */
    utf8_text_dbyte (text,ret,cs->tab,cv,de);
    break;
  case CT_DBYTE2:		/* 2 byte ASCII + utf8_eucparam plane1/2 */
    utf8_text_dbyte2 (text,ret,cs->tab,cv,de);
    break;
  case CT_UTF7:			/* variable UTF-7 encoded Unicode no table */
    utf8_text_utf7 (text,ret,cv,de);
    break;
  case CT_UCS2:			/* 2 byte 16-bit Unicode no table */
    utf8_text_ucs2 (text,ret,cv,de);
    break;
  case CT_UCS4:			/* 4 byte 32-bit Unicode no table */
    utf8_text_ucs4 (text,ret,cv,de);
    break;
  case CT_UTF16:		/* variable UTF-16 encoded Unicode no table */
    utf8_text_utf16 (text,ret,cv,de);
    break;
  case CT_2022:			/* variable ISO-2022 encoded no table*/
    utf8_text_2022 (text,ret,cv,de);
    break;
  case CT_SJIS:			/* 2 byte Shift-JIS encoded JIS no table */
    utf8_text_sjis (text,ret,cv,de);
    break;
  default:			/* unknown character set type */
    return NIL;
  }
  return LONGT;			/* return success */
}

/* Reverse mapping routines
 *
 * These routines only support character sets, not all possible charsets.  In
 * particular, they do not support any Unicode encodings or ISO 2022.
 *
 * As a special dispensation, utf8_cstext() and utf8_cstocstext() support
 * support ISO-2022-JP if EUC-JP can be reverse mapped; and utf8_rmaptext()
 * will generated ISO-2022-JP using an EUC-JP rmap if flagged to do so.
 *
 * No attempt is made to map "equivalent" Unicode characters or Unicode
 * characters that have the same glyph; nor is there any attempt to handle
 * combining characters or otherwise do any stringprep.  Maybe later.
 */


/* Convert UTF-8 sized text to charset
 * Accepts: source sized text
 *	    destination charset
 *	    pointer to returned sized text
 *	    substitute character if not in cs, else NIL to return failure
 * Returns: T if successful, NIL if failure
 */


long utf8_cstext (SIZEDTEXT *text,char *charset,SIZEDTEXT *ret,
		  unsigned long errch)
{
  short iso2022jp = !compare_cstring (charset,"ISO-2022-JP");
  unsigned short *rmap = utf8_rmap (iso2022jp ? "EUC-JP" : charset);
  return rmap ? utf8_rmaptext (text,rmap,ret,errch,iso2022jp) : NIL;
}

/* Convert charset labelled sized text to another charset
 * Accepts: source sized text
 *	    source charset
 *	    pointer to returned sized text
 *	    destination charset
 *	    substitute character if not in dest cs, else NIL to return failure
 * Returns: T if successful, NIL if failure
 *
 * This routine has the same restricts as utf8_cstext().
 */

long utf8_cstocstext (SIZEDTEXT *src,char *sc,SIZEDTEXT *dst,char *dc,
		      unsigned long errch)
{
  SIZEDTEXT utf8;
  const CHARSET *scs,*dcs;
  unsigned short *rmap;
  long ret = NIL;
  long iso2022jp;
				/* lookup charsets and reverse map */
  if ((dc && (dcs = utf8_charset (dc))) &&
      (rmap = (iso2022jp = ((dcs->type == CT_2022) &&
			    !compare_cstring (dcs->name,"ISO-2022-JP"))) ?
       utf8_rmap ("EUC-JP") : utf8_rmap_cs (dcs)) &&
      (scs = (sc && *sc) ? utf8_charset (sc) : utf8_infercharset (src))) {
				/* init temporary buffer */
    memset (&utf8,NIL,sizeof (SIZEDTEXT));
				/* source cs equivalent to dest cs? */
    if ((scs->type == dcs->type) && (scs->tab == dcs->tab)) {
      dst->data = src->data;	/* yes, just copy pointers */
      dst->size = src->size;
      ret = LONGT;
    }
				/* otherwise do the conversion */
    else ret = (utf8_text_cs (src,scs,&utf8,NIL,NIL) &&
		utf8_rmaptext (&utf8,rmap,dst,errch,iso2022jp));
				/* flush temporary buffer */
    if (utf8.data && (utf8.data != src->data) && (utf8.data != dst->data))
      fs_give ((void **) &utf8.data);
  }
  return ret;
}

/* Cached rmap */

static const CHARSET *currmapcs = NIL;
static unsigned short *currmap = NIL;


/* Cache and return map for UTF-8 -> character set
 * Accepts: character set name
 * Returns: cached map if character set found, else NIL
 */

unsigned short *utf8_rmap (char *charset)
{
  return (currmapcs && !compare_cstring (charset,currmapcs->name)) ? currmap :
    utf8_rmap_cs (utf8_charset (charset));
}


/* Cache and return map for UTF-8 -> character set given CHARSET block
 * Accepts: CHARSET block
 * Returns: cached map if character set found, else NIL
 */

unsigned short *utf8_rmap_cs (const CHARSET *cs)
{
  unsigned short *ret = NIL;
  if (!cs);			/* have charset? */
  else if (cs == currmapcs) ret = currmap;
  else if (ret = utf8_rmap_gen (cs,currmap)) {
    currmapcs = cs;
    currmap = ret;
  }
  return ret;
}

/* Return map for UTF-8 -> character set given CHARSET block
 * Accepts: CHARSET block
 *	    old map to recycle
 * Returns: map if character set found, else NIL
 */

unsigned short *utf8_rmap_gen (const CHARSET *cs,unsigned short *oldmap)
{
  unsigned short u,*tab,*rmap;
  unsigned int i,m,ku,ten;
  struct utf8_eucparam *param,*p2;
  switch (cs->type) {		/* is a character set? */
  case CT_ASCII:		/* 7-bit ASCII no table */
  case CT_1BYTE0:		/* 1 byte no table */
  case CT_1BYTE:		/* 1 byte ASCII + table 0x80-0xff */
  case CT_1BYTE8:		/* 1 byte table 0x00 - 0xff */
  case CT_EUC:			/* 2 byte ASCII + utf8_eucparam base/CS2/CS3 */
  case CT_DBYTE:		/* 2 byte ASCII + utf8_eucparam */
  case CT_DBYTE2:		/* 2 byte ASCII + utf8_eucparam plane1/2 */
  case CT_SJIS:			/* 2 byte Shift-JIS */
    rmap = oldmap ? oldmap :	/* recycle old map if supplied else make new */
      (unsigned short *) fs_get (65536 * sizeof (unsigned short));
				/* initialize table for ASCII */
    for (i = 0; i < 128; i++) rmap[i] = (unsigned short) i;
				/* populate remainder of table with NOCHAR */
#define NOCHARBYTE (NOCHAR & 0xff)
#if NOCHAR - ((NOCHARBYTE << 8) | NOCHARBYTE)
    while (i < 65536) rmap[i++] = NOCHAR;
#else
    memset (rmap + 128,NOCHARBYTE,(65536 - 128) * sizeof (unsigned short));
#endif
    break;
  default:			/* unsupported charset type */
    rmap = NIL;			/* no map possible */
  }
  if (rmap) {			/* have a map? */
    switch (cs->type) {		/* additional reverse map actions */
    case CT_1BYTE0:		/* 1 byte no table */
      for (i = 128; i < 256; i++) rmap[i] = (unsigned short) i;
      break;
    case CT_1BYTE:		/* 1 byte ASCII + table 0x80-0xff */
      for (tab = (unsigned short *) cs->tab,i = 128; i < 256; i++)
	if (tab[i & BITS7] != UBOGON) rmap[tab[i & BITS7]] = (unsigned short)i;
      break;
    case CT_1BYTE8:		/* 1 byte table 0x00 - 0xff */
      for (tab = (unsigned short *) cs->tab,i = 0; i < 256; i++)
	if (tab[i] != UBOGON) rmap[tab[i]] = (unsigned short) i;
      break;
    case CT_EUC:		/* 2 byte ASCII + utf8_eucparam base/CS2/CS3 */
      for (param = (struct utf8_eucparam *) cs->tab,
	     tab = (unsigned short *) param->tab, ku = 0;
	   ku < param->max_ku; ku++)
	for (ten = 0; ten < param->max_ten; ten++)
	  if ((u = tab[(ku * param->max_ten) + ten]) != UBOGON)
	    rmap[u] = ((ku + param->base_ku) << 8) +
	      (ten + param->base_ten) + 0x8080;
      break;

    case CT_DBYTE:		/* 2 byte ASCII + utf8_eucparam */
      for (param = (struct utf8_eucparam *) cs->tab,
	     tab = (unsigned short *) param->tab, ku = 0;
	   ku < param->max_ku; ku++)
	for (ten = 0; ten < param->max_ten; ten++)
	  if ((u = tab[(ku * param->max_ten) + ten]) != UBOGON)
	    rmap[u] = ((ku + param->base_ku) << 8) + (ten + param->base_ten);
      break;
    case CT_DBYTE2:		/* 2 byte ASCII + utf8_eucparam plane1/2 */
      param = (struct utf8_eucparam *) cs->tab;
      p2 = param + 1;		/* plane 2 parameters */
				/* only ten parameters should differ */
      if ((param->base_ku != p2->base_ku) || (param->max_ku != p2->max_ku))
	fatal ("ku definition error for CT_DBYTE2 charset");
				/* total codepoints in each ku */
      m = param->max_ten + p2->max_ten;
      tab = (unsigned short *) param->tab;
      for (ku = 0; ku < param->max_ku; ku++) {
	for (ten = 0; ten < param->max_ten; ten++)
	  if ((u = tab[(ku * m) + ten]) != UBOGON)
	    rmap[u] = ((ku + param->base_ku) << 8) + (ten + param->base_ten);
	for (ten = 0; ten < p2->max_ten; ten++)
	  if ((u = tab[(ku * m) + param->max_ten + ten]) != UBOGON)
	    rmap[u] = ((ku + param->base_ku) << 8) + (ten + p2->base_ten);
      }
      break;
    case CT_SJIS:		/* 2 byte Shift-JIS */
      for (ku = 0; ku < MAX_JIS0208_KU; ku++)
	for (ten = 0; ten < MAX_JIS0208_TEN; ten++)
	  if ((u = jis0208tab[ku][ten]) != UBOGON) {
	    int sku = ku + BASE_JIS0208_KU;
	    int sten = ten + BASE_JIS0208_TEN;
	    rmap[u] = ((((sku + 1) >> 1) + ((sku < 95) ? 112 : 176)) << 8) +
	      sten + ((sku % 2) ? ((sten > 95) ? 32 : 31) : 126);
	  }
				/* JIS Roman */
      rmap[UCS2_YEN] = JISROMAN_YEN;
      rmap[UCS2_OVERLINE] = JISROMAN_OVERLINE;
				/* JIS hankaku katakana */
      for (u = 0; u < (MAX_KANA_8 - MIN_KANA_8); u++)
	rmap[UCS2_KATAKANA + u] = MIN_KANA_8 + u;
      break;
    }
				/* hack: map NBSP to SP if otherwise no map */
    if (rmap[0x00a0] == NOCHAR) rmap[0x00a0] = rmap[0x0020];
  }
  return rmap;			/* return map */
}

/* Convert UTF-8 sized text to charset using rmap
 * Accepts: source sized text
 *	    conversion rmap
 *	    pointer to returned sized text
 *	    substitute character if not in rmap, else NIL to return failure
 *	    ISO-2022-JP conversion flag
 * Returns T if successful, NIL if failure
 *
 * This routine doesn't try to convert to all possible charsets; in particular
 * it doesn't support other Unicode encodings or any ISO 2022 other than
 * ISO-2022-JP.
 */

long utf8_rmaptext (SIZEDTEXT *text,unsigned short *rmap,SIZEDTEXT *ret,
		    unsigned long errch,long iso2022jp)
{
  unsigned long i,u,c;
				/* get size of buffer */
  if (i = utf8_rmapsize (text,rmap,errch,iso2022jp)) {
    unsigned char *s = text->data;
    unsigned char *t = ret->data = (unsigned char *) fs_get (i);
    ret->size = i - 1;		/* number of octets in destination buffer */
				/* start non-zero ISO-2022-JP state at 1 */
    if (iso2022jp) iso2022jp = 1;
				/* convert string, ignore BOM */
    for (i = text->size; i;) if ((u = utf8_get (&s,&i)) != UCS2_BOM) {
				/* substitute error character for NOCHAR */
      if ((u & U8GM_NONBMP) || ((c = rmap[u]) == NOCHAR)) c = errch;
      switch (iso2022jp) {	/* depends upon ISO 2022 mode */
      case 0:			/* ISO 2022 not in effect */
				/* two-byte character */
	if (c > 0xff) *t++ = (unsigned char) (c >> 8);
				/* single-byte or low-byte of two-byte */
	*t++ = (unsigned char) (c & 0xff);
	break;
      case 1:			/* ISO 2022 Roman */
				/* <ch> */
	if (c < 0x80) *t++ = (unsigned char) c;
	else {			/* JIS character */
	  *t++ = I2C_ESC;	/* ESC $ B <hi> <lo> */
	  *t++ = I2C_MULTI;
	  *t++ = I2CS_94x94_JIS_NEW;
	  *t++ = (unsigned char) (c >> 8) & 0x7f;
	  *t++ = (unsigned char) c & 0x7f;
	  iso2022jp = 2;	/* shift to ISO 2022 JIS */
	}
	break;
      case 2:			/* ISO 2022 JIS */
	if (c > 0x7f) {		/* <hi> <lo> */
	  *t++ = (unsigned char) (c >> 8) & 0x7f;
	  *t++ = (unsigned char) c & 0x7f;
	}
	else {			/* ASCII character */
	  *t++ = I2C_ESC;	/* ESC ( J <ch> */
	  *t++ = I2C_G0_94;
	  *t++ = I2CS_94_JIS_ROMAN;
	  *t++ = (unsigned char) c;
	  iso2022jp = 1;	/* shift to ISO 2022 Roman */
	}
	break;
      }
    }
    if (iso2022jp == 2) {	/* ISO-2022-JP string must end in Roman */
      *t++ = I2C_ESC;		/* ESC ( J */
      *t++ = I2C_G0_94;
      *t++ = I2CS_94_JIS_ROMAN;
    }
    *t++ = NIL;			/* tie off returned data */
    return LONGT;		/* return success */
  }
  ret->data = NIL;
  ret->size = 0;
  return NIL;			/* failure */
}

/* Calculate size of convertsion of UTF-8 sized text to charset using rmap
 * Accepts: source sized text
 *	    conversion rmap
 *	    pointer to returned sized text
 *	    substitute character if not in rmap, else NIL to return failure
 *	    ISO-2022-JP conversion flag
 * Returns size+1 if successful, NIL if failure
 *
 * This routine doesn't try to handle to all possible charsets; in particular
 * it doesn't support other Unicode encodings or any ISO 2022 other than
 * ISO-2022-JP.
 */

unsigned long utf8_rmapsize (SIZEDTEXT *text,unsigned short *rmap,
			     unsigned long errch,long iso2022jp)
{
  unsigned long i,u,c;
  unsigned long ret = 1;	/* terminating NUL */
  unsigned char *s = text->data;
  if (iso2022jp) iso2022jp = 1;	/* start non-zero ISO-2022-JP state at 1 */
  for (i = text->size; i;) if ((u = utf8_get (&s,&i)) != UCS2_BOM) {
    if ((u & U8GM_NONBMP) || (((c = rmap[u]) == NOCHAR) && !(c = errch)))
      return NIL;		/* not in BMP, or NOCHAR and no err char */
    switch (iso2022jp) {	/* depends upon ISO 2022 mode */
    case 0:			/* ISO 2022 not in effect */
      ret += (c > 0xff) ? 2 : 1;
      break;
    case 1:			/* ISO 2022 Roman */
      if (c < 0x80) ret += 1;	/* <ch> */
      else {			/* JIS character */
	ret += 5;		/* ESC $ B <hi> <lo> */
	iso2022jp = 2;		/* shift to ISO 2022 JIS */
      }
      break;
    case 2:			/* ISO 2022 JIS */
      if (c > 0x7f) ret += 2;	/* <hi> <lo> */
      else {			/* ASCII character */
	ret += 4;		/* ESC ( J <ch> */
	iso2022jp = 1;		/* shift to ISO 2022 Roman */
      }
      break;
    }
  }
  if (iso2022jp == 2) {		/* ISO-2022-JP string must end in Roman */
    ret += 3;			/* ESC ( J */
    iso2022jp = 1;		/* reset state to Roman */
  }
  return ret;
}

/* Convert UCS-4 to charset using rmap
 * Accepts: source UCS-4 character(s)
 *	    numver of UCS-4 characters
 *	    conversion rmap
 *	    pointer to returned sized text
 *	    substitute character if not in rmap, else NIL to return failure
 * Returns T if successful, NIL if failure
 *
 * Currently only supports BMP characters, and does not support ISO-2022
 */

long ucs4_rmaptext (unsigned long *ucs4,unsigned long len,unsigned short *rmap,
		    SIZEDTEXT *ret,unsigned long errch)
{
  long size = ucs4_rmaplen (ucs4,len,rmap,errch);
  return (size >= 0) ?		/* build in newly-created buffer */
    ucs4_rmapbuf (ret->data = (unsigned char *) fs_get ((ret->size = size) +1),
		  ucs4,len,rmap,errch) : NIL;
}

/* Return size of UCS-4 string converted to other CS via rmap
 * Accepts: source UCS-4 character(s)
 *	    numver of UCS-4 characters
 *	    conversion rmap
 *	    substitute character if not in rmap, else NIL to return failure
 * Returns: length if success, negative if failure (no-convert)
 */

long ucs4_rmaplen (unsigned long *ucs4,unsigned long len,unsigned short *rmap,
		   unsigned long errch)
{
  long ret;
  unsigned long i,u,c;
				/* count non-BOM characters */
  for (ret = 0,i = 0; i < len; ++i) if ((u = ucs4[i]) != UCS2_BOM) {
    if ((u & U8GM_NONBMP) || (((c = rmap[u]) == NOCHAR) && !(c = errch)))
      return -1;		/* not in BMP, or NOCHAR and no err char? */
    ret += (c > 0xff) ? 2 : 1;
  }
  return ret;
}


/* Stuff buffer with UCS-4 string converted to other CS via rmap
 * Accepts: destination buffer
 *	    source UCS-4 character(s)
 *	    number of UCS-4 characters
 *	    conversion rmap
 *	    substitute character if not in rmap, else NIL to return failure
 * Returns: T, always
 */

long ucs4_rmapbuf (unsigned char *t,unsigned long *ucs4,unsigned long len,
		   unsigned short *rmap,unsigned long errch)
{
  unsigned long i,u,c;
				/* convert non-BOM characters */
  for (i = 0; i < len; ++i) if ((u = ucs4[i]) != UCS2_BOM) {
				/* substitute error character for NOCHAR */
    if ((u & U8GM_NONBMP) || ((c = rmap[u]) == NOCHAR)) c = errch;
				/* two-byte character? */
    if (c > 0xff) *t++ = (unsigned char) (c >> 8);
				/* single-byte or low-byte of two-byte */
    *t++ = (unsigned char) (c & 0xff);
  }
  *t++ = NIL;			/* tie off returned data */
  return LONGT;
}

/* Return UCS-4 Unicode character from UTF-8 string
 * Accepts: pointer to string
 *	    remaining octets in string
 * Returns: UCS-4 character with pointer and count updated
 *	    or error code with pointer and count unchanged
 */

unsigned long utf8_get (unsigned char **s,unsigned long *i)
{
  unsigned char *t = *s;
  unsigned long j = *i;
				/* decode raw UTF-8 string */
  unsigned long ret = utf8_get_raw (&t,&j);
  if (ret & U8G_ERROR);		/* invalid raw UTF-8 decoding? */
				/* no, is it surrogate? */
  else if ((ret >= UTF16_SURR) && (ret <= UTF16_MAXSURR)) ret = U8G_SURROGA;
				/* or in non-Unicode ISO 10646 space? */
  else if (ret > UCS4_MAXUNICODE) ret = U8G_NOTUNIC;
  else {
    *s = t;			/* all is well, update pointer */
    *i = j;			/* and counter */
  }
  return ret;			/* return value */
}

/* Return raw (including non-Unicode) UCS-4 character from UTF-8 string
 * Accepts: pointer to string
 *	    remaining octets in string
 * Returns: UCS-4 character with pointer and count updated
 *	    or error code with pointer and count unchanged
 */

unsigned long utf8_get_raw (unsigned char **s,unsigned long *i)
{
  unsigned char c,c1;
  unsigned char *t = *s;
  unsigned long j = *i;
  unsigned long ret = U8G_NOTUTF8;
  int more = 0;
  do {				/* make sure have source octets available */
    if (!j--) return more ? U8G_ENDSTRI : U8G_ENDSTRG;
				/* UTF-8 continuation? */
    else if (((c = *t++) > 0x7f) && (c < 0xc0)) {
				/* continuation when not in progress */
      if (!more) return U8G_BADCONT;
      --more;			/* found a continuation octet */
      ret <<= 6;		/* shift current value by 6 bits */
      ret |= c & 0x3f;		/* merge continuation octet */
    }
				/* incomplete UTF-8 character */
    else if (more) return U8G_INCMPLT;
    else {			/* start of sequence */
      c1 = j ? *t : 0xbf;	/* assume valid continuation if incomplete */
      if (c < 0x80) ret = c;	/* U+0000 - U+007f */
      else if (c < 0xc2);	/* c0 and c1 never valid */
      else if (c < 0xe0) {	/* U+0080 - U+07ff */
	if (c &= 0x1f) more = 1;
      }
      else if (c < 0xf0) {	/* U+0800 - U+ffff */
	if ((c &= 0x0f) || (c1 >= 0xa0)) more = 2;
      }
      else if (c < 0xf8) {	/* U+10000 - U+10ffff (and 110000 - 1fffff) */
	if ((c &= 0x07) || (c1 >= 0x90)) more = 3;
      }
      else if (c < 0xfc) {	/* ISO 10646 200000 - 3ffffff */
	if ((c &= 0x03) || (c1 >= 0x88)) more = 4;
      }
      else if (c < 0xfe) {	/* ISO 10646 4000000 - 7fffffff */
	if ((c &= 0x01) || (c1 >= 0x84)) more = 5;
      }
				/* fe and ff never valid */
      if (more) {		/* multi-octet, make sure more to come */
	if (!j) return U8G_ENDSTRI;
	ret = c;		/* continuation needed, save start bits */
      }
    }
  } while (more);
  if (!(ret & U8G_ERROR)) {	/* success return? */
    *s = t;			/* yes, update pointer */
    *i = j;			/* and counter */
  }
  return ret;			/* return value */
}

/* Return UCS-4 character from named charset string
 * Accepts: charset
 *	    pointer to string
 *	    remaining octets in string
 * Returns: UCS-4 character with pointer and count updated, negative if error
 *
 * Error codes are the same as utf8_get().
 */

unsigned long ucs4_cs_get (CHARSET *cs,unsigned char **s,unsigned long *i)
{
  unsigned char c,c1,ku,ten;
  unsigned long ret,d;
  unsigned char *t = *s;
  unsigned long j = *i;
  struct utf8_eucparam *p1,*p2,*p3;
  if (j--) c = *t++;		/* get first octet */
  else return U8G_ENDSTRG;	/* empty string */
  switch (cs->type) {		/* convert if type known */
  case CT_UTF8:			/* variable UTF-8 encoded Unicode no table */
    return utf8_get (s,i);
  case CT_ASCII:		/* 7-bit ASCII no table */
    if (c >= 0x80) return U8G_NOTUTF8;
  case CT_1BYTE0:		/* 1 byte no table */
    ret = c;			/* identity */
    break;
  case CT_1BYTE:		/* 1 byte ASCII + table 0x80-0xff */
    ret = (c > 0x80) ? ((unsigned short *) cs->tab)[c & BITS7] : c;
    break;
  case CT_1BYTE8:		/* 1 byte table 0x00 - 0xff */
    ret = ((unsigned short *) cs->tab)[c];
    break;

  case CT_EUC:			/* 2 byte ASCII + utf8_eucparam base/CS2/CS3 */
    if (c & BIT8) {
      p1 = (struct utf8_eucparam *) cs->tab;
      p2 = p1 + 1;
      p3 = p1 + 2;
      if (j--) c1 = *t++;	/* get second octet */
      else return U8G_ENDSTRI;
      if (!(c1 & BIT8)) return U8G_NOTUTF8;
      switch (c) {		/* check 8bit code set */
      case EUC_CS2:		/* CS2 */
	if (p2->base_ku) {	/* CS2 set up? */
	  if (p2->base_ten) {	/* yes, multibyte? */
	    if (j--) c = *t++;	/* get second octet */
	    else return U8G_ENDSTRI;
	    if ((c & BIT8) &&
		((ku = (c1 & BITS7) - p2->base_ku) < p2->max_ku) &&
		((ten = (c & BITS7) - p2->base_ten) < p2->max_ten)) {
	      ret = ((unsigned short *) p2->tab)[(ku*p2->max_ten) + ten];
	      break;
	    }
	  }
	  else if ((c1 >= p2->base_ku) && (c1 < p2->max_ku)) {
	    ret = c1 + ((unsigned long) p2->tab);
	    break;
	  }
	}
	return U8G_NOTUTF8;	/* CS2 not set up or bogus */
      case EUC_CS3:		/* CS3 */
	if (p3->base_ku) {	/* CS3 set up? */
	  if (p3->base_ten) {	/* yes, multibyte? */
	    if (j--) c = *t++;	/* get second octet */
	    else return U8G_ENDSTRI;
	    if ((c & BIT8) &&
		((ku = (c1 & BITS7) - p3->base_ku) < p3->max_ku) &&
		((ten = (c & BITS7) - p3->base_ten) < p3->max_ten)) {
	      ret = ((unsigned short *) p3->tab)[(ku*p3->max_ten) + ten];
	      break;
	    }
	  }
	  else if ((c1 >= p3->base_ku) && (c1 < p3->max_ku)) {
	    ret = c1 + ((unsigned long) p3->tab);
	    break;
	  }
	}
	return U8G_NOTUTF8;	/* CS3 not set up or bogus */
      default:
	if (((ku = (c & BITS7) - p1->base_ku) >= p1->max_ku) ||
	    ((ten = (c1 & BITS7) - p1->base_ten) >= p1->max_ten))
	  return U8G_NOTUTF8;
	ret = ((unsigned short *) p1->tab)[(ku*p1->max_ten) + ten];
		/* special hack for JIS X 0212: merge rows less than 10 */
	if ((ret == UBOGON) && ku && (ku < 10) && p3->tab && p3->base_ten)
	  ret = ((unsigned short *) p3->tab)
	    [((ku - (p3->base_ku - p1->base_ku))*p3->max_ten) + ten];
	break;
      }
    }
    else ret = c;		/* ASCII character */
    break;

  case CT_DBYTE:		/* 2 byte ASCII + utf8_eucparam */
    if (c & BIT8) {		/* double-byte character? */
      p1 = (struct utf8_eucparam *) cs->tab;
      if (j--) c1 = *t++;	/* get second octet */
      else return U8G_ENDSTRI;
      if (((ku = c - p1->base_ku) < p1->max_ku) &&
	  ((ten = c1 - p1->base_ten) < p1->max_ten))
	ret = ((unsigned short *) p1->tab)[(ku*p1->max_ten) + ten];
      else return U8G_NOTUTF8;
    }
    else ret = c;		/* ASCII character */
    break;
  case CT_DBYTE2:		/* 2 byte ASCII + utf8_eucparam plane1/2 */
    if (c & BIT8) {		/* double-byte character? */
      p1 = (struct utf8_eucparam *) cs->tab;
      p2 = p1 + 1;
      if (j--) c1 = *t++;	/* get second octet */
      else return U8G_ENDSTRI;
      if (c1 & BIT8) {		/* high vs. low plane */
	if ((ku = c - p2->base_ku) < p2->max_ku &&
	    ((ten = c1 - p2->base_ten) < p2->max_ten))
	  ret = ((unsigned short *) p1->tab)
	    [(ku*(p1->max_ten + p2->max_ten)) + p1->max_ten + ten];
	else return U8G_NOTUTF8;
      }
      else if ((ku = c - p1->base_ku) < p1->max_ku &&
	       ((ten = c1 - p1->base_ten) < p1->max_ten))
	  ret = ((unsigned short *) p1->tab)
	    [(ku*(p1->max_ten + p2->max_ten)) + ten];
      else return U8G_NOTUTF8;
    }
    else ret = c;		/* ASCII character */
    break;
  case CT_SJIS:			/* 2 byte Shift-JIS encoded JIS no table */
				/* compromise - do yen sign but not overline */
    if (!(c & BIT8)) ret = (c == JISROMAN_YEN) ? UCS2_YEN : c;
				/* half-width katakana? */
    else if ((c >= MIN_KANA_8) && (c < MAX_KANA_8)) ret = c + KANA_8;
    else {			/* Shift-JIS */
      if (j--) c1 = *t++;	/* get second octet */
      else return U8G_ENDSTRI;
      SJISTOJIS (c,c1);
      c = JISTOUNICODE (c,c1,ku,ten);
    }
    break;

  case CT_UCS2:			/* 2 byte 16-bit Unicode no table */
    ret = c << 8;
    if (j--) c = *t++;		/* get second octet */
    else return U8G_ENDSTRI;	/* empty string */
    ret |= c;
    break;
  case CT_UCS4:			/* 4 byte 32-bit Unicode no table */
    if (c & 0x80) return U8G_NOTUTF8;
    if (j < 3) return U8G_ENDSTRI;
    j -= 3;			/* count three octets */
    ret = c << 24;
    ret |= (*t++) << 16;
    ret |= (*t++) << 8;
    ret |= (*t++);
    break;
  case CT_UTF16:		/* variable UTF-16 encoded Unicode no table */
    ret = c << 8;
    if (j--) c = *t++;		/* get second octet */
    else return U8G_ENDSTRI;	/* empty string */
    ret |= c;
				/* surrogate? */
    if ((ret >= UTF16_SURR) && (ret <= UTF16_MAXSURR)) {
				/* invalid first surrogate */
      if ((ret > UTF16_SURRHEND) || (j < 2)) return U8G_NOTUTF8;
      j -= 2;			/* count two octets */
      d = (*t++) << 8;		/* first octet of second surrogate */
      d |= *t++;		/* second octet of second surrogate */
      if ((d < UTF16_SURRL) || (d > UTF16_SURRLEND)) return U8G_NOTUTF8;
      ret = UTF16_BASE + ((ret & UTF16_MASK) << UTF16_SHIFT) +
	(d & UTF16_MASK);
    }
    break;
  default:			/* unknown/unsupported character set type */
    return U8G_NOTUTF8;
  }
  *s = t;			/* update pointer and counter */
  *i = j;
  return ret;
}

/* Produce charset validity map for BMP
 * Accepts: list of charsets to map
 * Returns: validity map, indexed by BMP codepoint
 *
 * Bit 0x1 is the "not-CJK" character bit
 */

unsigned long *utf8_csvalidmap (char *charsets[])
{
  unsigned short u,*tab;
  unsigned int m,ku,ten;
  unsigned long i,csi,csb;
  struct utf8_eucparam *param,*p2;
  char *s;
  const CHARSET *cs;
  unsigned long *ret = (unsigned long *)
    fs_get (i = 0x10000 * sizeof (unsigned long));
  memset (ret,0,i);		/* zero the entire vector */
				/* mark all the non-CJK codepoints */
	/* U+0000 - U+2E7F non-CJK */
  for (i = 0; i < 0x2E7F; ++i) ret[i] = 0x1;
	/* U+2E80 - U+2EFF CJK Radicals Supplement
	 * U+2F00 - U+2FDF Kangxi Radicals
	 * U+2FE0 - U+2FEF unassigned
	 * U+2FF0 - U+2FFF Ideographic Description Characters
	 * U+3000 - U+303F CJK Symbols and Punctuation
	 * U+3040 - U+309F Hiragana
	 * U+30A0 - U+30FF Katakana
	 * U+3100 - U+312F BoPoMoFo
	 * U+3130 - U+318F Hangul Compatibility Jamo
	 * U+3190 - U+319F Kanbun
	 * U+31A0 - U+31BF BoPoMoFo Extended
	 * U+31C0 - U+31EF CJK Strokes
	 * U+31F0 - U+31FF Katakana Phonetic Extensions
	 * U+3200 - U+32FF Enclosed CJK Letters and Months
	 * U+3300 - U+33FF CJK Compatibility
	 * U+3400 - U+4DBF CJK Unified Ideographs Extension A
	 * U+4DC0 - U+4DFF Yijing Hexagram Symbols
	 * U+4E00 - U+9FFF CJK Unified Ideographs
	 * U+A000 - U+A48F Yi Syllables
	 * U+A490 - U+A4CF Yi Radicals
	 * U+A700 - U+A71F Modifier Tone Letters
	 */
  for (i = 0xa720; i < 0xabff; ++i) ret[i] = 0x1;
	/* U+AC00 - U+D7FF Hangul Syllables */
  for (i = 0xd800; i < 0xf8ff; ++i) ret[i] = 0x1;
	/* U+F900 - U+FAFF CJK Compatibility Ideographs */
  for (i = 0xfb00; i < 0xfe2f; ++i) ret[i] = 0x1;
	/* U+FE30 - U+FE4F CJK Compatibility Forms
	 * U+FE50 - U+FE6F Small Form Variants (for CNS 11643)
	 */
  for (i = 0xfe70; i < 0xfeff; ++i) ret[i] = 0x1;
	/* U+FF00 - U+FFEF CJK Compatibility Ideographs */
  for (i = 0xfff0; i < 0x10000; ++i) ret[i] = 0x1;

				/* for each supplied charset */
  for (csi = 1; ret && charsets && (s = charsets[csi - 1]); ++csi) {
				/* substitute EUC-JP for ISO-2022-JP */
    if (!compare_cstring (s,"ISO-2022-JP")) s = "EUC-JP";
				/* look up charset */
    if (cs = utf8_charset (s)) {
      csb = 1 << csi;		/* charset bit */
      switch (cs->type) {
      case CT_ASCII:		/* 7-bit ASCII no table */
      case CT_1BYTE0:		/* 1 byte no table */
      case CT_1BYTE:		/* 1 byte ASCII + table 0x80-0xff */
      case CT_1BYTE8:		/* 1 byte table 0x00 - 0xff */
      case CT_EUC:		/* 2 byte ASCII + utf8_eucparam base/CS2/CS3 */
      case CT_DBYTE:		/* 2 byte ASCII + utf8_eucparam */
      case CT_DBYTE2:		/* 2 byte ASCII + utf8_eucparam plane1/2 */
      case CT_SJIS:		/* 2 byte Shift-JIS */
				/* supported charset type, all ASCII is OK */
	for (i = 0; i < 128; ++i) ret[i] |= csb;
	break;
      default:			/* unsupported charset type */
	fs_give ((void **) &ret);
	break;
      }
				/* now do additional operations */
      if (ret) switch (cs->type) {
      case CT_1BYTE0:		/* 1 byte no table */
	for (i = 128; i < 256; i++) ret[i] |= csb;
	break;
      case CT_1BYTE:		/* 1 byte ASCII + table 0x80-0xff */
	for (tab = (unsigned short *) cs->tab,i = 128; i < 256; i++)
	  if (tab[i & BITS7] != UBOGON) ret[tab[i & BITS7]] |= csb;
	break;
      case CT_1BYTE8:		/* 1 byte table 0x00 - 0xff */
	for (tab = (unsigned short *) cs->tab,i = 0; i < 256; i++)
	  if (tab[i] != UBOGON) ret[tab[i]] |= csb;
      break;
      case CT_EUC:		/* 2 byte ASCII + utf8_eucparam base/CS2/CS3 */
	for (param = (struct utf8_eucparam *) cs->tab,
	       tab = (unsigned short *) param->tab, ku = 0;
	     ku < param->max_ku; ku++)
	  for (ten = 0; ten < param->max_ten; ten++)
	    if ((u = tab[(ku * param->max_ten) + ten]) != UBOGON)
	      ret[u] |= csb;
	break;

      case CT_DBYTE:		/* 2 byte ASCII + utf8_eucparam */
	for (param = (struct utf8_eucparam *) cs->tab,
	       tab = (unsigned short *) param->tab, ku = 0;
	     ku < param->max_ku; ku++)
	  for (ten = 0; ten < param->max_ten; ten++)
	    if ((u = tab[(ku * param->max_ten) + ten]) != UBOGON)
	      ret[u] |= csb;
      break;
      case CT_DBYTE2:		/* 2 byte ASCII + utf8_eucparam plane1/2 */
	param = (struct utf8_eucparam *) cs->tab;
	p2 = param + 1;		/* plane 2 parameters */
				/* only ten parameters should differ */
	if ((param->base_ku != p2->base_ku) || (param->max_ku != p2->max_ku))
	  fatal ("ku definition error for CT_DBYTE2 charset");
				/* total codepoints in each ku */
	m = param->max_ten + p2->max_ten;
	tab = (unsigned short *) param->tab;
	for (ku = 0; ku < param->max_ku; ku++) {
	  for (ten = 0; ten < param->max_ten; ten++)
	    if ((u = tab[(ku * m) + ten]) != UBOGON)
	      ret[u] |= csb;
	  for (ten = 0; ten < p2->max_ten; ten++)
	    if ((u = tab[(ku * m) + param->max_ten + ten]) != UBOGON)
	      ret[u] |= csb;
	}
	break;
      case CT_SJIS:		/* 2 byte Shift-JIS */
	for (ku = 0; ku < MAX_JIS0208_KU; ku++)
	  for (ten = 0; ten < MAX_JIS0208_TEN; ten++)
	    if ((u = jis0208tab[ku][ten]) != UBOGON) ret[u] |= csb;
				/* JIS hankaku katakana */
	for (u = 0; u < (MAX_KANA_8 - MIN_KANA_8); u++)
	  ret[UCS2_KATAKANA + u] |= csb;
	break;
      }
    }
				/* invalid charset, punt */
    else fs_give ((void **) &ret);
  }
  return ret;
}

/* Infer charset from unlabelled sized text
 * Accepts: sized text
 * Returns: charset if one inferred, or NIL if unknown
 */

const CHARSET *utf8_infercharset (SIZEDTEXT *src)
{
  long iso2022jp = NIL;
  long eightbit = NIL;
  unsigned long i;
				/* look for ISO 2022 */
  if (src) for (i = 0; i < src->size; i++) {
				/* ESC sequence? */
    if ((src->data[i] == I2C_ESC) && (++i < src->size)) switch (src->data[i]) {
    case I2C_MULTI:		/* yes, multibyte? */
      if (++i < src->size) switch (src->data[i]) {
      case I2CS_94x94_JIS_OLD:	/* JIS X 0208-1978 */
      case I2CS_94x94_JIS_NEW:	/* JIS X 0208-1983 */
      case I2CS_94x94_JIS_EXT:	/* JIS X 0212-1990 (kludge...) */
	iso2022jp = T;		/* found an ISO-2022-JP sequence */
	break;
      default:			/* other multibyte */
	return NIL;		/* definitely invalid */
      }
      break;
    case I2C_G0_94:		/* single byte */
      if (++i < src->size) switch (src->data[i]) {
      case I2CS_94_JIS_BUGROM:	/* in case old buggy software */
      case I2CS_94_JIS_ROMAN:	/* JIS X 0201-1976 left half */
      case I2CS_94_ASCII:	/* ASCII */
      case I2CS_94_BRITISH:	/* good enough for gov't work */
	break;
      default:			/* other 94 single byte */
	return NIL;		/* definitely invalid */
      }
    }
				/* if possible UTF-8 and not ISO-2022-JP */
    else if (!iso2022jp && (eightbit >= 0) && (src->data[i] & BIT8) &&
	     (eightbit = utf8_validate (src->data + i,src->size - i)) > 0)
      i += eightbit - 1;	/* skip past all but last of UTF-8 char */
  }
				/* ISO-2022-JP overrides other guesses */
  if (iso2022jp) return utf8_charset ("ISO-2022-JP");
  if (eightbit > 0) return utf8_charset ("UTF-8");
  return eightbit ? NIL : utf8_charset ("US-ASCII");
}


/* Validate that character at this position is UTF-8
 * Accepts: string pointer
 *	    size of remaining string
 * Returns: size of UTF-8 character in octets or -1 if not UTF-8
 */

long utf8_validate (unsigned char *s,unsigned long i)
{
  unsigned long j = i;
  return (utf8_get (&s,&i) & U8G_ERROR) ? -1 : j - i;
}

/* Convert ISO 8859-1 to UTF-8
 * Accepts: source sized text
 *	    pointer to return sized text
 *	    canonicalization function
 */

void utf8_text_1byte0 (SIZEDTEXT *text,SIZEDTEXT *ret,ucs4cn_t cv,ucs4de_t de)
{
  unsigned long i;
  unsigned char *s;
  unsigned int c;
  for (ret->size = i = 0; i < text->size;) {
    c = text->data[i++];
    UTF8_COUNT_BMP (ret->size,c,cv,de)
  }
  (s = ret->data = (unsigned char *) fs_get (ret->size + 1))[ret->size] =NIL;
  for (i = 0; i < text->size;) {
    c = text->data[i++];
    UTF8_WRITE_BMP (s,c,cv,de)	/* convert UCS-2 to UTF-8 */
  }
}


/* Convert single byte ASCII+8bit character set sized text to UTF-8
 * Accepts: source sized text
 *	    pointer to return sized text
 *	    conversion table
 *	    canonicalization function
 */

void utf8_text_1byte (SIZEDTEXT *text,SIZEDTEXT *ret,void *tab,ucs4cn_t cv,
		      ucs4de_t de)
{
  unsigned long i;
  unsigned char *s;
  unsigned int c;
  unsigned short *tbl = (unsigned short *) tab;
  for (ret->size = i = 0; i < text->size;) {
    if ((c = text->data[i++]) & BIT8) c = tbl[c & BITS7];
    UTF8_COUNT_BMP (ret->size,c,cv,de)
  }
  (s = ret->data = (unsigned char *) fs_get (ret->size + 1))[ret->size] =NIL;
  for (i = 0; i < text->size;) {
    if ((c = text->data[i++]) & BIT8) c = tbl[c & BITS7];
    UTF8_WRITE_BMP (s,c,cv,de)	/* convert UCS-2 to UTF-8 */
  }
}

/* Convert single byte 8bit character set sized text to UTF-8
 * Accepts: source sized text
 *	    pointer to return sized text
 *	    conversion table
 *	    canonicalization function
 */

void utf8_text_1byte8 (SIZEDTEXT *text,SIZEDTEXT *ret,void *tab,ucs4cn_t cv,
		       ucs4de_t de)
{
  unsigned long i;
  unsigned char *s;
  unsigned int c;
  unsigned short *tbl = (unsigned short *) tab;
  for (ret->size = i = 0; i < text->size;) {
    c = tbl[text->data[i++]];
    UTF8_COUNT_BMP (ret->size,c,cv,de)
  }
  (s = ret->data = (unsigned char *) fs_get (ret->size + 1))[ret->size] =NIL;
  for (i = 0; i < text->size;) {
    c = tbl[text->data[i++]];
    UTF8_WRITE_BMP (s,c,cv,de)	/* convert UCS-2 to UTF-8 */
  }
}

/* Convert EUC sized text to UTF-8
 * Accepts: source sized text
 *	    pointer to return sized text
 *	    EUC parameter table
 *	    canonicalization function
 */

void utf8_text_euc (SIZEDTEXT *text,SIZEDTEXT *ret,void *tab,ucs4cn_t cv,
		    ucs4de_t de)
{
  unsigned long i;
  unsigned char *s;
  unsigned int pass,c,c1,ku,ten;
  struct utf8_eucparam *p1 = (struct utf8_eucparam *) tab;
  struct utf8_eucparam *p2 = p1 + 1;
  struct utf8_eucparam *p3 = p1 + 2;
  unsigned short *t1 = (unsigned short *) p1->tab;
  unsigned short *t2 = (unsigned short *) p2->tab;
  unsigned short *t3 = (unsigned short *) p3->tab;
  for (pass = 0,s = NIL,ret->size = 0; pass <= 1; pass++) {
    for (i = 0; i < text->size;) {
				/* not CS0? */
      if ((c = text->data[i++]) & BIT8) {
				/* yes, must have another high byte */
	if ((i >= text->size) || !((c1 = text->data[i++]) & BIT8))
	  c = UBOGON;		/* out of space or bogon */
	else switch (c) {	/* check 8bit code set */
	case EUC_CS2:		/* CS2 */
	  if (p2->base_ku) {	/* CS2 set up? */
	    if (p2->base_ten)	/* yes, multibyte? */
	      c = ((i < text->size) && ((c = text->data[i++]) & BIT8) &&
		   ((ku = (c1 & BITS7) - p2->base_ku) < p2->max_ku) &&
		   ((ten = (c & BITS7) - p2->base_ten) < p2->max_ten)) ?
		     t2[(ku*p2->max_ten) + ten] : UBOGON;
	    else c = ((c1 >= p2->base_ku) && (c1 < p2->max_ku)) ?
	      c1 + ((unsigned long) p2->tab) : UBOGON;
	  }	  
	  else {		/* CS2 not set up */
	    c = UBOGON;		/* swallow byte, say bogon */
	    if (i < text->size) i++;
	  }
	  break;
	case EUC_CS3:		/* CS3 */
	  if (p3->base_ku) {	/* CS3 set up? */
	    if (p3->base_ten)	/* yes, multibyte? */
	      c = ((i < text->size) && ((c = text->data[i++]) & BIT8) &&
		   ((ku = (c1 & BITS7) - p3->base_ku) < p3->max_ku) &&
		   ((ten = (c & BITS7) - p3->base_ten) < p3->max_ten)) ?
		     t3[(ku*p3->max_ten) + ten] : UBOGON;
	    else c = ((c1 >= p3->base_ku) && (c1 < p3->max_ku)) ?
	      c1 + ((unsigned long) p3->tab) : UBOGON;
	  }	  
	  else {		/* CS3 not set up */
	    c = UBOGON;		/* swallow byte, say bogon */
	    if (i < text->size) i++;
	  }
	  break;

	default:
	  if (((ku = (c & BITS7) - p1->base_ku) >= p1->max_ku) ||
	      ((ten = (c1 & BITS7) - p1->base_ten) >= p1->max_ten)) c = UBOGON;
	  else if (((c = t1[(ku*p1->max_ten) + ten]) == UBOGON) &&
		   /* special hack for JIS X 0212: merge rows less than 10 */
		   ku && (ku < 10) && t3 && p3->base_ten)
	    c = t3[((ku - (p3->base_ku - p1->base_ku))*p3->max_ten) + ten];
	}
      }
				/* convert if second pass */
      if (pass) UTF8_WRITE_BMP (s,c,cv,de)
      else UTF8_COUNT_BMP (ret->size,c,cv,de);
    }
    if (!pass) (s = ret->data = (unsigned char *)
		fs_get (ret->size + 1))[ret->size] =NIL;
  }
}


/* Convert ASCII + double-byte sized text to UTF-8
 * Accepts: source sized text
 *	    pointer to return sized text
 *	    conversion table
 *	    canonicalization function
 */

void utf8_text_dbyte (SIZEDTEXT *text,SIZEDTEXT *ret,void *tab,ucs4cn_t cv,
		      ucs4de_t de)
{
  unsigned long i;
  unsigned char *s;
  unsigned int c,c1,ku,ten;
  struct utf8_eucparam *p1 = (struct utf8_eucparam *) tab;
  unsigned short *t1 = (unsigned short *) p1->tab;
  for (ret->size = i = 0; i < text->size;) {
    if ((c = text->data[i++]) & BIT8) {
				/* special hack for GBK: 0x80 is Euro */
      if ((c == 0x80) && (t1 == (unsigned short *) gb2312tab)) c = UCS2_EURO;
      else c = ((i < text->size) && (c1 = text->data[i++]) &&
		((ku = c - p1->base_ku) < p1->max_ku) &&
		((ten = c1 - p1->base_ten) < p1->max_ten)) ?
	     t1[(ku*p1->max_ten) + ten] : UBOGON;
    }
    UTF8_COUNT_BMP (ret->size,c,cv,de)
  }
  (s = ret->data = (unsigned char *) fs_get (ret->size + 1))[ret->size] = NIL;
  for (i = 0; i < text->size;) {
    if ((c = text->data[i++]) & BIT8) {
				/* special hack for GBK: 0x80 is Euro */
      if ((c == 0x80) && (t1 == (unsigned short *) gb2312tab)) c = UCS2_EURO;
      else c = ((i < text->size) && (c1 = text->data[i++]) &&
		((ku = c - p1->base_ku) < p1->max_ku) &&
		((ten = c1 - p1->base_ten) < p1->max_ten)) ?
	     t1[(ku*p1->max_ten) + ten] : UBOGON;
    }
    UTF8_WRITE_BMP (s,c,cv,de)	/* convert UCS-2 to UTF-8 */
  }
}

/* Convert ASCII + double byte 2 plane sized text to UTF-8
 * Accepts: source sized text
 *	    pointer to return sized text
 *	    conversion table
 *	    canonicalization function
 */

void utf8_text_dbyte2 (SIZEDTEXT *text,SIZEDTEXT *ret,void *tab,ucs4cn_t cv,
		       ucs4de_t de)
{
  unsigned long i;
  unsigned char *s;
  unsigned int c,c1,ku,ten;
  struct utf8_eucparam *p1 = (struct utf8_eucparam *) tab;
  struct utf8_eucparam *p2 = p1 + 1;
  unsigned short *t = (unsigned short *) p1->tab;
  for (ret->size = i = 0; i < text->size;) {
    if ((c = text->data[i++]) & BIT8) {
      if ((i >= text->size) || !(c1 = text->data[i++]))
	c = UBOGON;		/* out of space or bogon */
      else if (c1 & BIT8)	/* high vs. low plane */
	c = ((ku = c - p2->base_ku) < p2->max_ku &&
	     ((ten = c1 - p2->base_ten) < p2->max_ten)) ?
	       t[(ku*(p1->max_ten + p2->max_ten)) + p1->max_ten + ten] :UBOGON;
      else c = ((ku = c - p1->base_ku) < p1->max_ku &&
		((ten = c1 - p1->base_ten) < p1->max_ten)) ?
		  t[(ku*(p1->max_ten + p2->max_ten)) + ten] : UBOGON;
    }
    UTF8_COUNT_BMP (ret->size,c,cv,de)
  }
  (s = ret->data = (unsigned char *) fs_get (ret->size + 1))[ret->size] = NIL;
  for (i = 0; i < text->size;) {
    if ((c = text->data[i++]) & BIT8) {
      if ((i >= text->size) || !(c1 = text->data[i++]))
	c = UBOGON;		/* out of space or bogon */
      else if (c1 & BIT8)	/* high vs. low plane */
	c = ((ku = c - p2->base_ku) < p2->max_ku &&
	     ((ten = c1 - p2->base_ten) < p2->max_ten)) ?
	       t[(ku*(p1->max_ten + p2->max_ten)) + p1->max_ten + ten] :UBOGON;
      else c = ((ku = c - p1->base_ku) < p1->max_ku &&
		((ten = c1 - p1->base_ten) < p1->max_ten)) ?
		  t[(ku*(p1->max_ten + p2->max_ten)) + ten] : UBOGON;
    }
    UTF8_WRITE_BMP (s,c,cv,de)	/* convert UCS-2 to UTF-8 */
  }
}

#ifdef JISTOUNICODE		/* Japanese */
/* Convert Shift JIS sized text to UTF-8
 * Accepts: source sized text
 *	    pointer to return sized text
 *	    canonicalization function
 */

void utf8_text_sjis (SIZEDTEXT *text,SIZEDTEXT *ret,ucs4cn_t cv,
		     ucs4de_t de)
{
  unsigned long i;
  unsigned char *s;
  unsigned int c,c1,ku,ten;
  for (ret->size = i = 0; i < text->size;) {
    if ((c = text->data[i++]) & BIT8) {
				/* half-width katakana */
      if ((c >= MIN_KANA_8) && (c < MAX_KANA_8)) c += KANA_8;
      else if (i >= text->size) c = UBOGON;
      else {			/* Shift-JIS */
	c1 = text->data[i++];
	SJISTOJIS (c,c1);
	c = JISTOUNICODE (c,c1,ku,ten);
      }
    }
				/* compromise - do yen sign but not overline */
    else if (c == JISROMAN_YEN) c = UCS2_YEN;
    UTF8_COUNT_BMP (ret->size,c,cv,de)
  }
  (s = ret->data = (unsigned char *) fs_get (ret->size + 1))[ret->size] = NIL;
  for (i = 0; i < text->size;) {
    if ((c = text->data[i++]) & BIT8) {
				/* half-width katakana */
      if ((c >= MIN_KANA_8) && (c < MAX_KANA_8)) c += KANA_8;
      else {			/* Shift-JIS */
	c1 = text->data[i++];
	SJISTOJIS (c,c1);
	c = JISTOUNICODE (c,c1,ku,ten);
      }
    }
				/* compromise - do yen sign but not overline */
    else if (c == JISROMAN_YEN) c = UCS2_YEN;
    UTF8_WRITE_BMP (s,c,cv,de)	/* convert UCS-2 to UTF-8 */
  }
}
#endif

/* Convert ISO-2022 sized text to UTF-8
 * Accepts: source sized text
 *	    pointer to returned sized text
 *	    canonicalization function
 */

void utf8_text_2022 (SIZEDTEXT *text,SIZEDTEXT *ret,ucs4cn_t cv,ucs4de_t de)
{
  unsigned long i;
  unsigned char *s;
  unsigned int pass,state,c,co,gi,gl,gr,g[4],ku,ten;
  for (pass = 0,s = NIL,ret->size = 0; pass <= 1; pass++) {
    gi = 0;			/* quell compiler warnings */
    state = I2S_CHAR;		/* initialize engine */
    g[0]= g[2] = I2CS_ASCII;	/* G0 and G2 are ASCII */
    g[1]= g[3] = I2CS_ISO8859_1;/* G1 and G3 are ISO-8850-1 */
    gl = I2C_G0; gr = I2C_G1;	/* left is G0, right is G1 */
    for (i = 0; i < text->size;) {
      c = text->data[i++];
      switch (state) {		/* dispatch based upon engine state */
      case I2S_ESC:		/* ESC seen */
	switch (c) {		/* process intermediate character */
	case I2C_MULTI:		/* multibyte character? */
	  state = I2S_MUL;	/* mark multibyte flag seen */
	  break;
        case I2C_SS2:		/* single shift GL to G2 */
	case I2C_SS2_ALT:	/* Taiwan SeedNet */
	  gl |= I2C_SG2;
	  break;
        case I2C_SS3:		/* single shift GL to G3 */
	case I2C_SS3_ALT:	/* Taiwan SeedNet */
	  gl |= I2C_SG3;
	  break;
        case I2C_LS2:		/* shift GL to G2 */
	  gl = I2C_G2;
	  break;
        case I2C_LS3:		/* shift GL to G3 */
	  gl = I2C_G3;
	  break;
        case I2C_LS1R:		/* shift GR to G1 */
	  gr = I2C_G1;
	  break;
        case I2C_LS2R:		/* shift GR to G2 */
	  gr = I2C_G2;
	  break;
        case I2C_LS3R:		/* shift GR to G3 */
	  gr = I2C_G3;
	  break;
	case I2C_G0_94: case I2C_G1_94: case I2C_G2_94:	case I2C_G3_94:
	  g[gi = c - I2C_G0_94] = (state == I2S_MUL) ? I2CS_94x94 : I2CS_94;
	  state = I2S_INT;	/* ready for character set */
	  break;
	case I2C_G0_96:	case I2C_G1_96: case I2C_G2_96:	case I2C_G3_96:
	  g[gi = c - I2C_G0_96] = (state == I2S_MUL) ? I2CS_96x96 : I2CS_96;
	  state = I2S_INT;	/* ready for character set */
	  break;
	default:		/* bogon */
	  if (pass) *s++ = I2C_ESC,*s++ = c;
	  else ret->size += 2;
	  state = I2S_CHAR;	/* return to previous state */
	}
	break;

      case I2S_MUL:		/* ESC $ */
	switch (c) {		/* process multibyte intermediate character */
	case I2C_G0_94: case I2C_G1_94: case I2C_G2_94:	case I2C_G3_94:
	  g[gi = c - I2C_G0_94] = I2CS_94x94;
	  state = I2S_INT;	/* ready for character set */
	  break;
	case I2C_G0_96:	case I2C_G1_96: case I2C_G2_96:	case I2C_G3_96:
	  g[gi = c - I2C_G0_96] = I2CS_96x96;
	  state = I2S_INT;	/* ready for character set */
	  break;
	default:		/* probably omitted I2CS_94x94 */
	  g[gi = I2C_G0] = I2CS_94x94 | c;
	  state = I2S_CHAR;	/* return to character state */
	}
	break;
      case I2S_INT:
	state = I2S_CHAR;	/* return to character state */
	g[gi] |= c;		/* set character set */
	break;

      case I2S_CHAR:		/* character data */
	switch (c) {
	case I2C_ESC:		/* ESC character */
	  state = I2S_ESC;	/* see if ISO-2022 prefix */
	  break;
	case I2C_SI:		/* shift GL to G0 */
	  gl = I2C_G0;
	  break;
	case I2C_SO:		/* shift GL to G1 */
	  gl = I2C_G1;
	  break;
        case I2C_SS2_ALT:	/* single shift GL to G2 */
	case I2C_SS2_ALT_7:
	  gl |= I2C_SG2;
	  break;
        case I2C_SS3_ALT:	/* single shift GL to G3 */
	case I2C_SS3_ALT_7:
	  gl |= I2C_SG3;
	  break;

	default:		/* ordinary character */
	  co = c;		/* note original character */
	  if (gl & (3 << 2)) {	/* single shifted? */
	    gi = g[gl >> 2];	/* get shifted character set */
	    gl &= 0x3;		/* cancel shift */
	  }
				/* select left or right half */
	  else gi = (c & BIT8) ? g[gr] : g[gl];
	  c &= BITS7;		/* make 7-bit */
	  switch (gi) {		/* interpret in character set */
	  case I2CS_ASCII:	/* ASCII */
	    break;		/* easy! */
	  case I2CS_BRITISH:	/* British ASCII */
				/* Pound sterling sign */
	    if (c == BRITISH_POUNDSTERLING) c = UCS2_POUNDSTERLING;
	    break;
	  case I2CS_JIS_ROMAN:	/* JIS Roman */
	  case I2CS_JIS_BUGROM:	/* old bugs */
	    switch (c) {	/* two exceptions to ASCII */
	    case JISROMAN_YEN:	/* Yen sign */
	      c = UCS2_YEN;
	      break;
				/* overline */
	    case JISROMAN_OVERLINE:
	      c = UCS2_OVERLINE;
	      break;
	    }
	    break;
	  case I2CS_JIS_KANA:	/* JIS hankaku katakana */
	    if ((c >= MIN_KANA_7) && (c < MAX_KANA_7)) c += KANA_7;
	    break;

	  case I2CS_ISO8859_1:	/* Latin-1 (West European) */
	    c |= BIT8;		/* just turn on high bit */
	    break;
	  case I2CS_ISO8859_2:	/* Latin-2 (Czech, Slovak) */
	    c = iso8859_2tab[c];
	    break;
	  case I2CS_ISO8859_3:	/* Latin-3 (Dutch, Turkish) */
	    c = iso8859_3tab[c];
	    break;
	  case I2CS_ISO8859_4:	/* Latin-4 (Scandinavian) */
	    c = iso8859_4tab[c];
	    break;
	  case I2CS_ISO8859_5:	/* Cyrillic */
	    c = iso8859_5tab[c];
	    break;
	  case I2CS_ISO8859_6:	/* Arabic */
	    c = iso8859_6tab[c];
	    break;
	  case I2CS_ISO8859_7:	/* Greek */
	    c = iso8859_7tab[c];
	    break;
	  case I2CS_ISO8859_8:	/* Hebrew */
	    c = iso8859_8tab[c];
	    break;
	  case I2CS_ISO8859_9:	/* Latin-5 (Finnish, Portuguese) */
	    c = iso8859_9tab[c];
	    break;
	  case I2CS_TIS620:	/* Thai */
	    c = tis620tab[c];
	    break;
	  case I2CS_ISO8859_10:	/* Latin-6 (Northern Europe) */
	    c = iso8859_10tab[c];
	    break;
	  case I2CS_ISO8859_13:	/* Latin-7 (Baltic) */
	    c = iso8859_13tab[c];
	    break;
	  case I2CS_VSCII:	/* Vietnamese */
	    c = visciitab[c];
	    break;
	  case I2CS_ISO8859_14:	/* Latin-8 (Celtic) */
	    c = iso8859_14tab[c];
	    break;
	  case I2CS_ISO8859_15:	/* Latin-9 (Euro) */
	    c = iso8859_15tab[c];
	    break;
	  case I2CS_ISO8859_16:	/* Latin-10 (Baltic) */
	    c = iso8859_16tab[c];
	    break;

	  default:		/* all other character sets */
				/* multibyte character set */
	    if ((gi & I2CS_MUL) && !(c & BIT8) && isgraph (c)) {
	      c = (i < text->size) ? text->data[i++] : 0;
	      switch (gi) {
#ifdef GBTOUNICODE
	      case I2CS_GB:	/* GB 2312 */
		co |= BIT8;	/* make into EUC */
		c |= BIT8;
		c = GBTOUNICODE (co,c,ku,ten);
		break;
#endif
#ifdef JISTOUNICODE
	      case I2CS_JIS_OLD:/* JIS X 0208-1978 */
	      case I2CS_JIS_NEW:/* JIS X 0208-1983 */
		c = JISTOUNICODE (co,c,ku,ten);
		break;
#endif
#ifdef JIS0212TOUNICODE
	      case I2CS_JIS_EXT:/* JIS X 0212-1990 */
		c = JIS0212TOUNICODE (co,c,ku,ten);
		break;
#endif
#ifdef KSCTOUNICODE
	      case I2CS_KSC:	/* KSC 5601 */
		co |= BIT8;	/* make into EUC */
		c |= BIT8;
		c = KSCTOUNICODE (co,c,ku,ten);
		break;
#endif
#ifdef CNS1TOUNICODE
	      case I2CS_CNS1:	/* CNS 11643 plane 1 */
		c = CNS1TOUNICODE (co,c,ku,ten);
		break;
#endif
#ifdef CNS2TOUNICODE
	      case I2CS_CNS2:	/* CNS 11643 plane 2 */
		c = CNS2TOUNICODE (co,c,ku,ten);
		break;
#endif
#ifdef CNS3TOUNICODE
	      case I2CS_CNS3:	/* CNS 11643 plane 3 */
		c = CNS3TOUNICODE (co,c,ku,ten);
		break;
#endif
#ifdef CNS4TOUNICODE
	      case I2CS_CNS4:	/* CNS 11643 plane 4 */
		c = CNS4TOUNICODE (co,c,ku,ten);
		break;
#endif
#ifdef CNS5TOUNICODE
	      case I2CS_CNS5:	/* CNS 11643 plane 5 */
		c = CNS5TOUNICODE (co,c,ku,ten);
		break;
#endif
#ifdef CNS6TOUNICODE
	      case I2CS_CNS6:	/* CNS 11643 plane 6 */
		c = CNS6TOUNICODE (co,c,ku,ten);
		break;
#endif
#ifdef CNS7TOUNICODE
	      case I2CS_CNS7:	/* CNS 11643 plane 7 */
		c = CNS7TOUNICODE (co,c,ku,ten);
		break;
#endif
	      default:		/* unknown multibyte, treat as UCS-2 */
		c |= (co << 8);	/* wrong, but nothing else to do */
		break;
	      }
	    }
	    else c = co;	/* unknown single byte, treat as 8859-1 */
	  }
				/* convert if second pass */
	  if (pass) UTF8_WRITE_BMP (s,c,cv,de)
	  else UTF8_COUNT_BMP (ret->size,c,cv,de);
	}
      }
    }
    if (!pass) (s = ret->data = (unsigned char *)
		fs_get (ret->size + 1))[ret->size] = NIL;
    else if (((unsigned long) (s - ret->data)) != ret->size)
      fatal ("ISO-2022 to UTF-8 botch");
  }
}

/* Convert UTF-7 sized text to UTF-8
 * Accepts: source sized text
 *	    pointer to returned sized text
 *	    canonicalization function
 */

void utf8_text_utf7 (SIZEDTEXT *text,SIZEDTEXT *ret,ucs4cn_t cv,ucs4de_t de)
{
  unsigned long i;
  unsigned char *s;
  unsigned int c,c1,d,uc,pass,e,e1,state,surrh;
  for (pass = 0,s = NIL,ret->size = 0; pass <= 1; pass++) {
    c1 = d = uc = e = e1 = 0;
    for (i = 0,state = NIL; i < text->size;) {
      c = text->data[i++];	/* get next byte */
      switch (state) {
      case U7_PLUS:		/* previous character was + */
	if (c == '-') {		/* +- means textual + */
	  c = '+';
	  state = U7_ASCII;	/* revert to ASCII */
	  break;
	}
	state = U7_UNICODE;	/* enter Unicode state */
	e = e1 = 0;		/* initialize Unicode quantum position */
      case U7_UNICODE:		/* Unicode state */
	if (c == '-') state = U7_MINUS;
	else {			/* decode Unicode */
	  /* don't use isupper/islower since this is ASCII only */
	  if ((c >= 'A') && (c <= 'Z')) c -= 'A';
	  else if ((c >= 'a') && (c <= 'z')) c -= 'a' - 26;
	  else if (isdigit (c)) c -= '0' - 52;
	  else if (c == '+') c = 62;
	  else if (c == '/') c = 63;
	  else state = U7_ASCII;/* end of modified BASE64 */
	}
	break;
      case U7_MINUS:		/* previous character was absorbed - */
	state = U7_ASCII;	/* revert to ASCII */
      case U7_ASCII:		/* ASCII state */
	if (c == '+') state = U7_PLUS;
	break;
      }

      switch (state) {		/* store character if in character mode */
      case U7_UNICODE:		/* Unicode */
	switch (e++) {		/* install based on BASE64 state */
	case 0:
	  c1 = c << 2;		/* byte 1: high 6 bits */
	  break;
	case 1:
	  d = c1 | (c >> 4);	/* byte 1: low 2 bits */
	  c1 = c << 4;		/* byte 2: high 4 bits */
	  break;
	case 2:
	  d = c1 | (c >> 2);	/* byte 2: low 4 bits */
	  c1 = c << 6;		/* byte 3: high 2 bits */
	  break;
	case 3:
	  d = c | c1;		/* byte 3: low 6 bits */
	  e = 0;		/* reinitialize mechanism */
	  break;
	}
	if (e == 1) break;	/* done if first BASE64 state */
	if (!e1) {		/* first byte of UCS-2 character */
	  uc = (d & 0xff) << 8;	/* note first byte */
	  e1 = T;		/* enter second UCS-2 state */
	  break;		/* done */
	}
	c = uc | (d & 0xff);	/* build UCS-2 character */
	e1 = NIL;		/* back to first UCS-2 state, drop in */
				/* surrogate pair?  */
	if ((c >= UTF16_SURR) && (c <= UTF16_MAXSURR)) {
				/* save high surrogate for later */
	  if (c < UTF16_SURRL) surrh = c;
	  else c = UTF16_BASE + ((surrh & UTF16_MASK) << UTF16_SHIFT) +
		 (c & UTF16_MASK);
	  break;		/* either way with surrogates, we're done */
	}
      case U7_ASCII:		/* just install if ASCII */
				/* convert if second pass */
	if (pass) UTF8_WRITE_BMP (s,c,cv,de)
	else UTF8_COUNT_BMP (ret->size,c,cv,de);
      }
    }
    if (!pass) (s = ret->data = (unsigned char *)
		fs_get (ret->size + 1))[ret->size] = NIL;
    else if (((unsigned long) (s - ret->data)) != ret->size)
      fatal ("UTF-7 to UTF-8 botch");
  }
}


/* Convert UTF-8 sized text to UTF-8
 * Accepts: source sized text
 *	    pointer to returned sized text
 *	    canonicalization function
 */

void utf8_text_utf8 (SIZEDTEXT *text,SIZEDTEXT *ret,ucs4cn_t cv,ucs4de_t de)
{
  unsigned long i,c;
  unsigned char *s,*t;
  for (ret->size = 0, t = text->data, i = text->size; i;) {
    if ((c = utf8_get (&t,&i)) & U8G_ERROR) {
      ret->data = text->data;	/* conversion failed */
      ret->size = text->size;
      return;
    }
    UTF8_COUNT (ret->size,c,cv,de)
  }
  (s = ret->data = (unsigned char *) fs_get (ret->size + 1))[ret->size] =NIL;
  for (t = text->data, i = text->size; i;) {
    c = utf8_get (&t,&i);
    UTF8_WRITE (s,c,cv,de)	/* convert UCS-4 to UTF-8 */
  }
  if (((unsigned long) (s - ret->data)) != ret->size)
    fatal ("UTF-8 to UTF-8 botch");
}

/* Convert UCS-2 sized text to UTF-8
 * Accepts: source sized text
 *	    pointer to returned sized text
 *	    canonicalization function
 */

void utf8_text_ucs2 (SIZEDTEXT *text,SIZEDTEXT *ret,ucs4cn_t cv,ucs4de_t de)
{
  unsigned long i;
  unsigned char *s,*t;
  unsigned int c;
  for (ret->size = 0, t = text->data, i = text->size / 2; i; --i) {
    c = *t++ << 8;
    c |= *t++;
    UTF8_COUNT_BMP (ret->size,c,cv,de);
  }
  (s = ret->data = (unsigned char *) fs_get (ret->size + 1))[ret->size] = NIL;
  for (t = text->data, i = text->size / 2; i; --i) {
    c = *t++ << 8;
    c |= *t++;
    UTF8_WRITE_BMP (s,c,cv,de)	/* convert UCS-2 to UTF-8 */
  }
  if (((unsigned long) (s - ret->data)) != ret->size)
    fatal ("UCS-2 to UTF-8 botch");
}


/* Convert UCS-4 sized text to UTF-8
 * Accepts: source sized text
 *	    pointer to returned sized text
 *	    canonicalization function
 */

void utf8_text_ucs4 (SIZEDTEXT *text,SIZEDTEXT *ret,ucs4cn_t cv,ucs4de_t de)
{
  unsigned long i;
  unsigned char *s,*t;
  unsigned long c;
  for (ret->size = 0, t = text->data, i = text->size / 4; i; --i) {
    c = *t++ << 24; c |= *t++ << 16; c |= *t++ << 8; c |= *t++;
    UTF8_COUNT (ret->size,c,cv,de);
  }
  (s = ret->data = (unsigned char *) fs_get (ret->size + 1))[ret->size] = NIL;
  for (t = text->data, i = text->size / 2; i; --i) {
    c = *t++ << 24; c |= *t++ << 16; c |= *t++ << 8; c |= *t++;
    UTF8_WRITE (s,c,cv,de)	/* convert UCS-4 to UTF-8 */
  }
  if (((unsigned long) (s - ret->data)) != ret->size)
    fatal ("UCS-4 to UTF-8 botch");
}

/* Convert UTF-16 sized text to UTF-8
 * Accepts: source sized text
 *	    pointer to returned sized text
 *	    canonicalization function
 */

void utf8_text_utf16 (SIZEDTEXT *text,SIZEDTEXT *ret,ucs4cn_t cv,ucs4de_t de)
{
  unsigned long i;
  unsigned char *s,*t;
  unsigned long c,d;
  for (ret->size = 0, t = text->data, i = text->size / 2; i; --i) {
    c = *t++ << 8;
    c |= *t++;
				/* possible surrogate? */
    if ((c >= UTF16_SURR) && (c <= UTF16_MAXSURR)) {
				/* invalid first surrogate */
      if ((c > UTF16_SURRHEND) || !i) c = UBOGON;
      else {			/* get second surrogate */
	d = *t++ << 8;
	d |= *t++;
	--i;			/* swallowed another 16-bits */
				/* invalid second surrogate */
	if ((d < UTF16_SURRL) || (d > UTF16_SURRLEND)) c = UBOGON;
	else c = UTF16_BASE + ((c & UTF16_MASK) << UTF16_SHIFT) +
	       (d & UTF16_MASK);
      }
    }
    UTF8_COUNT (ret->size,c,cv,de);
  }
  (s = ret->data = (unsigned char *) fs_get (ret->size + 1))[ret->size] = NIL;
  for (t = text->data, i = text->size / 2; i; --i) {
    c = *t++ << 8;
    c |= *t++;
				/* possible surrogate? */
    if ((c >= UTF16_SURR) && (c <= UTF16_MAXSURR)) {
				/* invalid first surrogate */
      if ((c > UTF16_SURRHEND) || !i) c = UBOGON;
      else {			/* get second surrogate */
	d = *t++ << 8;
	d |= *t++;
	--i;			/* swallowed another 16-bits */
				/* invalid second surrogate */
	if ((d < UTF16_SURRL) || (d > UTF16_SURRLEND)) c = UBOGON;
	else c = UTF16_BASE + ((c & UTF16_MASK) << UTF16_SHIFT) +
	       (d & UTF16_MASK);
      }
    }
    UTF8_WRITE (s,c,cv,de)	/* convert UCS-4 to UTF-8 */
  }
  if (((unsigned long) (s - ret->data)) != ret->size)
    fatal ("UTF-16 to UTF-8 botch");
}

/* Size of UCS-4 character, possibly not in BMP, as UTF-8 octets
 * Accepts: character
 * Returns: size (0 means bogon)
 *
 * Use UTF8_SIZE macro if known to be in the BMP
 */

unsigned long utf8_size (unsigned long c)
{
  if (c < 0x80) return 1;
  else if (c < 0x800) return 2;
  else if (c < 0x10000) return 3;
  else if (c < 0x200000) return 4;
  else if (c < 0x4000000) return 5;
  else if (c < 0x80000000) return 6;
  return 0;
}


/* Put UCS-4 character, possibly not in BMP, as UTF-8 octets
 * Accepts: destination string pointer
 *	    character
 * Returns: updated destination pointer
 *
 * Use UTF8_PUT_BMP macro if known to be in the BMP
 */

unsigned char *utf8_put (unsigned char *s,unsigned long c)
{
  unsigned char mark[6] = {0x00,0xc0,0xe0,0xf0,0xf8,0xfc};
  unsigned long size = utf8_size (c);
  switch (size) {
  case 6:
    s[5] = 0x80 | (unsigned char) (c & 0x3f);
    c >>= 6;
  case 5:
    s[4] = 0x80 | (unsigned char) (c & 0x3f);
    c >>= 6;
  case 4:
    s[3] = 0x80 | (unsigned char) (c & 0x3f);
    c >>= 6;
  case 3:
    s[2] = 0x80 | (unsigned char) (c & 0x3f);
    c >>= 6;
  case 2:
    s[1] = 0x80 | (unsigned char) (c & 0x3f);
    c >>= 6;
  case 1:
    *s = mark[size-1] | (unsigned char) (c & 0x7f);
    break;
  }
  return s + size;
}

/* Return title case of a fixed-width UCS-4 character
 * Accepts: character
 * Returns: title case of character
 */

unsigned long ucs4_titlecase (unsigned long c)
{
  if (c <= UCS4_TMAPMAX) return ucs4_tmaptab[c];
  if (c < UCS4_TMAPHIMIN) return c;
  if (c <= UCS4_TMAPHIMAX) return c - UCS4_TMAPHIMAP;
  if (c < UCS4_TMAPDESERETMIN) return c;
  if (c <= UCS4_TMAPDESERETMAX) return c - UCS4_TMAPDESERETMAP;
  return c;
}


/* Return width of a fixed-width UCS-4 character in planes 0-2
 * Accepts: character
 * Returns: width (0, 1, 2) or negative error condition if not valid
 */

long ucs4_width (unsigned long c)
{
  long ret;
				/* out of range, not-a-char, or surrogates */
  if ((c > UCS4_MAXUNICODE) || ((c & 0xfffe) == 0xfffe) ||
      ((c >= UTF16_SURR) && (c <= UTF16_MAXSURR))) ret = U4W_NOTUNCD;
				/* private-use */
  else if (c >= UCS4_PVTBASE) ret = U4W_PRIVATE;
				/* SSP are not printing characters */
  else if (c >= UCS4_SSPBASE) ret = U4W_SSPCHAR;
				/* unassigned planes */
  else if (c >= UCS4_UNABASE) ret = U4W_UNASSGN;
				/* SIP and reserved plane 3 are wide */
  else if (c >= UCS4_SIPBASE) ret = 2;
#if (UCS4_WIDLEN != UCS4_SIPBASE)
#error "UCS4_WIDLEN != UCS4_SIPBASE"
#endif
				/* C0/C1 controls */
  else if ((c <= UCS2_C0CONTROLEND) ||
	   ((c >= UCS2_C1CONTROL) && (c <= UCS2_C1CONTROLEND)))
    ret = U4W_CONTROL;
				/* BMP and SMP get value from table */
  else switch (ret = (ucs4_widthtab[(c >> 2)] >> ((3 - (c & 0x3)) << 1)) &0x3){
  case 0:			/* zero-width */
    if (c == 0x00ad) ret = 1;	/* force U+00ad (SOFT HYPHEN) to width 1 */
  case 1:			/* single-width */
  case 2:			/* double-width */
    break;
  case 3:			/* ambiguous width */
    ret = (c >= 0x2100) ? 2 : 1;/* need to do something better than this */
    break;
  }
  return ret;
}

/* Return screen width of UTF-8 string
 * Accepts: string
 * Returns: width or negative if not valid UTF-8
 */

long utf8_strwidth (unsigned char *s)
{
  unsigned long c,i,ret;
				/* go through string */
  for (ret = 0; *s; ret += ucs4_width (c)) {
    /* It's alright to give a fake value for the byte count to utf8_get()
     * since the null of a null-terminated string will stop processing anyway.
     */
    i = 6;			/* fake value */
    if ((c = utf8_get (&s,&i)) & U8G_ERROR) return -1;
  }
  return ret;
}


/* Return screen width of UTF-8 text
 * Accepts: SIZEDTEXT to string
 * Returns: width or negative if not valid UTF-8
 */

long utf8_textwidth (SIZEDTEXT *utf8)
{
  unsigned long c;
  unsigned char *s = utf8->data;
  unsigned long i = utf8->size;
  unsigned long ret = 0;
  while (i) {			/* while there's a string to process */
    if ((c = utf8_get (&s,&i)) & U8G_ERROR) return -1;
    ret += ucs4_width (c);
  }
  return ret;
}

/* Decomposition (phew!) */

#define MORESINGLE 1		/* single UCS-4 tail value */
#define MOREMULTIPLE 2		/* multiple UCS-2 tail values */

struct decomposemore {
  short type;			/* type of more */
  union {
    unsigned long single;	/* single decomposed value */
    struct {			/* multiple BMP values */
      unsigned short *next;
      unsigned long count;
    } multiple;
  } data;
};

#define RECURSIVEMORE struct recursivemore

RECURSIVEMORE {
  struct decomposemore *more;
  RECURSIVEMORE *next;
};


/* Return decomposition of a UCS-4 character
 * Accepts: character or U8G_ERROR to return next from "more"
 *	    pointer to returned more
 * Returns: [next] decomposed value, more set if still more decomposition
 */

unsigned long ucs4_decompose (unsigned long c,void **more)
{
  unsigned long i,ix,ret;
  struct decomposemore *m;
  if (c & U8G_ERROR) {		/* want to chase more? */
				/* do sanity check */
    if (m = (struct decomposemore *) *more) switch (m->type) {
    case MORESINGLE:		/* single value */
      ret = m->data.single;
      fs_give (more);		/* no more decomposition */
      break;
    case MOREMULTIPLE:		/* multiple value */
      ret = *m->data.multiple.next++;
      if (!--m->data.multiple.count) fs_give (more);
      break;
    default:			/* uh-oh */
      fatal ("invalid more block argument to ucs4_decompose!");
    }
    else fatal ("no more block provided to ucs4_decompose!");
  }

  else {			/* start decomposition */
    *more = NIL;		/* initially set no more */
				/* BMP low decompositions */
    if (c < UCS4_BMPLOMIN) ret = c;
				/* fix this someday */
    else if (c == UCS4_BMPLOMIN) ret = ucs4_dbmplotab[0];
    else if (c <= UCS4_BMPLOMAX) {
				/* within range - have a decomposition? */
      if (i = ucs4_dbmploixtab[c - UCS4_BMPLOMIN]) {
				/* get first value of decomposition */
	ret = ucs4_dbmplotab[ix = i & UCS4_BMPLOIXMASK];
				/* has continuation? */
	if (i & UCS4_BMPLOSIZEMASK) {
	  m = (struct decomposemore *)
	    (*more = memset (fs_get (sizeof (struct decomposemore)),0,
			    sizeof (struct decomposemore)));
	  m->type = MOREMULTIPLE;
	  m->data.multiple.next = &ucs4_dbmplotab[++ix];
	  m->data.multiple.count = i >> UCS4_BMPLOSIZESHIFT;
	}
      }
      else ret = c;		/* in range but doesn't decompose */
    }
				/* BMP CJK compatibility */
    else if (c < UCS4_BMPCJKMIN) ret = c;
    else if (c <= UCS4_BMPCJKMAX) {
      if (!(ret = ucs4_bmpcjk1decomptab[c - UCS4_BMPCJKMIN])) ret = c;
    }
				/* BMP CJK compatibility - some not in BMP */
#if UCS4_BMPCJK2MIN - (UCS4_BMPCJKMAX + 1)
    else if (c < UCS4_BMPCJK2MIN) ret = c;
#endif
    else if (c <= UCS4_BMPCJK2MAX)
      ret = ucs4_bmpcjk2decomptab[c - UCS4_BMPCJK2MIN];
				/* BMP high decompositions */
    else if (c < UCS4_BMPHIMIN) ret = c;
    else if (c <= UCS4_BMPHIMAX) {
				/* within range - have a decomposition? */
      if (i = ucs4_dbmphiixtab[c - UCS4_BMPHIMIN]) {
				/* get first value of decomposition */
	ret = ucs4_dbmphitab[ix = i & UCS4_BMPHIIXMASK];
				/* has continuation? */
	if (i & UCS4_BMPHISIZEMASK) {
	  m = (struct decomposemore *)
	    (*more = memset (fs_get (sizeof (struct decomposemore)),0,
			    sizeof (struct decomposemore)));
	  m->type = MOREMULTIPLE;
	  m->data.multiple.next = &ucs4_dbmphitab[++ix];
	  m->data.multiple.count = i >> UCS4_BMPHISIZESHIFT;
	}
      }
      else ret = c;		/* in range but doesn't decompose */
    }

				/* BMP half and full width forms */
    else if (c < UCS4_BMPHALFFULLMIN) ret = c;
    else if (c <= UCS4_BMPHALFFULLMAX) {
      if (!(ret = ucs4_bmphalffulldecomptab[c - UCS4_BMPHALFFULLMIN])) ret = c;
    }
				/* SMP music */
    else if (c < UCS4_SMPMUSIC1MIN) ret = c;
    else if (c <= UCS4_SMPMUSIC1MAX) {
      ret = ucs4_smpmusic1decomptab[c -= UCS4_SMPMUSIC1MIN][0];
      m = (struct decomposemore *)
	(*more = memset (fs_get (sizeof (struct decomposemore)),0,
			 sizeof (struct decomposemore)));
      m->type = MORESINGLE;
      m->data.single = ucs4_smpmusic1decomptab[c][1];
    }
    else if (c < UCS4_SMPMUSIC2MIN) ret = c;
    else if (c <= UCS4_SMPMUSIC2MAX) {
      ret = ucs4_smpmusic2decomptab[c -= UCS4_SMPMUSIC2MIN][0];
      m = (struct decomposemore *)
	(*more = memset (fs_get (sizeof (struct decomposemore)),0,
			 sizeof (struct decomposemore)));
      m->type = MORESINGLE;
      m->data.single = ucs4_smpmusic2decomptab[c][1];
    }
				/* SMP mathematical forms */
    else if (c < UCS4_SMPMATHMIN) ret = c;
    else if (c <= UCS4_SMPMATHMAX) {
      if (!(ret = ucs4_smpmathdecomptab[c - UCS4_SMPMATHMIN])) ret = c;
    }
				/* CJK compatibility ideographs in SIP */
    else if (!(ret = ((c >= UCS4_SIPMIN) && (c <= UCS4_SIPMAX)) ?
	       ucs4_sipdecomptab[c - UCS4_SIPMIN] : c)) ret = c;
  }
  return ret;
}

/* Return recursive decomposition of a UCS-4 character
 * Accepts: character or U8G_ERROR to return next from "more"
 *	    pointer to returned more
 * Returns: [next] decomposed value, more set if still more decomposition
 */

unsigned long ucs4_decompose_recursive (unsigned long c,void **more)
{
  unsigned long c1;
  void *m,*mn;
  RECURSIVEMORE *mr;
  if (c & U8G_ERROR) {		/* want to chase more? */
    mn = NIL;
    if (mr = (RECURSIVEMORE *) *more) switch (mr->more->type) {
    case MORESINGLE:		/* decompose single value */
      c = ucs4_decompose_recursive (mr->more->data.single,&mn);
      *more = mr->next;		/* done with this more, remove it */
      fs_give ((void **) &mr->more);
      fs_give ((void **) &mr);
      break;
    case MOREMULTIPLE:		/* decompose current value in multiple */
      c = ucs4_decompose_recursive (*mr->more->data.multiple.next++,&mn);
				/* if done with this multiple decomposition */
      if (!--mr->more->data.multiple.count) {
	*more = mr->next;	/* done with this more, remove it */
	fs_give ((void **) &mr->more);
	fs_give ((void **) &mr);
      }
      break;
    default:			/* uh-oh */
      fatal ("invalid more block argument to ucs4_decompose_recursive!");
    }
    else fatal ("no more block provided to ucs4_decompose_recursive!");
    if (mr = mn) {		/* did this value recurse on us? */
      mr->next = *more;		/* yes, insert new more at head */
      *more = mr;
    }
  }
  else {			/* start decomposition */
    *more = NIL;		/* initially set no more */
    mr = NIL;
    do {			/* repeatedly decompose this codepoint */
      c = ucs4_decompose (c1 = c,&m);
      if (m) {			/* multi-byte decomposition */
	if (c1 == c) fatal ("endless multiple decomposition!");
				/* create a block to stash this more */
	mr = memset (fs_get (sizeof (RECURSIVEMORE)),0,sizeof (RECURSIVEMORE));
	mr->more = m;		/* note the expansion */
	mr->next = *more;	/* old list is the tail */
	*more = mr;		/* and this is the new head */
      }
    } while (c1 != c);		/* until nothing more to decompose */
  }
  return c;
}
