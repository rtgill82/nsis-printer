#include <stdio.h>
#include <windows.h>
#include <winspool.h>

#include "pluginapi.h"
#include "portmon.h"
#include "printdlg.h"
#include "redmonrc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRIVER_INFO_VERSION 3
#define DRIVER_INFO DRIVER_INFO_3

HANDLE g_hInstance = NULL;
TCHAR *PrnName = NULL;
DWORD dwPrintersNum = 0, dwLevel = 4;
LPPRINTER_INFO_4 lpbPrintInfo4 = NULL;
LPPRINTER_INFO_5 lpbPrintInfo5 = NULL;

/* Defined and used by Redmon 1.9. */
typedef struct reconfig_s {
    DWORD dwSize;	/* sizeof this structure */
    DWORD dwVersion;	/* version number of RedMon */
    TCHAR szPortName[MAXSTR];
    TCHAR szDescription[MAXSTR];
    TCHAR szCommand[MAXSTR];
    TCHAR szArguments[MAXSTR];
    TCHAR szPrinter[MAXSTR];
    DWORD dwOutput;
    DWORD dwShow;
    DWORD dwRunUser;
    DWORD dwDelay;
    DWORD dwLogFileUse;
    TCHAR szLogFileName[MAXSTR];
    DWORD dwLogFileDebug;
    DWORD dwPrintError;
};

static BOOL CALLBACK
PrintDlgProc    ( HWND   hwnd,
                  UINT   msg,
                  WPARAM wParam,
                  LPARAM lParam    );

void __declspec(dllexport)
nsPrinterSelectDlg(HWND hwndParent,int string_size,
        LPTSTR variables, stack_t **stacktop)
{
    OSVERSIONINFO osVerInfo;
    DWORD dwNeeded = 0;
    LPBYTE lpbPrinters = NULL;

	EXDLL_INIT();

    /* Detect OS Version */
    ZeroMemory(&osVerInfo, sizeof(osVerInfo));
    osVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osVerInfo);

    switch (osVerInfo.dwPlatformId) {
    case VER_PLATFORM_WIN32_NT:
        dwLevel = 4;   /* Use printer info level 4 for NT. */
        break;
    case VER_PLATFORM_WIN32_WINDOWS:
        dwLevel = 5;   /* Use printer info level 5 for 9x. */
        break;
    }

    EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
            NULL, dwLevel, NULL, 0, &dwNeeded, &dwPrintersNum);

    if (dwLevel == 4) {
        lpbPrintInfo4 = (LPPRINTER_INFO_4) GlobalAlloc(GPTR, dwNeeded);
        lpbPrinters = (LPBYTE) lpbPrintInfo4;
    } else {
        lpbPrintInfo5 = (LPPRINTER_INFO_5) GlobalAlloc(GPTR, dwNeeded);
        lpbPrinters = (LPBYTE) lpbPrintInfo5;
    }

    if (lpbPrinters) {
        if (!EnumPrinters(
            PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
            NULL,
            dwLevel,
            lpbPrinters,
            dwNeeded,
            &dwNeeded,
            &dwPrintersNum
        )) {
            GlobalFree(lpbPrinters);
            lpbPrinters = NULL;
        }
    }

    DialogBox(g_hInstance,
            MAKEINTRESOURCE(IDD_PRNSEL), hwndParent, PrintDlgProc);

    pushstring(PrnName);
    GlobalFree(PrnName);
    GlobalFree(lpbPrinters);
}

void __declspec(dllexport)
nsEnumPorts(HWND hwndParent, int string_size,
        LPTSTR variables, stack_t **stacktop)
{
	int i;
    DWORD pcbNeeded, pcReturned;
    PORT_INFO_1 *portinfo;
    DWORD err; BOOL rv;

	EXDLL_INIT();
    EnumPorts(NULL, 1, NULL, 0, &pcbNeeded, &pcReturned);
    portinfo = GlobalAlloc(GPTR, pcbNeeded);

    rv = EnumPorts(
            NULL,
            1,
            (PBYTE) portinfo,
            pcbNeeded,
            &pcbNeeded,
            &pcReturned
        );

    if (rv != TRUE) {
        err = GetLastError();
        pusherrormessage(TEXT("Unable to enumerate ports"), err);
        setuservariable(INST_R0, TEXT("-1"));
        return;
    }

	for (i = pcReturned - 1; i >= 0; i--)
		pushstring(portinfo[i].pName);

	GlobalFree(portinfo);
    wsprintf(buffer, TEXT("%ld"), pcReturned);
    setuservariable(INST_R0, buffer);
}

