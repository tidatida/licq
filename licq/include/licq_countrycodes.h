#ifndef COUNTRY_H
#define COUNTRY_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define COUNTRY_UNSPECIFIED  0xFFFF
#define COUNTRY_UNKNOWN      0xFFFE
#define NUM_COUNTRIES 242

struct SCountry
{
  char *szName;          /* Name of the country */
  unsigned short nCode;  /* Country code */
  unsigned short nIndex; /* Index in array */
};
extern const struct SCountry gCountries[];

const struct SCountry *GetCountryByCode(unsigned short _nCountryCode);
const struct SCountry *GetCountryByIndex(unsigned short _nIndex);
const struct SCountry *GetCountryByName(const char *_szName);

#ifdef __cplusplus
}
#endif

#endif
