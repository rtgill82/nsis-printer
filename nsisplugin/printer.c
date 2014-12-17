#include <stdio.h>
#include <strsafe.h>
#include <windows.h>
#include <winspool.h>

#include "pluginapi.h"
#include "portmon.h"
#include "printdlg.h"
#include "redmonrc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRIVER_INFO_LEVEL 3
#define DRIVER_INFO DRIVER_INFO_3

#define LPPRINTER_INFO_LEVEL 4
#define LPPRINTER_INFO LPPRINTER_INFO_4

#define ARCH_X86 0x0
#define ARCH_X64 0x1

#define MAX(a,b) (((a)>(b))?(a):(b))

TCHAR tmpbuf[NSIS_VARSIZE];
HANDLE g_hInstance = NULL;
TCHAR *PrnName = NULL;
DWORD dwPrintersNum = 0;
LPPRINTER_INFO lpbPrintInfo = NULL;

/* Defined and used by Redmon 1.9. */
typedef struct reconfig_s {
    DWORD dwSize;   /* sizeof this structure */
    DWORD dwVersion;    /* version number of RedMon */
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
PrintDlgProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LPTSTR
alloc_strcpy(LPTSTR str)
{
    LPTSTR newstr;

    newstr = GlobalAlloc(GPTR, (_tcslen(str) + 1) * sizeof(TCHAR));
    _tcscpy(newstr, str);

    return newstr;
}

LPTSTR
parse_depfiles(LPTSTR str)
{
    LPTSTR multisz;
    LPTSTR p, pstr, pmultisz;
    size_t cnt;

    multisz = GlobalAlloc(GPTR, (_tcslen(str) + 2) * sizeof(TCHAR));
    cnt = 0; pstr = str; pmultisz = multisz;
    for (p = str; *p; p++) {
        if (*p == _T(';')) {
            _tcsncpy(pmultisz, pstr, cnt);
            pmultisz[cnt] = _T('\0');
            pmultisz += cnt + 1;
            cnt = 0;
            pstr = p + 1;
        } else {
            cnt++;
        }
    }

    _tcscpy(pmultisz, _T(""));
    pmultisz += 2;
    _tcscpy(pmultisz, _T(""));

    return multisz;
}

void
read_driverini(LPTSTR inifile, DRIVER_INFO *di)
{
    LPTSTR buf;

    buf = GlobalAlloc(GPTR, MAX_PATH * sizeof(TCHAR));

    GetPrivateProfileString(_T("driver"), _T("name"), NULL, buf, MAX_PATH, inifile);
    di->pName = alloc_strcpy(buf);

    GetPrivateProfileString(_T("driver"), _T("environment"), NULL, buf, MAX_PATH, inifile);
    di->pEnvironment = alloc_strcpy(buf);

    GetPrivateProfileString(_T("driver"), _T("driver"), NULL, buf, MAX_PATH, inifile);
    di->pDriverPath = alloc_strcpy(buf);

    GetPrivateProfileString(_T("driver"), _T("datafile"), NULL, buf, MAX_PATH, inifile);
    di->pDataFile = alloc_strcpy(buf);

    GetPrivateProfileString(_T("driver"), _T("configfile"), NULL, buf, MAX_PATH, inifile);
    di->pConfigFile = alloc_strcpy(buf);

    GetPrivateProfileString(_T("driver"), _T("helpfile"), NULL, buf, MAX_PATH, inifile);
    di->pHelpFile = alloc_strcpy(buf);

    GetPrivateProfileString(_T("driver"), _T("depfiles"), NULL, buf, MAX_PATH, inifile);
    di->pDependentFiles = parse_depfiles(buf);

    GlobalFree(buf);
}

void
cleanup_driverinfo(DRIVER_INFO *di)
{
    GlobalFree(di->pName);
    GlobalFree(di->pEnvironment);
    GlobalFree(di->pDriverPath);
    GlobalFree(di->pDataFile);
    GlobalFree(di->pConfigFile);
    GlobalFree(di->pHelpFile);
    GlobalFree(di->pDependentFiles);
}

size_t
max_driverfile_name(DRIVER_INFO *di)
{
    size_t filemax = 0;
    LPTSTR filename;

    filemax = MAX(filemax, _tcslen(di->pDriverPath));
    filemax = MAX(filemax, _tcslen(di->pDataFile));
    filemax = MAX(filemax, _tcslen(di->pConfigFile));
    filemax = MAX(filemax, _tcslen(di->pHelpFile));

    filename = di->pDependentFiles;
    while (TRUE) {
        size_t len = _tcslen(filename);
        if (len == 0) break;
        filemax = MAX(filemax, len);
        filename = filename + len + 1;
    }

    return filemax;
}

DWORD
copy_driverfiles(LPTSTR srcdir, DRIVER_INFO *di)
{
    DWORD pcbNeeded, err = 0;
    LPTSTR dest, src, filename, driverdir;
    size_t srcbuflen, destbuflen, filemax;
    BOOL rv;

    GetPrinterDriverDirectory(NULL, di->pEnvironment, 1, NULL, 0, &pcbNeeded);
    driverdir = GlobalAlloc(GPTR, pcbNeeded);
    GetPrinterDriverDirectory(NULL, di->pEnvironment, 1, (LPBYTE) driverdir, pcbNeeded, &pcbNeeded);

    filemax = max_driverfile_name(di);
    srcbuflen = (_tcslen(srcdir) + filemax + 2) * sizeof(TCHAR);
    src = GlobalAlloc(GPTR, srcbuflen);
    destbuflen = (_tcslen(driverdir) + filemax + 2) * sizeof(TCHAR);
    dest = GlobalAlloc(GPTR, destbuflen);

    StringCbPrintf(src, srcbuflen, _T("%s\\%s"), srcdir, di->pDriverPath);
    StringCbPrintf(dest, destbuflen, _T("%s\\%s"), driverdir, di->pDriverPath);
    rv = CopyFile(src, dest, FALSE);
    if (rv == FALSE) {
        err = GetLastError();
        goto cleanup;
    }

    StringCbPrintf(src, srcbuflen, _T("%s\\%s"), srcdir, di->pDataFile);
    StringCbPrintf(dest, destbuflen, _T("%s\\%s"), driverdir, di->pDataFile);
    CopyFile(src, dest, FALSE);
    StringCbPrintf(src, srcbuflen, _T("%s\\%s"), srcdir, di->pConfigFile);
    StringCbPrintf(dest, destbuflen, _T("%s\\%s"), driverdir, di->pConfigFile);
    CopyFile(src, dest, FALSE);
    StringCbPrintf(src, srcbuflen, _T("%s\\%s"), srcdir, di->pHelpFile);
    StringCbPrintf(dest, destbuflen, _T("%s\\%s"), driverdir, di->pHelpFile);
    CopyFile(src, dest, FALSE);

    filename = di->pDependentFiles;
    while (TRUE) {
        size_t len = _tcslen(filename);
        if (len == 0) break;
        StringCbPrintf(src, srcbuflen, _T("%s\\%s"), srcdir, filename);
        StringCbPrintf(dest, destbuflen, _T("%s\\%s"), driverdir, filename);
        rv = CopyFile(src, dest, FALSE);
        filename = filename + len + 1;
    }

cleanup:
    GlobalFree(driverdir);
    GlobalFree(dest);
    GlobalFree(src);

    return err;
}

DWORD
delete_driverfiles(DRIVER_INFO *di)
{
    DWORD pcbNeeded;
    LPTSTR driverdir, filepath, filename;
    size_t filemax, buflen;

    GetPrinterDriverDirectory(NULL, di->pEnvironment, 1, NULL, 0, &pcbNeeded);
    driverdir = GlobalAlloc(GPTR, pcbNeeded);
    GetPrinterDriverDirectory(NULL, di->pEnvironment, 1, (LPBYTE) driverdir, pcbNeeded, &pcbNeeded);

    filemax = max_driverfile_name(di);
    buflen = (_tcslen(driverdir) + filemax + 2) * sizeof(TCHAR);
    filepath = GlobalAlloc(GPTR, buflen);
    StringCbPrintf(filepath, buflen, _T("%s\\%s"), driverdir, di->pDriverPath);
    DeleteFile(filepath);
    StringCbPrintf(filepath, buflen, _T("%s\\%s"), driverdir, di->pDataFile);
    DeleteFile(filepath);
    StringCbPrintf(filepath, buflen, _T("%s\\%s"), driverdir, di->pConfigFile);
    DeleteFile(filepath);
    StringCbPrintf(filepath, buflen, _T("%s\\%s"), driverdir, di->pHelpFile);
    DeleteFile(filepath);

    filename = di->pDependentFiles;
    while (TRUE) {
        size_t len = _tcslen(filename);
        if (len == 0) break;
        StringCbPrintf(filepath, buflen, _T("%s\\%s"), driverdir, filename);
        DeleteFile(filepath);
        filename = filename + len + 1;
    }

    GlobalFree(driverdir);
    GlobalFree(filepath);
}

void __declspec(dllexport)
nsPrinterSelectDlg(HWND hwndParent,int string_size,
        LPTSTR variables, stack_t **stacktop)
{
    DWORD dwNeeded = 0;

    EXDLL_INIT();
    EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
            NULL, LPPRINTER_INFO_LEVEL, NULL, 0, &dwNeeded, &dwPrintersNum);

    lpbPrintInfo = (LPPRINTER_INFO) GlobalAlloc(GPTR, dwNeeded);
    if (lpbPrintInfo) {
        if (!EnumPrinters(
            PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
            NULL,
            LPPRINTER_INFO_LEVEL,
            (LPBYTE) lpbPrintInfo,
            dwNeeded,
            &dwNeeded,
            &dwPrintersNum
        )) {
            GlobalFree(lpbPrintInfo);
            lpbPrintInfo = NULL;
        }
    }

    DialogBox(g_hInstance,
            MAKEINTRESOURCE(IDD_PRNSEL), hwndParent, PrintDlgProc);

    pushstring(PrnName);
    GlobalFree(PrnName);
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
        pusherrormessage(_T("Unable to enumerate ports"), err);
        setuservariable(INST_R0, _T("-1"));
        return;
    }

    for (i = pcReturned - 1; i >= 0; i--)
        pushstring(portinfo[i].pName);

    GlobalFree(portinfo);
    StringCbPrintf(buffer, NSIS_VARSIZE, _T("%ld"), pcReturned);
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
    StringCbPrintf(monbuf, NSIS_VARSIZE, _T(",XcvMonitor %s"), buffer);

    rv = OpenPrinter(monbuf, &hPrinter, &pd);
    if (rv == FALSE) {
        err = GetLastError();
        pusherrormessage(_T("Unable to open XcvMonitor"), err);
        setuservariable(INST_R0, _T("0"));
        return;
    }
    GlobalFree(monbuf);

    /* Pop port name */
    popstring(buffer);
    rv = XcvData(
            hPrinter,
            _T("AddPort"),
            (PBYTE) buffer,
            ((lstrlen(buffer)+1)*sizeof(TCHAR)),
            NULL,
            0,
            &dwNeeded,
            &dwStatus
        );

    if (rv == FALSE) {
        err = GetLastError();
        pusherrormessage(_T("Unable to add port"), err);
        setuservariable(INST_R0, _T("0"));
        ClosePrinter(hPrinter);
        return;
    }

    ClosePrinter(hPrinter);
    setuservariable(INST_R0, _T("1"));
}

