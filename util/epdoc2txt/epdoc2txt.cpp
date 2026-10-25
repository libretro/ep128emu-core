
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <exception>
#include <stdexcept>

static void errorMessage(const char *msg)
{
  throw std::runtime_error(std::string(msg));
}

// Enterprise character set to Unicode conversion tables
// from http://epbas.lgb.hu/result-chset.html

static const unsigned short epCharacterSetTable[10][128] = {
  // BRD
  {
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,     // 0x20
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,     // 0x28
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,     // 0x30
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,     // 0x38
    0x00A7, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,     // 0x40
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,     // 0x48
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,     // 0x50
    0x0058, 0x0059, 0x005A, 0x00C4, 0x00D6, 0x00DC, 0x005E, 0x005F,     // 0x58
    0x00B4, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,     // 0x60
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,     // 0x68
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,     // 0x70
    0x0078, 0x0079, 0x007A, 0x00E4, 0x00F6, 0x00FC, 0x00DF, 0x232B,     // 0x78
    0x00A9, 0x0023, 0x00C4, 0x00D6, 0x00DC, 0x00C5, 0x00D8, 0x00C6,     // 0x80
    0x00E9, 0x00C7, 0x00E0, 0x2191, 0x2192, 0x03A9, 0x25AE, 0x0192,     // 0x88
    0x03C0, 0x03A3, 0x00E4, 0x00F6, 0x00FC, 0x00E5, 0x00F8, 0x00E6,     // 0x90
    0x00E8, 0x00A7, 0x2600, 0x2193, 0x2190, 0x03BC, 0x2190, 0x25A0      // 0x98
  },
  // EPPHUN
  {
    0x0020, 0x0021, 0x0022, 0x00A3, 0x0024, 0x0025, 0x0026, 0x0027,     // 0x20
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,     // 0x28
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,     // 0x30
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,     // 0x38
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,     // 0x40
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,     // 0x48
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,     // 0x50
    0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,     // 0x58
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,     // 0x60
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,     // 0x68
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,     // 0x70
    0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x232B,     // 0x78
    0x00C1, 0x00C9, 0x00ED, 0x00F3, 0x00F6, 0x0151, 0x00FA, 0x00FC,     // 0x80
    0x0171, 0x250C, 0x2514, 0x2502, 0x251C, 0x252C, 0x25AE, 0x253C,     // 0x88
    0x00E1, 0x00E9, 0x00ED, 0x00F3, 0x00F6, 0x0151, 0x00FA, 0x00FC,     // 0x90
    0x0171, 0x2510, 0x2518, 0x2500, 0x2524, 0x2534, 0x2190, 0x003F      // 0x98
  },
  // HFONT
  {
    0x0020, 0x0021, 0x0022, 0x00A3, 0x0024, 0x0025, 0x0026, 0x0027,     // 0x20
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,     // 0x28
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,     // 0x30
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,     // 0x38
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,     // 0x40
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,     // 0x48
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,     // 0x50
    0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,     // 0x58
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,     // 0x60
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,     // 0x68
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,     // 0x70
    0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x232B,     // 0x78
    0x00A9, 0x00E1, 0x255D, 0x255A, 0x00ED, 0x00CD, 0x00F3, 0x00F6,     // 0x80
    0x0151, 0x00DC, 0x00FA, 0x00FC, 0x0171, 0x2551, 0x003F, 0x0170,     // 0x88
    0x03C0, 0x00C1, 0x00D3, 0x00E9, 0x00D6, 0x00DA, 0x2550, 0x00C9,     // 0x90
    0x2557, 0x0150, 0x2550, 0x2193, 0x2554, 0x2551, 0x2190, 0x25A0      // 0x98
  },
  // HUN
  {
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,     // 0x20
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,     // 0x28
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,     // 0x30
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,     // 0x38
    0x00C1, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,     // 0x40
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,     // 0x48
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,     // 0x50
    0x0058, 0x0059, 0x005A, 0x00C9, 0x00D6, 0x00DC, 0x005E, 0x005F,     // 0x58
    0x00E1, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,     // 0x60
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,     // 0x68
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,     // 0x70
    0x0078, 0x0079, 0x007A, 0x00E9, 0x00F6, 0x00FC, 0x00F3, 0x232B,     // 0x78
    0x00A9, 0x0023, 0x00C4, 0x00D6, 0x00DC, 0x00C5, 0x00D8, 0x00C6,     // 0x80
    0x00E9, 0x00C7, 0x00E0, 0x2191, 0x2192, 0x03A9, 0x25AE, 0x0192,     // 0x88
    0x03C0, 0x03A3, 0x00E4, 0x00F6, 0x00FC, 0x00E5, 0x00F8, 0x00E6,     // 0x90
    0x00E8, 0x00A7, 0x2600, 0x2193, 0x2190, 0x03BC, 0x2190, 0x25A0      // 0x98
  },
  // UK
  {
    0x0020, 0x0021, 0x0022, 0x00A3, 0x0024, 0x0025, 0x0026, 0x0027,     // 0x20
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,     // 0x28
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,     // 0x30
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,     // 0x38
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,     // 0x40
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,     // 0x48
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,     // 0x50
    0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,     // 0x58
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,     // 0x60
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,     // 0x68
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,     // 0x70
    0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x232B,     // 0x78
    0x00A9, 0x0023, 0x00C4, 0x00D6, 0x00DC, 0x00C5, 0x00D8, 0x00C6,     // 0x80
    0x00E9, 0x00C7, 0x00E0, 0x2191, 0x2192, 0x03A9, 0x25AE, 0x0192,     // 0x88
    0x03C0, 0x03A3, 0x00E4, 0x00F6, 0x00FC, 0x00E5, 0x00F8, 0x00E6,     // 0x90
    0x00E8, 0x00A7, 0x2600, 0x2193, 0x2190, 0x03BC, 0x2190, 0x25A0      // 0x98
  },
  // ISO 646 encodings for WordStar documents
  // BRD
  {
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x00B4,     // 0x20
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,     // 0x28
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,     // 0x30
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,     // 0x38
    0x00A7, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,     // 0x40
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,     // 0x48
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,     // 0x50
    0x0058, 0x0059, 0x005A, 0x00C4, 0x00D6, 0x00DC, 0x005E, 0x005F,     // 0x58
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,     // 0x60
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,     // 0x68
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,     // 0x70
    0x0078, 0x0079, 0x007A, 0x00E4, 0x00F6, 0x00FC, 0x00DF, 0x232B,     // 0x78
    0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,     // 0x80
    0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,     // 0x88
    0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,     // 0x90
    0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F      // 0x98
  },
  // HUN
  {
    0x0020, 0x0021, 0x0022, 0x0023, 0x00A4, 0x0025, 0x0026, 0x00B4,     // 0x20
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,     // 0x28
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,     // 0x30
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,     // 0x38
    0x00C1, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,     // 0x40
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,     // 0x48
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,     // 0x50
    0x0058, 0x0059, 0x005A, 0x00C9, 0x00D6, 0x00DC, 0x005E, 0x005F,     // 0x58
    0x00E1, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,     // 0x60
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,     // 0x68
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,     // 0x70
    0x0078, 0x0079, 0x007A, 0x00E9, 0x00F6, 0x00FC, 0x02DD, 0x232B,     // 0x78
    0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,     // 0x80
    0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,     // 0x88
    0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,     // 0x90
    0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F      // 0x98
  },
  // HUN
  {
    0x0020, 0x0021, 0x0022, 0x0023, 0x00A4, 0x0025, 0x0026, 0x00B4,     // 0x20
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,     // 0x28
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,     // 0x30
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,     // 0x38
    0x00C1, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,     // 0x40
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,     // 0x48
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,     // 0x50
    0x0058, 0x0059, 0x005A, 0x00C9, 0x00D6, 0x00DC, 0x005E, 0x005F,     // 0x58
    0x00E1, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,     // 0x60
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,     // 0x68
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,     // 0x70
    0x0078, 0x0079, 0x007A, 0x00E9, 0x00F6, 0x00FC, 0x02DD, 0x232B,     // 0x78
    0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,     // 0x80
    0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,     // 0x88
    0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,     // 0x90
    0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F      // 0x98
  },
  // HUN
  {
    0x0020, 0x0021, 0x0022, 0x0023, 0x00A4, 0x0025, 0x0026, 0x00B4,     // 0x20
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,     // 0x28
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,     // 0x30
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,     // 0x38
    0x00C1, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,     // 0x40
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,     // 0x48
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,     // 0x50
    0x0058, 0x0059, 0x005A, 0x00C9, 0x00D6, 0x00DC, 0x005E, 0x005F,     // 0x58
    0x00E1, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,     // 0x60
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,     // 0x68
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,     // 0x70
    0x0078, 0x0079, 0x007A, 0x00E9, 0x00F6, 0x00FC, 0x02DD, 0x232B,     // 0x78
    0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,     // 0x80
    0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,     // 0x88
    0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,     // 0x90
    0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F      // 0x98
  },
  // UK
  {
    0x0020, 0x0021, 0x0022, 0x00A3, 0x0024, 0x0025, 0x0026, 0x0027,     // 0x20
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,     // 0x28
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,     // 0x30
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,     // 0x38
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,     // 0x40
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,     // 0x48
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,     // 0x50
    0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,     // 0x58
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,     // 0x60
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,     // 0x68
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,     // 0x70
    0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x232B,     // 0x78
    0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,     // 0x80
    0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,     // 0x88
    0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,     // 0x90
    0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F      // 0x98
  }
};

