#ifndef _DEBUG_
#define _DEBUG_

//extern void Debug(char *debug, ...);
extern void reset_stream();
//extern void Debug(char *debug, ...);
extern void _trace_(char *debug, ...);

// Found in rsp_debugger.cpp...
void Debugger_Open ();



#define USE_ASSERT			1

#if USE_ASSERT
#	include <assert.h>

#	define _assert_(_a_)	if (!(_a_)){\
								char szError [512];\
								sprintf(szError,"Error localized at...\n\n  Line:\t %d\n  File:\t %s\n  Time:\t %s\n\nIgnore and continue?",__LINE__,__FILE__,__TIMESTAMP__);\
								OutputDebugString (szError);\
								if(MessageBox(NULL,szError,"*** Assertion Report ***",MB_YESNO|MB_ICONERROR)==IDNO)__asm{int 3};\
							}
#	define _assert_msg_(_a_,_type_,_desc_)	if (!(_a_)){\
											char szError [512];\
											sprintf (szError,"Type: %s \nDesc.: %s\n\nIgnore and continue?",_type_,_desc_);\
											OutputDebugString (szError);\
											if(MessageBox (NULL,szError,"*** Fatal Error ***",MB_YESNO|MB_ICONERROR)==IDNO)__asm{int 3};\
										}
#else
#	define _assert_(_a_)
#	define _assert_msg_(_a_,_b_,_c_)
#endif




#endif