void __declspec(dllexport)
nsAddPrinterDriver(HWND hwndParent, int string_size,
        LPTSTR variables, stack_t **stacktop)
{
    DWORD arch;
    LPTSTR driverdir;
    LPTSTR inifile;
    DRIVER_INFO di;
    DWORD err;
    BOOL rv;

    EXDLL_INIT();
    ZeroMemory(&di, sizeof(DRIVER_INFO));

    /* System Architecture (x86 or x64) */
    popstring(buffer);
    if (!lstrcmp(buffer, _T("x64"))) {
        arch = ARCH_X64;
    } else {
        arch = ARCH_X86;
    }

    /* Driver Directory */
    popstring(buffer);
    StringCbPrintf(tmpbuf, NSIS_VARSIZE, _T("%s\\%s"),
            buffer, (arch == ARCH_X64 ? _T("x64") : _T("w32x86")));
    driverdir = alloc_strcpy(tmpbuf);

    StringCbPrintf(tmpbuf, NSIS_VARSIZE, _T("%s\\DRIVER.INI"), driverdir);
    inifile = alloc_strcpy(tmpbuf);
    read_driverini(inifile, &di);
    err = copy_driverfiles(driverdir, &di);
    if (err) {
        pusherrormessage(_T("Unable to copy driver files"), err);
        setuservariable(INST_R0, _T("0"));
        goto cleanup;
    }

    rv = AddPrinterDriver(NULL, DRIVER_INFO_LEVEL, (LPBYTE) &di);
    if (rv == FALSE) {
        err = GetLastError();
        pusherrormessage(_T("Unable to add printer driver"), err);
        setuservariable(INST_R0, _T("0"));
        goto cleanup;
    }

    delete_driverfiles(&di);
    setuservariable(INST_R0, _T("1"));
cleanup:
    cleanup_driverinfo(&di);
    GlobalFree(driverdir);
    GlobalFree(inifile);
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
    fprintf(f, "Port Name: %S\n", printerInfo.pPortName); fflush(f);

    /* Printer Driver */
    popstring(buffer);
    printerInfo.pDriverName = GlobalAlloc(GPTR, ((lstrlen(buffer)+1)*sizeof(TCHAR)));
    lstrcpy(printerInfo.pDriverName, buffer);
    fprintf(f, "Driver Name: %S\n", printerInfo.pDriverName); fflush(f);

    hPrinter = AddPrinter(NULL, 2, (LPBYTE) &printerInfo);
    fprintf(f, "hPrinter = %p\n", hPrinter); fflush(f);
    if (hPrinter == NULL) {
        GlobalFree(printerInfo.pPrinterName);
        GlobalFree(printerInfo.pPortName);
        GlobalFree(printerInfo.pDriverName);

        err = GetLastError();
        pusherrormessage(_T("Unable to add printer"), err);
        setuservariable(INST_R0, _T("0"));
        return;
    }

    fclose(f);
    GlobalFree(printerInfo.pPrinterName);
    GlobalFree(printerInfo.pPortName);
    GlobalFree(printerInfo.pDriverName);
    ClosePrinter(hPrinter);
    setuservariable(INST_R0, _T("1"));
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
        pusherrormessage(_T("Unable to open printer"), err);
        setuservariable(INST_R0, _T("0"));
        return;
    }

    rv = DeletePrinter(hPrinter);
    if (rv == FALSE) {
        err = GetLastError();
        pusherrormessage(_T("Unable to delete printer"), err);
        ClosePrinter(hPrinter);
        setuservariable(INST_R0, _T("-1"));
        return;
    }

    ClosePrinter(hPrinter);
    setuservariable(INST_R0, _T("1"));
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
    lstrcpy(config.szDescription, _T("Redirected Port"));

    rv = OpenPrinter(_T(",XcvMonitor Redirected Port"), &hPrinter, &pd);
    if (rv == FALSE) {
        err = GetLastError();
        pusherrormessage(_T("Unable to open XcvMonitor"), err);
        setuservariable(INST_R0, _T("0"));
        return;
    }

    rv = XcvData(
            hPrinter,
            _T("SetConfig"),
            (PBYTE)(&config),
            sizeof(RECONFIG),
            NULL,
            0,
            &dwNeeded,
            &dwStatus
        );
    if (rv == FALSE) {
        err = GetLastError();
        pusherrormessage(_T("Unable to configure port"), err);
        setuservariable(INST_R0, _T("0"));
        return;
    }

    setuservariable(INST_R0, _T("1"));
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
        pusherrormessage(_T("Not enough buffer space for default printer name"), 0);
        setuservariable(INST_R0, _T("0"));
        return;
    }

    rv = GetDefaultPrinter(buffer, &dwNeeded);
    if (rv == FALSE) {
        err = GetLastError();
        pusherrormessage(_T("Unable to get default printer"), err);
        setuservariable(INST_R0, _T("0"));
        return;
    }

    pushstring(buffer);
    setuservariable(INST_R0, _T("1"));
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
        pusherrormessage(_T("Unable to set default printer"), err);
        setuservariable(INST_R0, _T("0"));
        return;
    }

    setuservariable(INST_R0, _T("1"));
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

        for (idx = 0; idx < dwPrintersNum; idx++) {
            SendMessage(prnCombo, CB_ADDSTRING, 0,
                    (LPARAM) lpbPrintInfo[idx].pPrinterName);
        }

        SendMessage(prnCombo, CB_SETCURSEL, 0, 0);
        PrnName = (TCHAR *) GlobalAlloc(GMEM_FIXED, 64);
        lstrcpy(PrnName, _T(""));
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            if ((idx = SendMessage(prnCombo, CB_GETCURSEL, 0, 0)) == 0) {
                lstrcpy(PrnName, _T(""));
            } else {
                len = SendMessage(prnCombo, CB_GETLBTEXTLEN, idx, 0);
                PrnName = (TCHAR *)
                    GlobalReAlloc(PrnName, (len+1)*sizeof(TCHAR), 0);
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