static void epChrToUTF8(std::vector< unsigned char >& outBuf, unsigned char c,
                        int chrSet)
{
  unsigned short  w = c & 0x7F;
  if (chrSet >= 0 && chrSet <= 9 && c >= 0x20)
    w = epCharacterSetTable[chrSet][(c - 0x20) & 0x7F];
  if (!w)
    w = 0x002E;
  if (w < 0x0080) {
    outBuf.push_back((unsigned char) w);
  }
  else if (w < 0x0800) {
    outBuf.push_back((unsigned char) (0xC0 | (w >> 6)));
    outBuf.push_back((unsigned char) (0x80 | (w & 0x3F)));
  }
  else {
    outBuf.push_back((unsigned char) (0xE0 | (w >> 12)));
    outBuf.push_back((unsigned char) (0x80 | ((w >> 6) & 0x3F)));
    outBuf.push_back((unsigned char) (0x80 | (w & 0x3F)));
  }
}

// ----------------------------------------------------------------------------

#define TEXT_FORMAT_CLEAR       0xF8
#define TEXT_FORMAT_COLOR1      0xF9
#define TEXT_FORMAT_COLOR2      0xFA
#define TEXT_FORMAT_COLOR3      0xFB
#define TEXT_FORMAT_BOLD        0xFC
#define TEXT_FORMAT_ITALIC      0xFD
#define TEXT_FORMAT_UNDERLINE   0xFE

