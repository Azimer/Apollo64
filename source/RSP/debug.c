#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void Debug(char *debug, ...);


void reset_stream()
{
	FILE *stream = fopen("d:/RSPAUDIO.LOG.txt","wt");
	fprintf(stream,"starting...\n");
	fclose(stream);

}


long c = 0;
void _trace_(char *debug, ...)
{
	va_list argptr;
	char	text[1024];
	char	text1[1024];

//	return;

	va_start(argptr, debug);
	vsprintf(text, debug, argptr);
	va_end(argptr);

	sprintf(text1, "%i: %s", c, text);
	OutputDebugString(text1);
}


/*
void Debug(char *debug, ...)
{
	va_list argptr;
	char	text[1024];
	FILE *stream = fopen("d:/RSPAUDIO.LOG.txt","at");


	va_start(argptr, debug);
	vsprintf(text, debug, argptr);
	va_end(argptr);

//	MessageBox( NULL, text, "HLE", MB_OK);
	fprintf(stream,"%s", text);

	fclose(stream);
}
*/



