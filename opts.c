/********************04/28/2003*********************
This is a fix to the problem with GetTicks() under
Windows which returns a DWORD only allocating
4 bytes of storage, so if your uptime is
greater than 2^32/1000 seconds (ticks are in millisecs)
then it experiences a rollover.  I guess Windows
developers never considered a non WinXP/2k/NT box to
have any uptime greater than 49.7 days.

At any rate this returns the seconds elapsed since
the last reboot through the performance counter
and there are no sanity checks as to whether you have
the performance counter dll present (WinXP/NT/2k) or
not... it's on the to-do list, as is future improvements
to give XiRCON functionality to obtain other counter
information.

Call the proc under XiRCON as 'opts' and from there,
if using kano, you can run it by 'since', e.g.:
set uptime [since [opts]]
Which will return something along the lines of:
[ka] Tcl: 2mn 4d 15h 51m 1s

This fix returns a 64 bit wide int (LONGLONG) that'll
guarantee accurate non-rollover uptime reports for
up to 2^64 seconds, or 584942417355.07203247 years...
I don't think I'll need to write another patch
any time soon.

									 - Matt Saladna
						  msaladna@apisnetworks.com
***************************************************/ 	
#define VERSION "1.0.0"
#define DEFTITLE "Opts, it's opts, cha cha cha."
#define WIN32_LEAN_AND_MEAN
#define TCL_OK 0
#define TCL_ERROR 1
#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE  1
#endif
#endif
#ifndef PDH_CSTATUS_VALID_DATA
#define	PDH_CSTATUS_VALID_DATA 0
#endif

#include <windows.h>
#include <winbase.h>
#include <pdh.h>

/* Tcl typedefs, for Tcl usage without their library */
typedef int Tcl_CmdProc(void *cd, void *interp, int argc, char *argv[]);
typedef void (*dyn_CreateCommand)(void*, const char*, Tcl_CmdProc*, void*, void*);
typedef int (*dyn_AppendResult)(void*, ...);
/* Globals */
//extern static char *curstr = (char*)calloc(sizeof(char)*255 + 1 /* max buffer size of XiRCON window + null termination */ );

// handle to this DLL, passed in to DllMain() and needed for several API functions
HINSTANCE hInstance;

// handle to xircntcl.dll, wherein resides Tcl, which we need to call functions from
HINSTANCE hXircTcl;

// reference count, kept to know when we're being loaded the first time, or unloaded the last time
static int refcount = 0;

// function pointers for the two functions we'll need to call in Tcl
dyn_CreateCommand Tcl_CreateCommand;
dyn_AppendResult Tcl_AppendResult;


/* Prototypes */
int Opts(void *cd, void *interp, int argc, char *argv[]);
void PUTDEBUG(char *errormsg);

// debug!
void PUTDEBUG(char *errormsg) {
	MessageBoxEx(NULL,errormsg,"opts.dll debug:",MB_OK,LANG_ENGLISH);
}

// main dll
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {
	switch(dwReason) {
	case DLL_PROCESS_ATTACH:
		refcount++;
		if (refcount == 1) {
			hInstance = hInst; /* save instance handle */
			/* get handle to xircntcl for Tcl functions. */
			hXircTcl = GetModuleHandle("xtcl.dll");

			if (hXircTcl) {
				Tcl_CreateCommand = (dyn_CreateCommand)GetProcAddress(hXircTcl, "_Tcl_CreateCommand");
				Tcl_AppendResult = (dyn_AppendResult)GetProcAddress(hXircTcl, "_Tcl_AppendResult");
			} else {
				Tcl_CreateCommand = NULL;
				Tcl_AppendResult = NULL;
			}
			/* need no data structures initialized, but if we did, this would
			   be the place */
		}
		break;
	case DLL_PROCESS_DETACH:
		refcount--;
		if (refcount == 0) {
			Tcl_CreateCommand = NULL;
			Tcl_AppendResult = NULL;
		}
		break;
	/* don't care about DLL_THREAD_* */
	}
	return TRUE; /* successful load */
}


/* Main Function */

int Opts(void *cd, void *interp, int argc, char *argv[]) {

	HQUERY				hQuery;
	PDH_STATUS           pdhStatus;
	PDH_FMT_COUNTERVALUE *lpItemBuffer;
	HCOUNTER				hCounter;
	CHAR					szCounterPath[45] = TEXT("\\System\\System Up Time");
	TCHAR szBuffer[128];

	if (PdhValidatePath(szCounterPath) == PDH_CSTATUS_VALID_DATA) {
		// Open a query object.
		pdhStatus = PdhOpenQuery (0, 0, &hQuery);

		// Allocate the counter handle array. Allocate room for
		//  one handle per command line arg, not including the
		//  executable file name.
		lpItemBuffer = (PDH_FMT_COUNTERVALUE *) GlobalAlloc 
						(GPTR, sizeof(PDH_FMT_COUNTERVALUE));

		// Add one counter that will provide the data.
		pdhStatus = PdhAddCounter (hQuery, szCounterPath, 0, &hCounter);
		pdhStatus = PdhCollectQueryData (hQuery);	

		pdhStatus = PdhGetFormattedCounterValue (hCounter,
					PDH_FMT_LONG,
					(LPDWORD)NULL,
					lpItemBuffer);
		wsprintf(szBuffer,"%lu",
			lpItemBuffer->largeValue
		);
		pdhStatus = PdhCloseQuery (hQuery); 
	} else {
		wsprintf(szBuffer,"Counter path not found (this really shouldn't happen).");
		(*Tcl_AppendResult)(interp, szBuffer, NULL);
		return TCL_ERROR;
	}	

	if (!Tcl_AppendResult)
		return TCL_ERROR;

	(*Tcl_AppendResult)(interp, szBuffer, NULL);
		return TCL_OK;
}

// dll init export, run by XiRCON's Tcl interp when dll is loaded..
int __declspec(dllexport) Opts_Init(void *interp) {
	if (Tcl_CreateCommand && Tcl_AppendResult) {
		// register command with xircons tcl interpreter
		(*Tcl_CreateCommand)(interp, "opts", Opts, NULL, NULL);
	}
	return TCL_OK;
}