#define TEXT_FORMAT_EXTENDED    0xFF
#define TEXT_FORMAT_STRIKE      0xF8
#define TEXT_FORMAT_SUB         0xF9
#define TEXT_FORMAT_SUP         0xFA

static const char *htmlHeader =
  "<html>\n"
  "  <head>\n"
  "    <meta http-equiv=\"Content-Type\" "
  "content=\"text/html; charset=UTF-8\">\n"
  "  </head>\n"
  "  <body text=\"#40FF40\" bgcolor=\"#000000\">\n"
  "    <pre>";

static const char *htmlFooter =
  "</pre>\n"
  "  </body>\n"
  "</html>\n";

static void writeHTMLFormatTags(std::vector< unsigned char >& outBuf,
                                unsigned short c, unsigned char& formatFlags)
{
  char    tmpBuf[64];
  char    *s = &(tmpBuf[0]);
  switch (c) {
  case TEXT_FORMAT_CLEAR:
    if (formatFlags & 0x40)
      s = s + std::sprintf(s, "</sup>");
    if (formatFlags & 0x20)
      s = s + std::sprintf(s, "</sub>");
    if (formatFlags & 0x10)
      s = s + std::sprintf(s, "</s>");
    if (formatFlags & 0x08)
      s = s + std::sprintf(s, "</u>");
    if (formatFlags & 0x04)
      s = s + std::sprintf(s, "</i>");
    if (formatFlags & 0x02)
      s = s + std::sprintf(s, "</b>");
    if (formatFlags & 0x01)
      s = s + std::sprintf(s, "</font>");
    formatFlags = 0x00;
    break;
  case TEXT_FORMAT_COLOR1:
  case TEXT_FORMAT_COLOR2:
  case TEXT_FORMAT_COLOR3:
    if (formatFlags & 0x01)
      s = s + std::sprintf(s, "</font>");
    formatFlags = formatFlags | 0x01;
    if (c == TEXT_FORMAT_COLOR1)
      s = s + std::sprintf(s, "<font color=\"#FF6060\">");
    else if (c == TEXT_FORMAT_COLOR2)
      s = s + std::sprintf(s, "<font color=\"#7070FF\">");
    else
      s = s + std::sprintf(s, "<font color=\"#00FFFF\">");
    break;
  case TEXT_FORMAT_BOLD:
    if (!(formatFlags & 0x02)) {
      formatFlags = formatFlags | 0x02;
      s = s + std::sprintf(s, "<b>");
    }
    break;
  case TEXT_FORMAT_ITALIC:
    if (!(formatFlags & 0x04)) {
      formatFlags = formatFlags | 0x04;
      s = s + std::sprintf(s, "<i>");
    }
    break;
  case TEXT_FORMAT_UNDERLINE:
    if (!(formatFlags & 0x08)) {
      formatFlags = formatFlags | 0x08;
      s = s + std::sprintf(s, "<u>");
    }
    break;
  case 0xFF00 | TEXT_FORMAT_STRIKE:
    if (!(formatFlags & 0x10)) {
      formatFlags = formatFlags | 0x10;
      s = s + std::sprintf(s, "<s>");
    }
    break;
  case 0xFF00 | TEXT_FORMAT_SUB:
    if (!(formatFlags & 0x20)) {
      formatFlags = formatFlags | 0x20;
      s = s + std::sprintf(s, "<sub>");
    }
    break;
  case 0xFF00 | TEXT_FORMAT_SUP:
    if (!(formatFlags & 0x40)) {
      formatFlags = formatFlags | 0x40;
      s = s + std::sprintf(s, "<sup>");
    }
    break;
  }
  *s = '\0';
  for (size_t i = 0; i < size_t(s - &(tmpBuf[0])); i++)
    outBuf.push_back((unsigned char) tmpBuf[i]);
}

