#include "CoreFoundation/CFString.h"
#include "Testing.h"
#include "../CFTesting.h"

int main (void)
{
  CFStringRef str1;
  CFStringRef str2;
  
  str1 = CFStringCreateWithFormat (NULL, NULL,
    __CFStringMakeConstantString("%c %c%c"), 'r', 'u', 'n');
  str2 = __CFStringMakeConstantString ("r un");
  PASS_CFEQ(str1, str2, "Characters are formatted correctly");
  CFRelease(str1);
  CFRelease(str2);
  
  str1 = CFStringCreateWithFormat (NULL, NULL,
    __CFStringMakeConstantString("'%2$04d %d'"), 3, 0x00a4);
  str2 = __CFStringMakeConstantString ("'0164 3'");
  PASS_CFEQ(str1, str2, "Integers are formatted correctly");
  CFRelease(str1);
  CFRelease(str2);
  
  str1 = CFStringCreateWithFormat (NULL, NULL,
    __CFStringMakeConstantString("%f %1$-5.2g %e"), 5.5, .000003);
  str2 = __CFStringMakeConstantString ("5.500000 5.5   3.000000e-06");
  PASS_CFEQ(str1, str2, "Float/Doubles are formatted correctly");
  CFRelease(str1);
  CFRelease(str2);
  
  str1 = CFStringCreateWithFormat (NULL, NULL,
    __CFStringMakeConstantString("%x %1$#08X %hhx"), 10788, 256);
  str2 = __CFStringMakeConstantString ("2a24 0X00002A24 0");
  PASS_CFEQ(str1, str2, "Hexs are formatted correctly");
  CFRelease(str1);
  CFRelease(str2);
  
  str1 = CFStringCreateWithFormat (NULL, NULL,
    __CFStringMakeConstantString("%o %1$#06o %hho"), 10788, 256);
  str2 = __CFStringMakeConstantString ("25044 0025044 0");
  PASS_CFEQ(str1, str2, "Octals are formatted correctly");
  CFRelease(str1);
  CFRelease(str2);
  
  return 0;
}