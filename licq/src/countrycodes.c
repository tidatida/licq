// -*- c-basic-offset: 2 -*-
/* ----------------------------------------------------------------------------
 * Licq - A ICQ Client for Unix
 * Copyright (C) 1998 - 2003 Licq developers
 *
 * This program is licensed under the terms found in the LICENSE file.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <string.h>

#include "licq_countrycodes.h"

const struct SCountry gCountries[NUM_COUNTRIES + 1] =
{
  { "Unspecified", COUNTRY_UNSPECIFIED, 0 },
  { "Afghanistan", 93, 1 },
  { "Albania", 355, 2 },
  { "Algeria", 213, 3 },
  { "American Samoa", 684, 4 },
  { "Andorra", 376, 5 },
  { "Angola", 244, 6 },
  { "Anguilla", 101, 7 },
  { "Antigua", 102, 8 },
  { "Argentina", 54, 9 },
  { "Armenia", 374, 10 },
  { "Aruba", 297, 11 },
  { "Ascension Island", 247, 12 },
  { "Australia", 61, 13 },
  { "Australian Antarctic Territory", 6721, 14 },
  { "Austria", 43, 15 },
  { "Azerbaijan", 994, 16 },
  { "Bahamas", 103, 17 },
  { "Bahrain", 973, 18 },
  { "Bangladesh", 880, 19 },
  { "Barbados", 104, 20 },
  { "Barbuda", 120, 21 },
  { "Belarus", 375, 22 },
  { "Belgium", 32, 23 },
  { "Belize", 501, 24 },
  { "Benin", 229, 25 },
  { "Bermuda", 105, 26 },
  { "Bhutan", 975, 27 },
  { "Bolivia", 591, 28 },
  { "Bosnia and Herzegovina", 387, 29 },
  { "Botswana", 267, 30 },
  { "Brazil", 55, 31 },
  { "British Virgin Islands", 106, 32 },
  { "Brunei", 673, 33 },
  { "Bulgaria", 359, 34 },
  { "Burkina Faso", 226, 34 },
  { "Burundi", 257, 36 },
  { "Cambodia", 855, 37 },
  { "Cameroon", 237, 38 },
  { "Canada", 107, 39 },
  { "Cape Verde Islands", 238, 40 },
  { "Cayman Islands", 108, 41 },
  { "Central African Republic", 236, 42 },
  { "Chad", 235, 43 },
  { "Chile", 56, 44 },
  { "China", 86, 45 },
  { "Christmas Island", 672, 46 },
  { "Cocos-Keeling Islands", 6101, 47 },
  { "Colombia", 57, 48 },
  { "Comoros", 2691, 49 },
  { "Congo", 242, 50 },
  { "Cook Islands", 682, 51 },
  { "Costa Rica", 506, 52 },
  { "Croatia", 385, 53 },
  { "Cuba", 53, 54 },
  { "Cyprus", 357, 55 },
  { "Czech Republic", 42, 56 },
  { "Denmark", 45, 57 },
  { "Diego Garcia", 246, 58 },
  { "Djibouti", 253, 59 },
  { "Dominica", 109, 60 },
  { "Dominican Republic", 110, 61 },
  { "Ecuador", 593, 62 },
  { "Egypt", 20, 63 },
  { "El Salvador", 503, 64 },
  { "Equatorial Guinea", 240, 65 },
  { "Eritrea", 291, 66 },
  { "Estonia", 372, 67 },
  { "Ethiopia", 251, 68 },
  { "Faeroe Islands", 298, 69 },
  { "Falkland Islands", 500, 70 },
  { "Fiji Islands", 679, 71 },
  { "Finland", 358, 72 },
  { "France", 33, 73 },
  { "French Antilles", 5901, 74 },
  { "French Guiana", 594, 75 },
  { "French Polynesia", 689, 76 },
  { "Gabon", 241, 77 },
  { "Gambia", 220, 78 },
  { "Georgia", 995, 79 },
  { "Germany", 49, 80 },
  { "Ghana", 233, 81 },
  { "Gibraltar", 350, 82 },
  { "Greece", 30, 83 },
  { "Greenland", 299, 84 },
  { "Grenada", 111, 85 },
  { "Guadeloupe", 590, 86 },
  { "Guam", 671, 87 },
  { "Guantanamo Bay", 5399, 88 },
  { "Guatemala", 502, 89 },
  { "Guinea", 224, 90 },
  { "Guinea-Bissau", 245, 91 },
  { "Guyana", 592, 92 },
  { "Haiti", 509, 93 },
  { "Honduras", 504, 94 },
  { "Hong Kong", 852, 95 },
  { "Hungary", 36, 96 },
  { "INMARSAT (Atlantic-East)", 871, 97 },
  { "INMARSAT (Atlantic-West)", 874, 98 },
  { "INMARSAT (Indian)", 873, 99 },
  { "INMARSAT (Pacific)", 872, 100 },
  { "INMARSAT", 870, 101 },
  { "Iceland", 354, 102 },
  { "India", 91, 103 },
  { "Indonesia", 62, 104 },
  { "International Freephone Service", 800, 105 },
  { "Iran", 98, 106 },
  { "Iraq", 964, 107 },
  { "Ireland", 353, 108 },
  { "Israel", 972, 109 },
  { "Italy", 39, 110 },
  { "Ivory Coast", 225, 111 },
  { "Jamaica", 112, 112 },
  { "Japan", 81, 113 },
  { "Jordan", 962, 114 },
  { "Kazakhstan", 705, 115 },
  { "Kenya", 254, 116 },
  { "Kiribati Republic", 686, 117 },
  { "Korea (North)", 850, 118 },
  { "Korea (Republic of)", 82, 119 },
  { "Kuwait", 965, 120 },
  { "Kyrgyz Republic", 706, 121 },
  { "Laos", 856, 122 },
  { "Latvia", 371, 123 },
  { "Lebanon", 961, 124 },
  { "Lesotho", 266, 125 },
  { "Liberia", 231, 126 },
  { "Libya", 218, 127 },
  { "Liechtenstein", 4101, 128 },
  { "Lithuania", 370, 129 },
  { "Luxembourg", 352, 130 },
  { "Macau", 853, 131 },
  { "Madagascar", 261, 132 },
  { "Malawi", 265, 133 },
  { "Malaysia", 60, 134 },
  { "Maldives", 960, 135 },
  { "Mali", 223, 136 },
  { "Malta", 356, 137 },
  { "Marshall Islands", 692, 138 },
  { "Martinique", 596, 139 },
  { "Mauritania", 222, 140 },
  { "Mauritius", 230, 141 },
  { "Mayotte Island", 269, 142 },
  { "Mexico", 52, 143 },
  { "Micronesia, Federated States of", 691, 144 },
  { "Moldova", 373, 145 },
  { "Monaco", 377, 146 },
  { "Mongolia", 976, 147 },
  { "Montserrat", 113, 148 },
  { "Morocco", 212, 149 },
  { "Mozambique", 258, 150 },
  { "Myanmar", 95, 151 },
  { "Namibia", 264, 152 },
  { "Nauru", 674, 153 },
  { "Nepal", 977, 154 },
  { "Netherlands Antilles", 599, 155 },
  { "Netherlands", 31, 156 },
  { "Nevis", 114, 157 },
  { "New Caledonia", 687, 158 },
  { "New Zealand", 64, 159 },
  { "Nicaragua", 505, 160 },
  { "Niger", 227, 161 },
  { "Nigeria", 234, 162 },
  { "Niue", 683, 163 },
  { "Norfolk Island", 6722, 164 },
  { "Norway", 47, 165 },
  { "Oman", 968, 166 },
  { "Pakistan", 92, 167 },
  { "Palau", 680, 168 },
  { "Panama", 507, 169 },
  { "Papua New Guinea", 675, 170 },
  { "Paraguay", 595, 171 },
  { "Peru", 51, 172 },
  { "Philippines", 63, 173 },
  { "Poland", 48, 174 },
  { "Portugal", 351, 175 },
  { "Puerto Rico", 121, 176 },
  { "Qatar", 974, 177 },
  { "Republic of Macedonia", 389, 178 },
  { "Reunion Island", 262, 179 },
  { "Romania", 40, 180 },
  { "Rota Island", 6701, 181 },
  { "Russia", 7, 182 },
  { "Rwanda", 250, 183 },
  { "Saint Lucia", 122, 184 },
  { "Saipan Island", 670, 185 },
  { "San Marino", 378, 186 },
  { "Sao Tome and Principe", 239, 187 },
  { "Saudi Arabia", 966, 188 },
  { "Senegal Republic", 221, 189 },
  { "Seychelle Islands", 248, 190 },
  { "Sierra Leone", 232, 191 },
  { "Singapore", 65, 192 },
  { "Slovak Republic", 4201, 193 },
  { "Slovenia", 386, 194 },
  { "Solomon Islands", 677, 195 },
  { "Somalia", 252, 196 },
  { "South Africa", 27, 197 },
  { "Spain", 34, 198 },
  { "Sri Lanka", 94, 199 },
  { "St. Helena", 290, 200 },
  { "St. Kitts", 115, 201 },
  { "St. Pierre and Miquelon", 508, 202 },
  { "St. Vincent and the Grenadines", 116, 203 },
  { "Sudan", 249, 204 },
  { "Suriname", 597, 205 },
  { "Swaziland", 268, 206 },
  { "Sweden", 46, 207 },
  { "Switzerland", 41, 208 },
  { "Syria", 963, 209 },
  { "Taiwan, Republic of China", 886, 210 },
  { "Tajikistan", 708, 211 },
  { "Tanzania", 255, 212 },
  { "Thailand", 66, 213 },
  { "Tinian Island", 6702, 214 },
  { "Togo", 228, 215 },
  { "Tokelau", 690, 216 },
  { "Tonga", 676, 217 },
  { "Trinidad and Tobago", 117, 218 },
  { "Tunisia", 216, 219 },
  { "Turkey", 90, 220 },
  { "Turkmenistan", 709, 221 },
  { "Turks and Caicos Islands", 118, 222 },
  { "Tuvalu", 688, 223 },
  { "USA", 1, 224 },
  { "Uganda", 256, 225 },
  { "Ukraine", 380, 226 },
  { "United Arab Emirates", 971, 227 },
  { "United Kingdom", 44, 228 },
  { "United States Virgin Islands", 123, 229 },
  { "Uruguay", 598, 230 },
  { "Uzbekistan", 711, 231 },
  { "Vanuatu", 678, 232 },
  { "Vatican City", 379, 233 },
  { "Venezuela", 58, 234 },
  { "Vietnam", 84, 235 },
  { "Wallis and Futuna Islands", 681, 236 },
  { "Western Samoa", 685, 237 },
  { "Yemen", 967, 238 },
  { "Yugoslavia", 381, 239 },
  { "Zaire", 243, 240 },
  { "Zambia", 260, 241 },
  { "Zimbabwe", 263, 242 },
};


const struct SCountry *GetCountryByCode(unsigned short _nCountryCode)
{
   // do a simple linear search as there aren't too many countries
   unsigned short i = 0;
   while (i < NUM_COUNTRIES && gCountries[i].nCode != _nCountryCode) i++;
   if (i == NUM_COUNTRIES) return NULL;
   return &gCountries[i];
}

const struct SCountry *GetCountryByIndex(unsigned short _nIndex)
{
   if (_nIndex >= NUM_COUNTRIES) return NULL;
   return (&gCountries[_nIndex]);
}

const struct SCountry *GetCountryByName(const char *_szName)
{
   unsigned short i = 0;
   while (i < NUM_COUNTRIES && strcasecmp(gCountries[i].szName, _szName)) i++;
   if (i == NUM_COUNTRIES) return NULL;
   return &gCountries[i];
}