static void convertTextToHTML(std::vector< unsigned char >& buf)
{
  std::vector< unsigned char >  tmpBuf;
  tmpBuf.insert(tmpBuf.end(), buf.begin(), buf.end());
  buf.clear();
  for (size_t i = 0; htmlHeader[i] != '\0'; i++)
    buf.push_back((unsigned char) htmlHeader[i]);
  // bit 0 = color
  // bit 1 = bold
  // bit 2 = italic
  // bit 3 = underline
  // bit 4 = strike-through
  // bit 5 = subscript
  // bit 6 = superscript
  unsigned char prvFormatFlags = 0x00;
  unsigned char formatFlags = 0x00;
  unsigned char prvTextColor = 0;
  unsigned char textColor = 0;
  for (size_t i = 0; i < tmpBuf.size(); i++) {
    unsigned char c = tmpBuf[i];
    if (c >= TEXT_FORMAT_CLEAR) {
      switch (c) {
      case TEXT_FORMAT_CLEAR:
        formatFlags = 0x00;
        break;
      case TEXT_FORMAT_COLOR1:
      case TEXT_FORMAT_COLOR2:
      case TEXT_FORMAT_COLOR3:
        formatFlags = formatFlags | 0x01;
        textColor = c;
        break;
      case TEXT_FORMAT_BOLD:
        formatFlags = formatFlags | 0x02;
        break;
      case TEXT_FORMAT_ITALIC:
        formatFlags = formatFlags | 0x04;
        break;
      case TEXT_FORMAT_UNDERLINE:
        formatFlags = formatFlags | 0x08;
        break;
      case TEXT_FORMAT_EXTENDED:
        if ((i + 1) < tmpBuf.size()) {
          i++;
          switch (tmpBuf[i]) {
          case TEXT_FORMAT_STRIKE:
            formatFlags = formatFlags | 0x10;
            break;
          case TEXT_FORMAT_SUB:
            formatFlags = formatFlags | 0x20;
            break;
          case TEXT_FORMAT_SUP:
            formatFlags = formatFlags | 0x40;
            break;
          }
        }
        break;
      }
      continue;
    }
    if (c < 0x20) {
      if (c == 0x0D) {
        if ((i + 1) < tmpBuf.size() && tmpBuf[i + 1] == 0x0A)
          continue;
        c = 0x0A;
      }
      if (c == 0x09 || c == 0x0A) {
        if (c == 0x0A) {
          if (prvFormatFlags)
            writeHTMLFormatTags(buf, TEXT_FORMAT_CLEAR, prvFormatFlags);
          prvFormatFlags = 0x00;
          formatFlags = 0x00;
        }
      }
      else {
        continue;
      }
    }
    if (formatFlags != prvFormatFlags) {
      if (prvFormatFlags & ~formatFlags) {
        writeHTMLFormatTags(buf, TEXT_FORMAT_CLEAR, prvFormatFlags);
        prvFormatFlags = 0x00;
      }
      unsigned char changeMask = formatFlags & ~prvFormatFlags;
      if ((changeMask & 0x01) != 0 ||
          ((formatFlags & 0x01) != 0 && textColor != prvTextColor)) {
        prvTextColor = textColor;
        writeHTMLFormatTags(buf, textColor, prvFormatFlags);
      }
      if (changeMask & 0x02)
        writeHTMLFormatTags(buf, TEXT_FORMAT_BOLD, prvFormatFlags);
      if (changeMask & 0x04)
        writeHTMLFormatTags(buf, TEXT_FORMAT_ITALIC, prvFormatFlags);
      if (changeMask & 0x08)
        writeHTMLFormatTags(buf, TEXT_FORMAT_UNDERLINE, prvFormatFlags);
      if (changeMask & 0x10)
        writeHTMLFormatTags(buf, 0xFF00 | TEXT_FORMAT_STRIKE, prvFormatFlags);
      if (changeMask & 0x20)
        writeHTMLFormatTags(buf, 0xFF00 | TEXT_FORMAT_SUB, prvFormatFlags);
      if (changeMask & 0x40)
        writeHTMLFormatTags(buf, 0xFF00 | TEXT_FORMAT_SUP, prvFormatFlags);
      prvFormatFlags = formatFlags;
    }
    if (char(c) == '&' || char(c) == '<' || char(c) == '>') {
      const char  *s = (char(c) == '&' ?
                        "&amp;" : (char(c) == '<' ? "&lt;" : "&gt;"));
      while (*s) {
        buf.push_back((unsigned char) *s);
        s++;
      }
    }
    else {
      buf.push_back(c);
    }
  }
  if (prvFormatFlags)
    writeHTMLFormatTags(buf, TEXT_FORMAT_CLEAR, prvFormatFlags);
  for (size_t i = 0; htmlFooter[i] != '\0'; i++)
    buf.push_back((unsigned char) htmlFooter[i]);
}