void __declspec(dllexport)
nsAddPort(HWND hwndParent, int string_size,
        LPTSTR variables, stack_t **stacktop)
{
    HANDLE hPrinter;
	TCHAR *monbuf;
	DWORD dwNeeded, dwStatus;
	DWORD err; BOOL rv;

	PRINTER_DEFAULTS pd;
	pd.pDatatype = NULL;
	pd.pDevMode = NULL;
	pd.DesiredAccess = SERVER_ACCESS_ADMINISTER;

	EXDLL_INIT();
    /* Pop print monitor */
    popstring(buffer);
	monbuf = GlobalAlloc(GPTR, NSIS_VARSIZE);
	wsprintf(monbuf, TEXT(",XcvMonitor %s"), buffer);

    rv = OpenPrinter(monbuf, &hPrinter, &pd);
	if (rv == FALSE) {
		err = GetLastError();
        pusherrormessage(TEXT("Unable to open XcvMonitor"), err);
		setuservariable(INST_R0, TEXT("0"));
		return;
	}
	GlobalFree(monbuf);

    /* Pop port name */
	popstring(buffer);
    rv = XcvData(
            hPrinter,
            TEXT("AddPort"),
            (PBYTE) buffer,
            ((lstrlen(buffer)+1)*sizeof(TCHAR)),
            NULL,
            0,
            &dwNeeded,
            &dwStatus
        );

	if (rv == FALSE) {
		err = GetLastError();
		pusherrormessage(TEXT("Unable to add port"), err);
		setuservariable(INST_R0, TEXT("0"));
		ClosePrinter(hPrinter);
		return;
	}

	ClosePrinter(hPrinter);
	setuservariable(INST_R0, TEXT("1"));
}

void __declspec(dllexport)
nsAddPrinterDriver(HWND hwndParent, int string_size,
		LPTSTR variables, stack_t **stacktop)
{
	DWORD err;

	EXDLL_INIT();
}

void __declspec(dllexport)
nsAddPrinter(HWND hwndParent, int string_size,
        LPTSTR variables, stack_t **stacktop)
{
    PRINTER_INFO_2 printerInfo;
    HANDLE hPrinter;
    DWORD err;

	EXDLL_INIT();
    ZeroMemory(&printerInfo, sizeof(PRINTER_INFO_2));

	FILE *f;
	f = fopen("C:\\nsisprinter.log", "w");

    /* Printer Name */
    popstring(buffer);
    printerInfo.pPrinterName = GlobalAlloc(GPTR, ((lstrlen(buffer)+1)*sizeof(TCHAR)));
    lstrcpy(printerInfo.pPrinterName, buffer);
	fprintf(f, "Printer Name: %S\n", printerInfo.pPrinterName); fflush(f);

    /* Port Name */
    popstring(buffer);
    printerInfo.pPortName = GlobalAlloc(GPTR, ((lstrlen(buffer)+1)*sizeof(TCHAR)));
    lstrcpy(printerInfo.pPortName, buffer);
	fprintf(f, "Port Name: %s\n", printerInfo.pPortName); fflush(f);

    /* Printer Driver */
    popstring(buffer);
    printerInfo.pDriverName = GlobalAlloc(GPTR, ((lstrlen(buffer)+1)*sizeof(TCHAR)));
    lstrcpy(printerInfo.pDriverName, buffer);
	fprintf(f, "Driver Name: %s\n", printerInfo.pDriverName); fflush(f);

    hPrinter = AddPrinter(NULL, 2, (LPBYTE) &printerInfo);
	fprintf(f, "hPrinter = %p\n", hPrinter); fflush(f);
    if (hPrinter == NULL) {
        GlobalFree(printerInfo.pPrinterName);
        GlobalFree(printerInfo.pPortName);
        GlobalFree(printerInfo.pDriverName);

        err = GetLastError();
        pusherrormessage(TEXT("Unable to add printer"), err);
        setuservariable(INST_R0, TEXT("0"));
        return;
    }

	fclose(f);
    GlobalFree(printerInfo.pPrinterName);
    GlobalFree(printerInfo.pPortName);
    GlobalFree(printerInfo.pDriverName);
    ClosePrinter(hPrinter);
    setuservariable(INST_R0, TEXT("1"));
}

void __declspec(dllexport)
nsDeletePrinter(HWND hwndParent, int string_size,
        LPTSTR variables, stack_t **stacktop)
{
    HANDLE hPrinter;
    BOOL rv;
    DWORD err;

	PRINTER_DEFAULTS pd;
	pd.pDatatype = NULL;
	pd.pDevMode = NULL;
	pd.DesiredAccess = PRINTER_ALL_ACCESS;

	EXDLL_INIT();
    popstring(buffer);
    rv = OpenPrinter(buffer, &hPrinter, &pd);
    if (rv == FALSE) {
		err = GetLastError();
        pusherrormessage(TEXT("Unable to open printer"), err);
		setuservariable(INST_R0, TEXT("0"));
		return;
    }

    rv = DeletePrinter(hPrinter);
    if (rv == FALSE) {
        err = GetLastError();
        pusherrormessage(TEXT("Unable to delete printer"), err);
		ClosePrinter(hPrinter);
        setuservariable(INST_R0, TEXT("-1"));
        return;
    }

	ClosePrinter(hPrinter);
    setuservariable(INST_R0, TEXT("1"));
}

