#ifndef KOAL_H
#define KOAL_H

typedef int (*OUTCHAR)(const char c);
typedef int (*OUTSTRING)(const char* c);

struct kernelOutputAbstractionLayer
{
	char name[32];
	OUTCHAR outChar;
	OUTSTRING outString;
};

void koalInit();
int koalAttach(struct kernelOutputAbstractionLayer* koal);
int koalSelectCurrentOutputByName(const char* name);

int koalPrintChar(const char c);
int koalPrint(const char* str);

#endif