// ----------------------------------------------------------------------------

static void convertHEASSFile(std::vector< unsigned char >& outBuf,
                             const std::vector< unsigned char >& inBuf,
                             int chrSet)
{
  if (inBuf.size() < 17 ||
      inBuf[0] != 0x00 || inBuf[1] != 0x80 ||
      inBuf.size() < (size_t(inBuf[16]) + 24)) {
    errorMessage("invalid .HEA file header");
  }
  const unsigned char *p = &(inBuf.front()) + (size_t(inBuf[16]) + 24);
  size_t  nBytes = inBuf.size() - (size_t(inBuf[16]) + 24);
  while (true) {
    if (nBytes < 2)
      errorMessage("unexpected end of .HEA file");
    size_t  blockSize = size_t(p[0]) | (size_t(p[1]) << 8);
    if (nBytes < blockSize)
      errorMessage("unexpected end of .HEA file");
    p = p + 2;
    nBytes = nBytes - blockSize;
    if (blockSize < 4) {
      if (blockSize < 1)
        break;
      errorMessage("invalid .HEA file block header");
    }
    blockSize = blockSize - 4;
    p = p + 2;
    size_t  lineLen = 0;
    while (blockSize-- > 0) {
      if (!lineLen) {
        lineLen = *(p++) & 0x7F;
        if (!lineLen)
          outBuf.push_back(0x0A);       // empty line
        continue;
      }
      lineLen--;
      unsigned char c = *(p++);
      if (c >= 0x21 && c <= 0x9F) {
        epChrToUTF8(outBuf, c, chrSet);
      }
      else {
        size_t  rleLen = (size_t(c) + 0x61) & 0xFF;
        do {
          outBuf.push_back(0x20);
        } while (--rleLen);
      }
      if (!lineLen)
        outBuf.push_back(0x0A);         // end of line
    }
    if (lineLen)
      outBuf.push_back(0x0A);
  }
}

static void convertHWPFile(std::vector< unsigned char >& outBuf,
                           const std::vector< unsigned char >& inBuf,
                           int chrSet, bool htmlFormat)
{
  if (inBuf.size() < 46 ||
      std::strncmp(reinterpret_cast< const char * >(&(inBuf.front())),
                   "HWP.DOC 1.", 10) != 0) {
    errorMessage("invalid .HWP file header");
  }
  size_t  nBlocks = inBuf[11];
  const unsigned char *p = &(inBuf.front()) + 46;
  size_t  nBytes = inBuf.size() - 46;
  while (nBlocks-- > 0) {
    if (nBytes < 2)
      errorMessage("unexpected end of .HWP file");
    size_t  blockSize = size_t(p[0]) | (size_t(p[1]) << 8);
    if (nBytes < (blockSize + 2))
      errorMessage("unexpected end of .HWP file");
    p = p + 2;
    nBytes = nBytes - (blockSize + 2);
    size_t  lineLen = 0;
    unsigned char formatFlags = 0x00;   // 1: underline, 2: bold
    while (blockSize-- > 0) {
      if (!lineLen) {
        lineLen = *(p++);
        if (!lineLen) {
          if (blockSize)
            errorMessage("invalid line length in .HWP file");
          break;
        }
        if (lineLen > (blockSize + 1))
          errorMessage("invalid line length in .HWP file");
        if (lineLen <= 2) {
          if (lineLen == 2) {
            blockSize--;
            p++;
          }
          lineLen = 0;
          outBuf.push_back(0x0A);       // empty line
          continue;
        }
        lineLen = lineLen - 2;
        blockSize = blockSize - 2;
        p++;
      }
      lineLen--;
      unsigned char c = *(p++);
      if (c >= 0x20 && c <= 0x9F) {
        epChrToUTF8(outBuf, c, chrSet);
      }
      else if (c > 0xA0) {
        size_t  rleLen = c - 0xA0;
        do {
          outBuf.push_back(0x20);
        } while (--rleLen);
      }
      else if (c == 0x0C && htmlFormat) {
        formatFlags = formatFlags ^ 0x01;
        outBuf.push_back(
            !(formatFlags & 0x01) ? TEXT_FORMAT_CLEAR : TEXT_FORMAT_UNDERLINE);
        if ((formatFlags & 0x02) != 0 && lineLen != 0)
          outBuf.push_back(TEXT_FORMAT_BOLD);
      }
      else if (c == 0x0D && htmlFormat) {
        formatFlags = formatFlags ^ 0x02;
        outBuf.push_back(
            !(formatFlags & 0x02) ? TEXT_FORMAT_CLEAR : TEXT_FORMAT_BOLD);
        if ((formatFlags & 0x01) != 0 && lineLen != 0)
          outBuf.push_back(TEXT_FORMAT_UNDERLINE);
      }
      if (!lineLen) {
        if (formatFlags) {
          formatFlags = 0x00;
          outBuf.push_back(TEXT_FORMAT_CLEAR);
        }
        outBuf.push_back(0x0A);         // end of line
      }
    }
    if (lineLen) {
      if (formatFlags) {
        formatFlags = 0x00;
        outBuf.push_back(TEXT_FORMAT_CLEAR);
      }
      outBuf.push_back(0x0A);
    }
  }
  if (htmlFormat)
    convertTextToHTML(outBuf);
}