void __declspec(dllexport)
nsRedMonConfigurePort(HWND hwndParent, int string_size,
        LPTSTR variables, stack_t **stacktop)
{
    HANDLE hPrinter;
    RECONFIG config;
	PRINTER_DEFAULTS pd;
	DWORD dwNeeded, dwStatus;
	DWORD err; BOOL rv;

	EXDLL_INIT();
    ZeroMemory(&config, sizeof(RECONFIG));
    config.dwSize = sizeof(RECONFIG);
    config.dwVersion = VERSION_NUMBER;
    config.dwOutput = OUTPUT_SELF;
    config.dwShow = FALSE;
    config.dwRunUser = TRUE;
    config.dwDelay = DEFAULT_DELAY;
    config.dwLogFileDebug = FALSE;

	pd.pDatatype = NULL;
	pd.pDevMode = NULL;
	pd.DesiredAccess = SERVER_ACCESS_ADMINISTER;

    popstring(buffer);
    lstrcpy(config.szPortName, buffer);
    popstring(buffer);
    lstrcpy(config.szCommand, buffer);
    lstrcpy(config.szDescription, TEXT("Redirected Port"));

    rv = OpenPrinter(TEXT(",XcvMonitor Redirected Port"), &hPrinter, &pd);
    if (rv == FALSE) {
        err = GetLastError();
        pusherrormessage(TEXT("Unable to open XcvMonitor"), err);
        setuservariable(INST_R0, TEXT("0"));
        return;
    }

    rv = XcvData(
            hPrinter,
            TEXT("SetConfig"),
            (PBYTE)(&config),
            sizeof(RECONFIG),
            NULL,
            0,
            &dwNeeded,
            &dwStatus
        );
	if (rv == FALSE) {
		err = GetLastError();
		pusherrormessage(TEXT("Unable to configure port"), err);
		setuservariable(INST_R0, TEXT("0"));
		return;
	}

	setuservariable(INST_R0, TEXT("1"));
}

void __declspec(dllexport)
nsGetDefaultPrinter(HWND hwndParent, int string_size,
        LPTSTR variables, stack_t **stacktop)
{
	DWORD dwNeeded, err;
    BOOL rv;

	EXDLL_INIT();
    dwNeeded = 0;
    GetDefaultPrinter(NULL, &dwNeeded);
    if (dwNeeded > NSIS_VARSIZE) {
        pusherrormessage(TEXT("Not enough buffer space for default printer name"), 0);
        setuservariable(INST_R0, TEXT("0"));
        return;
    }

    rv = GetDefaultPrinter(buffer, &dwNeeded);
    if (rv == FALSE) {
        err = GetLastError();
        pusherrormessage(TEXT("Unable to get default printer"), err);
        setuservariable(INST_R0, TEXT("0"));
        return;
    }

    pushstring(buffer);
    setuservariable(INST_R0, TEXT("1"));
}

void __declspec(dllexport)
nsSetDefaultPrinter(HWND hwndParent, int string_size,
        LPTSTR variables, stack_t **stacktop)
{
	DWORD err;
    BOOL rv;

	EXDLL_INIT();
    popstring(buffer);
    rv = SetDefaultPrinter(buffer);
    if (rv == FALSE) {
        err = GetLastError();
        pusherrormessage(TEXT("Unable to set default printer"), err);
        setuservariable(INST_R0, TEXT("0"));
        return;
    }

    setuservariable(INST_R0, TEXT("1"));
}

BOOL WINAPI
DllMain(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	g_hInstance = hInst;
    return TRUE;
}

static BOOL CALLBACK
PrintDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    unsigned int idx, len;
    static HWND prnCombo = NULL;

    switch (msg) {
    case WM_INITDIALOG:
        prnCombo = GetDlgItem(hwnd, IDC_PRNCOMBO);
        SendMessage(prnCombo, CB_RESETCONTENT, 0, 0);
        SendMessage(prnCombo,
                CB_ADDSTRING, 0, (LPARAM) "None (Printing Disabled)");

        switch (dwLevel) {
        case 4:
            for (idx = 0; idx < dwPrintersNum; idx++)
                SendMessage(prnCombo,CB_ADDSTRING,
                        0, (LPARAM) lpbPrintInfo4[idx].pPrinterName);
            break;
        case 5:
            for (idx = 0; idx < dwPrintersNum; idx++)
                SendMessage(prnCombo, CB_ADDSTRING,
                        0, (LPARAM) lpbPrintInfo5[idx].pPrinterName);
            break;
        }

        SendMessage(prnCombo, CB_SETCURSEL, 0, 0);
        PrnName = (TCHAR *) GlobalAlloc(GMEM_FIXED, 64);
        lstrcpy(PrnName, TEXT(""));
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            if ((idx = SendMessage(prnCombo, CB_GETCURSEL, 0, 0)) == 0) {
                lstrcpy(PrnName, TEXT(""));
            } else {
                len = SendMessage(prnCombo, CB_GETLBTEXTLEN, idx, 0);
                PrnName = (TCHAR *)
                    GlobalReAlloc(PrnName, (len + 1)*sizeof(TCHAR), 0);
                SendMessage(prnCombo, CB_GETLBTEXT, idx, (LPARAM) PrnName);
            }

            EndDialog(hwnd, IDOK);
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

#ifdef __cplusplus
} /* extern "C" */
#endif