static void convertASMONFile(std::vector< unsigned char >& outBuf,
                             const std::vector< unsigned char >& inBuf,
                             int chrSet)
{
  if (inBuf.size() < 1 || inBuf[0] != 0x01)
    errorMessage("invalid ASMON file header");
  const unsigned char *p = &(inBuf.front()) + 1;
  size_t  nBytes = inBuf.size() - 1;
  while (nBytes-- > 0) {
    unsigned char c = *(p++);
    if (c < 0x80) {
      if (c == 0x1A)
        continue;
      if (c == 0x0D)
        c = 0x0A;
      epChrToUTF8(outBuf, c, chrSet);
    }
    else {
      size_t  rleLen = c - 0x7F;
      while (rleLen--)
        outBuf.push_back(0x20);
    }
  }
}

static void convertWPFile(std::vector< unsigned char >& outBuf,
                          const std::vector< unsigned char >& inBuf,
                          int chrSet, bool htmlFormat)
{
  if (inBuf.size() < 16 ||
      !(inBuf[0] == 0x00 && inBuf[1] == 0x08)) {
    errorMessage("invalid .WP file header");
  }
  const unsigned char *p = &(inBuf.front()) + 16;
  size_t  nBytes = inBuf.size() - 16;
  while (nBytes > 0) {
    if (nBytes < 2)
      errorMessage("invalid line data in .WP file");
    unsigned char lm = p[0];
    unsigned char lineFlags = p[1];
    p = p + 2;
    nBytes = nBytes - 2;
    if (lm == 0x00 && lineFlags == 0x0A)
      break;                            // end of file
    if (!(lm >= 0x81 && lineFlags >= 0x80 && p[0] == 0x03))
      errorMessage("invalid line data in .WP file");
    while (lm > 0x81) {
      outBuf.push_back(0x20);
      lm--;
    }
    if ((lineFlags & 0x03) != 0 && htmlFormat)
      outBuf.push_back(TEXT_FORMAT_COLOR1 + ((lineFlags & 0x03) - 1));
    while (nBytes > 0) {
      unsigned char c = *(p++);
      nBytes--;
      if (c >= 0x20 && c <= 0xA0) {
        epChrToUTF8(outBuf, c, chrSet);
      }
      else if (c < 0x20) {
        if (c == 0x02 || c == 0x04 || c == 0x05) {
          if ((lineFlags & 0x03) != 0 && htmlFormat)
            outBuf.push_back(TEXT_FORMAT_CLEAR);
          lineFlags = 0;
          outBuf.push_back(0x0A);
          break;
        }
      }
      else {
        errorMessage("invalid character in .WP file");
      }
    }
    if (lineFlags) {
      if ((lineFlags & 0x03) != 0 && htmlFormat)
        outBuf.push_back(TEXT_FORMAT_CLEAR);
      outBuf.push_back(0x0A);
    }
  }
  if (htmlFormat)
    convertTextToHTML(outBuf);
}

static void convertWSFile(std::vector< unsigned char >& outBuf,
                          const std::vector< unsigned char >& inBuf,
                          int chrSet, bool htmlFormat)
{
  const unsigned char *p = &(inBuf.front());
  size_t  nBytes = inBuf.size();
  bool    ignoreFlag = false;
  bool    firstChar = true;
  // bit 0 = alternate font
  // bit 1 = bold
  // bit 2 = double-strike
  // bit 3 = italic
  // bit 4 = underline
  // bit 5 = overstrike
  // bit 6 = subscript
  // bit 7 = superscript
  unsigned char formatFlags = 0x00;
  while (nBytes-- > 0) {
    unsigned char c = *(p++);
    if (c == 0x1B && nBytes >= 2 && p[1] == 0x1C) {
      c = p[0];
      p = p + 2;
      nBytes = nBytes - 2;
    }
    else {
      c = c & 0x7F;
    }
    if (ignoreFlag) {
      ignoreFlag = (c != 0x0A);
      continue;
    }
    if (c == 0x2E && firstChar) {
      if (nBytes >= 4) {
        char    c0 = char(p[0] & 0x5F);
        char    c1 = char(p[1] & 0x7F);
        if ((c0 == 'H' &&
             (c1 == 'E' || c1 == 'e' || (c1 >= '1' && c1 <= '5'))) ||
            (c0 == 'F' &&
             (c1 == 'O' || c1 == 'o' || (c1 >= '1' && c1 <= '5')))) {
          char    c2 = char(p[2] & 0x7F);
          char    c3 = char(p[3] & 0x7F);
          if (c2 < '0' && c2 != '\r' && c2 != '\n' &&
              c3 != '\r' && c3 != '\n') {
            p = p + (c2 == ' ' ? 3 : 2);
            nBytes = nBytes - 3;
            continue;
          }
        }
      }
      ignoreFlag = true;
      continue;
    }
    if (c != 0x1A)
      firstChar = false;
    if (c < 0x20) {
      unsigned char prvFormatFlags = formatFlags;
      switch (c) {
      case 0x01:                        // toggle alternate font
        formatFlags = formatFlags ^ 0x01;
        break;
      case 0x02:                        // toggle bold font
        formatFlags = formatFlags ^ 0x02;
        break;
      case 0x04:                        // toggle double-strike
        formatFlags = formatFlags ^ 0x04;
        break;
      case 0x09:                        // tab
        outBuf.push_back(c);
        break;
      case 0x1A:                        // EOF
        if (firstChar)
          break;
      case 0x0A:                        // line feed
        firstChar = true;
        if (formatFlags != 0 && htmlFormat) {
          formatFlags = 0x00;
          outBuf.push_back(TEXT_FORMAT_CLEAR);
        }
        outBuf.push_back(0x0A);
        break;
      case 0x13:                        // toggle underline
        formatFlags = formatFlags ^ 0x10;
        break;
      case 0x14:                        // toggle superscript mode
        formatFlags = formatFlags ^ 0x80;
        break;
      case 0x16:                        // toggle subscript mode
        formatFlags = formatFlags ^ 0x40;
        break;
      case 0x18:                        // toggle overstrike
        formatFlags = formatFlags ^ 0x20;
        break;
      case 0x19:                        // toggle italic font
        formatFlags = formatFlags ^ 0x08;
        break;
      }
      if (!htmlFormat) {
        formatFlags = 0x00;
      }
      else if (formatFlags != prvFormatFlags) {
        if (prvFormatFlags & ~formatFlags) {
          prvFormatFlags = 0x00;
          outBuf.push_back(TEXT_FORMAT_CLEAR);
        }
        unsigned char changeMask = ~prvFormatFlags & formatFlags;
        if (changeMask & 0x01)
          outBuf.push_back(TEXT_FORMAT_COLOR1);
        if (changeMask & 0x06)
          outBuf.push_back(TEXT_FORMAT_BOLD);
        if (changeMask & 0x08)
          outBuf.push_back(TEXT_FORMAT_ITALIC);
        if (changeMask & 0x10)
          outBuf.push_back(TEXT_FORMAT_UNDERLINE);
        if (changeMask & 0x20) {
          outBuf.push_back(TEXT_FORMAT_EXTENDED);
          outBuf.push_back(TEXT_FORMAT_STRIKE);
        }
        if (changeMask & 0x40) {
          outBuf.push_back(TEXT_FORMAT_EXTENDED);
          outBuf.push_back(TEXT_FORMAT_SUB);
        }
        if (changeMask & 0x80) {
          outBuf.push_back(TEXT_FORMAT_EXTENDED);
          outBuf.push_back(TEXT_FORMAT_SUP);
        }
      }
    }
    else {
      epChrToUTF8(outBuf, c, chrSet + 5);
    }
  }
  if (!firstChar) {
    if (formatFlags != 0 && htmlFormat)
      outBuf.push_back(TEXT_FORMAT_CLEAR);
    outBuf.push_back(0x0A);
  }
  if (htmlFormat)
    convertTextToHTML(outBuf);
}

// ----------------------------------------------------------------------------

static bool checkFileNameExtension(const char *s, const char *t)
{
  size_t  len1 = std::strlen(s);
  size_t  len2 = std::strlen(t);
  if (len1 < len2)
    return false;
  s = s + (len1 - len2);
  for ( ; *s != '\0'; s++, t++) {
    char    c = *s;
    if (c >= 'A' && c <= 'Z')
      c = c + ('a' - 'A');
    if (c != *t)
      return false;
  }
  return true;
}

int main(int argc, char **argv)
{
  if (argc != 3 && argc != 4) {
    std::printf("Usage: %s [-brd|-epphun|-hfont|-hun|-uk] INFILE OUTFILE\n",
                argv[0]);
    return -1;
  }
  std::FILE *f = (std::FILE *) 0;
  int       chrSet = 4;
  bool      htmlFormat = (checkFileNameExtension(argv[argc - 1], ".htm") ||
                          checkFileNameExtension(argv[argc - 1], ".html"));
  if (argc > 3) {
    if (std::strcmp(argv[1], "-brd") == 0)
      chrSet = 0;
    else if (std::strcmp(argv[1], "-epphun") == 0)
      chrSet = 1;
    else if (std::strcmp(argv[1], "-hfont") == 0)
      chrSet = 2;
    else if (std::strcmp(argv[1], "-hun") == 0)
      chrSet = 3;
    else if (std::strcmp(argv[1], "-uk") == 0)
      chrSet = 4;
    else
      errorMessage("invalid command line option");
  }
  try {
    std::vector< unsigned char >  inBuf;
    std::vector< unsigned char >  outBuf;
    // read input file
    f = std::fopen(argv[argc - 2], "rb");
    if (!f)
      errorMessage("error opening input file");
    {
      int     c;
      while ((c = std::fgetc(f)) != EOF)
        inBuf.push_back((unsigned char) (c & 0xFF));
    }
    std::fclose(f);
    f = (std::FILE *) 0;
    // check file type:
    //   0 = text
    //   1 = HEASS
    //   2 = HWP
    //   3 = ASMON
    //   4 = WP
    //   5 = WordStar
    int     fileType = 0;
    if (inBuf.size() >= 46 &&
        std::strncmp(reinterpret_cast< char * >(&(inBuf.front())),
                     "HWP.DOC 1.", 10) == 0) {
      fileType = 2;
    }
    else if (inBuf.size() >= 16 && inBuf[0] == 0x00 && inBuf[1] == 0x80) {
      fileType = 1;
    }
    else if (inBuf.size() >= 16 && inBuf[0] == 0x00 && inBuf[1] == 0x08) {
      fileType = 4;
    }
    else if (inBuf.size() > 1 && inBuf[0] == 0x01) {
      fileType = 3;
    }
    else if (inBuf.size() > 32 && inBuf[0] == 0x02) {
      // ASMON/FENAS with "intelligent save" header
      fileType = 3;
      inBuf.erase(inBuf.begin() + 1, inBuf.begin() + 32);
      inBuf[0] = 0x01;
    }
    else if (inBuf.size() > 0x0210 && inBuf[0] == 0xFF && inBuf[1] == 0xBF) {
      // ASMON/FENAS with SAVE.XR header
      fileType = 3;
      inBuf.erase(inBuf.begin() + 1, inBuf.begin() + 0x0210);
      if (inBuf[inBuf.size() - 1] == 0x45)
        inBuf.resize(inBuf.size() - 1);
      inBuf[0] = 0x01;
    }
    else {
      // bit 0 = have NUL character
      // bit 1 = have LF character
      // bit 2 = have CR character
      // bit 3 = have character with bit 7 set
      unsigned char fileFlags = 0x00;
      for (size_t i = 0; i < inBuf.size(); i++) {
        unsigned char c = inBuf[i];
        switch (c) {
        case 0x00:
          fileFlags = fileFlags | 0x01;
          break;
        case 0x0A:
          fileFlags = fileFlags | 0x02;
          break;
        case 0x0D:
          fileFlags = fileFlags | 0x04;
          break;
        default:
          fileFlags = fileFlags | ((c >> 4) & 0x08);
          break;
        }
      }
      if (fileFlags == 0x0C) {
        // ASMON without header byte
        fileType = 3;
        inBuf.insert(inBuf.begin(), 0x01);
      }
      else if (fileFlags == 0x0E &&
               (checkFileNameExtension(argv[argc - 2], ".doc") ||
                checkFileNameExtension(argv[argc - 2], ".ws"))) {
        // WordStar
        fileType = 5;
      }
    }
    switch (fileType) {
    case 1:
      convertHEASSFile(outBuf, inBuf, chrSet);
      break;
    case 2:
      convertHWPFile(outBuf, inBuf, chrSet, htmlFormat);
      break;
    case 3:
      convertASMONFile(outBuf, inBuf, chrSet);
      break;
    case 4:
      convertWPFile(outBuf, inBuf, chrSet, htmlFormat);
      break;
    case 5:
      convertWSFile(outBuf, inBuf, chrSet, htmlFormat);
      break;
    default:
      // default to plain text
      for (size_t i = 0; i < inBuf.size(); i++) {
        unsigned char c = inBuf[i];
        if (c == 0x1A)
          continue;
        if (c == 0x0D) {
          if ((i + 1) < inBuf.size() && inBuf[i + 1] == 0x0A)
            continue;
          c = 0x0A;
        }
        epChrToUTF8(outBuf, c, chrSet);
      }
      break;
    }
    // write output file
    f = std::fopen(argv[argc - 1], "w");
    if (!f)
      errorMessage("error opening output file");
    if (std::fwrite(&(outBuf.front()), sizeof(unsigned char), outBuf.size(), f)
        != outBuf.size() ||
        std::fflush(f) != 0) {
      errorMessage("error writing output file");
    }
    int     err = std::fclose(f);
    f = (std::FILE *) 0;
    if (err != 0)
      errorMessage("error writing output file");
  }
  catch (std::exception& e) {
    if (f)
      std::fclose(f);
    std::fprintf(stderr, " *** %s: %s\n", argv[0], e.what());
    return -1;
  }
  return 0;
}

