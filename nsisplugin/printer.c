#include <stdio.h>
#include <windows.h>
#include <winspool.h>
#include <tchar.h>

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
#define BUF_SIZE (string_size * sizeof(TCHAR))

static HANDLE g_hInstance = NULL;
static LPTSTR prnName = NULL;
static DWORD dwPrintersNum = 0;
static LPPRINTER_INFO lpbPrintInfo = NULL;

/* Defined and used by Redmon 1.9. */
struct reconfig_s {
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

static void NSISCALL
pusherrormessage (TCHAR *errbuf, int bufsiz, LPCTSTR msg, DWORD err)
{
  int len;
  lstrcpy (errbuf, msg);

  if (err > 0)
    {
      lstrcat (errbuf, _T (": "));
      len = lstrlen(errbuf);
      FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM,
             NULL,
             err,
             0,
             errbuf + len,
             (bufsiz - len) * sizeof(TCHAR),
             NULL
        );
    }

  pushstring (errbuf);
}

static LPTSTR
alloc_strcpy(LPTSTR str)
{
    LPTSTR newstr;

    newstr = GlobalAlloc(GPTR, (_tcslen(str) + 1) * sizeof(TCHAR));
    _tcscpy(newstr, str);

    return newstr;
}

static LPTSTR
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

static void
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

static void
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

static size_t
max_driverfile_name(DRIVER_INFO *di)
{
    LPTSTR filename;
    size_t filemax = 0;

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

static DWORD
copy_driverfiles(LPTSTR srcdir, DRIVER_INFO *di)
{
    DWORD pcbNeeded;
    LPTSTR filename;
    size_t srcbuflen, destbuflen, filemax;
    BOOL rv;

    DWORD err = 0;
    LPTSTR dest = NULL, src = NULL, driverdir = NULL;

    GetPrinterDriverDirectory(NULL, di->pEnvironment, 1, NULL, 0, &pcbNeeded);
    driverdir = GlobalAlloc(GPTR, pcbNeeded);
    GetPrinterDriverDirectory(NULL, di->pEnvironment, 1, (LPBYTE) driverdir, pcbNeeded, &pcbNeeded);

    filemax = max_driverfile_name(di);
    srcbuflen = (_tcslen(srcdir) + filemax + 2) * sizeof(TCHAR);
    src = GlobalAlloc(GPTR, srcbuflen);
    destbuflen = (_tcslen(driverdir) + filemax + 2) * sizeof(TCHAR);
    dest = GlobalAlloc(GPTR, destbuflen);

    _sntprintf(src, srcbuflen, _T("%s\\%s"), srcdir, di->pDriverPath);
    _sntprintf(dest, destbuflen, _T("%s\\%s"), driverdir, di->pDriverPath);
    rv = CopyFile(src, dest, FALSE);
    if (rv == FALSE) {
        err = GetLastError();
        goto cleanup;
    }

    _sntprintf(src, srcbuflen, _T("%s\\%s"), srcdir, di->pDataFile);
    _sntprintf(dest, destbuflen, _T("%s\\%s"), driverdir, di->pDataFile);
    rv = CopyFile(src, dest, FALSE);
    if (rv == FALSE) {
        err = GetLastError();
        goto cleanup;
    }

    _sntprintf(src, srcbuflen, _T("%s\\%s"), srcdir, di->pConfigFile);
    _sntprintf(dest, destbuflen, _T("%s\\%s"), driverdir, di->pConfigFile);
    rv = CopyFile(src, dest, FALSE);
    if (rv == FALSE) {
        err = GetLastError();
        goto cleanup;
    }

    _sntprintf(src, srcbuflen, _T("%s\\%s"), srcdir, di->pHelpFile);
    _sntprintf(dest, destbuflen, _T("%s\\%s"), driverdir, di->pHelpFile);
    rv = CopyFile(src, dest, FALSE);
    if (rv == FALSE) {
        err = GetLastError();
        goto cleanup;
    }

    filename = di->pDependentFiles;
    while (TRUE) {
        size_t len = _tcslen(filename);
        if (len == 0) break;
        _sntprintf(src, srcbuflen, _T("%s\\%s"), srcdir, filename);
        _sntprintf(dest, destbuflen, _T("%s\\%s"), driverdir, filename);
        rv = CopyFile(src, dest, FALSE);
        if (rv == FALSE) {
            err = GetLastError();
            goto cleanup;
        }
        filename = filename + len + 1;
    }

cleanup:
    GlobalFree(driverdir);
    GlobalFree(dest);
    GlobalFree(src);

    return err;
}

static DWORD
delete_driverfiles(DRIVER_INFO *di)
{
    DWORD pcbNeeded;
    LPTSTR filename;
    size_t filemax, buflen;
    BOOL rv;

    DWORD err = 0;
    LPTSTR driverdir = NULL, filepath = NULL;

    GetPrinterDriverDirectory(NULL, di->pEnvironment, 1, NULL, 0, &pcbNeeded);
    driverdir = GlobalAlloc(GPTR, pcbNeeded);
    GetPrinterDriverDirectory(NULL, di->pEnvironment, 1, (LPBYTE) driverdir, pcbNeeded, &pcbNeeded);

    filemax = max_driverfile_name(di);
    buflen = (_tcslen(driverdir) + filemax + 2) * sizeof(TCHAR);
    filepath = GlobalAlloc(GPTR, buflen);
    _sntprintf(filepath, buflen, _T("%s\\%s"), driverdir, di->pDriverPath);
    rv = DeleteFile(filepath);
    if (rv == FALSE) {
        err = GetLastError();
        goto cleanup;
    }

    _sntprintf(filepath, buflen, _T("%s\\%s"), driverdir, di->pDataFile);
    rv = DeleteFile(filepath);
    if (rv == FALSE) {
        err = GetLastError();
        goto cleanup;
    }

    _sntprintf(filepath, buflen, _T("%s\\%s"), driverdir, di->pConfigFile);
    rv = DeleteFile(filepath);
    if (rv == FALSE) {
        err = GetLastError();
        goto cleanup;
    }

    _sntprintf(filepath, buflen, _T("%s\\%s"), driverdir, di->pHelpFile);
    rv = DeleteFile(filepath);
    if (rv == FALSE) {
        err = GetLastError();
        goto cleanup;
    }

    filename = di->pDependentFiles;
    while (TRUE) {
        size_t len = _tcslen(filename);
        if (len == 0) break;
        _sntprintf(filepath, buflen, _T("%s\\%s"), driverdir, filename);
        rv = DeleteFile(filepath);
        if (rv == FALSE) {
            err = GetLastError();
            goto cleanup;
        }
        filename = filename + len + 1;
    }

cleanup:
    GlobalFree(driverdir);
    GlobalFree(filepath);

    return err;
}

INT_PTR CALLBACK
print_dlg_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    unsigned int idx, len;
    static HWND prnCombo = NULL;

    switch (msg) {
    case WM_INITDIALOG:
        prnCombo = GetDlgItem(hwnd, IDC_PRNCOMBO);
        SendMessage(prnCombo, CB_RESETCONTENT, 0, 0);
        SendMessage(prnCombo, CB_ADDSTRING, 0, (LPARAM) _T("None (Printing Disabled)"));

        for (idx = 0; idx < dwPrintersNum; idx++)
            SendMessage(prnCombo, CB_ADDSTRING, 0, (LPARAM)lpbPrintInfo[idx].pPrinterName);

        SendMessage(prnCombo, CB_SETCURSEL, 0, 0);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            idx = SendMessage(prnCombo, CB_GETCURSEL, 0, 0);
            len = SendMessage(prnCombo, CB_GETLBTEXTLEN, idx, 0);
            prnName = (TCHAR *)GlobalAlloc(GPTR, len);

            if (idx == 0) {
                lstrcpy(prnName, _T(""));
            } else {
                SendMessage(prnCombo, CB_GETLBTEXT, idx, (LPARAM) prnName);
            }

            prnCombo = NULL;
            EndDialog(hwnd, IDOK);
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

void __declspec(dllexport)
nsPrinterSelectDialog(HWND hwndParent,int string_size,
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
            ));
    }

    DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_PRNSEL), hwndParent, print_dlg_proc);

    pushstring(prnName);
    GlobalFree(lpbPrintInfo);
    GlobalFree(prnName);
    lpbPrintInfo = NULL;
    prnName = NULL;
    dwPrintersNum = 0;
}

void __declspec(dllexport)
nsEnumPorts(HWND hwndParent, int string_size,
        LPTSTR variables, stack_t **stacktop)
{
    int i;
    DWORD pcbNeeded, pcReturned;
    DWORD err; BOOL rv;

    PORT_INFO_1 *portinfo = NULL;
    TCHAR *buf = NULL;

    EXDLL_INIT();
    buf = GlobalAlloc(GPTR, BUF_SIZE);

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
        pusherrormessage(buf, BUF_SIZE, _T("Unable to enumerate ports"), err);
        setuservariable(INST_R0, _T("-1"));
        goto cleanup;
    }

    for (i = pcReturned - 1; i >= 0; i--)
        pushstring(portinfo[i].pName);

    _sntprintf(buf, BUF_SIZE, _T("%ld"), pcReturned);
    setuservariable(INST_R0, buf);

cleanup:
    GlobalFree(buf);
    GlobalFree(portinfo);
}

void __declspec(dllexport)
nsAddPort(HWND hwndParent, int string_size,
        LPTSTR variables, stack_t **stacktop)
{
    DWORD dwNeeded, dwStatus;
    DWORD err; BOOL rv;

    HANDLE hPrinter = NULL;
    TCHAR *buf = NULL, *monbuf = NULL;

    PRINTER_DEFAULTS pd;
    pd.pDatatype = NULL;
    pd.pDevMode = NULL;
    pd.DesiredAccess = SERVER_ACCESS_ADMINISTER;

    EXDLL_INIT();
    buf = GlobalAlloc(GPTR, BUF_SIZE);
    monbuf = GlobalAlloc(GPTR, BUF_SIZE);

    /* Pop print monitor */
    popstring(buf);
    _sntprintf(monbuf, BUF_SIZE, _T(",XcvMonitor %s"), buf);

    rv = OpenPrinter(monbuf, &hPrinter, &pd);
    if (rv == FALSE) {
        err = GetLastError();
        pusherrormessage(buf, BUF_SIZE, _T("Unable to open XcvMonitor"), err);
        setuservariable(INST_R0, _T("0"));
        goto cleanup;
    }

    /* Pop port name */
    popstring(buf);
    rv = XcvData(
            hPrinter,
            L"AddPort",
            (PBYTE) buf,
            ((lstrlen(buf)+1)*sizeof(TCHAR)),
            NULL,
            0,
            &dwNeeded,
            &dwStatus
        );

    if (rv == FALSE) {
        err = GetLastError();
        pusherrormessage(buf, BUF_SIZE, _T("Unable to add port"), err);
        setuservariable(INST_R0, _T("0"));
        goto cleanup;
    }

    setuservariable(INST_R0, _T("1"));

cleanup:
    ClosePrinter(hPrinter);
    GlobalFree(buf);
    GlobalFree(monbuf);
}

void __declspec(dllexport)
nsAddPrinterDriver(HWND hwndParent, int string_size,
        LPTSTR variables, stack_t **stacktop)
{
    DWORD arch;
    DRIVER_INFO di;
    DWORD err;
    BOOL rv;

    LPTSTR driverdir = NULL, inifile = NULL;
    TCHAR *buf1 = NULL, *buf2 = NULL;

    EXDLL_INIT();
    ZeroMemory(&di, sizeof(DRIVER_INFO));
    buf1 = GlobalAlloc(GPTR, BUF_SIZE);
    buf2 = GlobalAlloc(GPTR, BUF_SIZE);

    /* System Architecture (x86 or x64) */
    popstring(buf1);
    if (!lstrcmp(buf1, _T("x64"))) {
        arch = ARCH_X64;
    } else {
        arch = ARCH_X86;
    }

    /* Driver Directory */
    popstring(buf1);
    _sntprintf(buf2, BUF_SIZE, _T("%s\\%s"),
            buf1, (arch == ARCH_X64 ? _T("x64") : _T("w32x86")));
    driverdir = alloc_strcpy(buf2);

    _sntprintf(buf2, BUF_SIZE, _T("%s\\DRIVER.INI"), driverdir);
    inifile = alloc_strcpy(buf2);
    read_driverini(inifile, &di);
    err = copy_driverfiles(driverdir, &di);
    if (err) {
        pusherrormessage(buf1, BUF_SIZE, _T("Unable to copy driver files"), err);
        setuservariable(INST_R0, _T("0"));
        goto cleanup;
    }

    rv = AddPrinterDriver(NULL, DRIVER_INFO_LEVEL, (LPBYTE) &di);
    if (rv == FALSE) {
        err = GetLastError();
        pusherrormessage(buf1, BUF_SIZE, _T("Unable to add printer driver"), err);
        setuservariable(INST_R0, _T("0"));
        goto cleanup;
    }

    delete_driverfiles(&di);
    setuservariable(INST_R0, _T("1"));

cleanup:
    cleanup_driverinfo(&di);
    GlobalFree(buf1); GlobalFree(buf2);
    GlobalFree(driverdir);
    GlobalFree(inifile);
}

void __declspec(dllexport)
nsAddPrinter(HWND hwndParent, int string_size,
        LPTSTR variables, stack_t **stacktop)
{
    PRINTER_INFO_2 printerInfo;
    DWORD err;

    HANDLE hPrinter = NULL;
    TCHAR *buf = NULL;

    EXDLL_INIT();
    ZeroMemory(&printerInfo, sizeof(PRINTER_INFO_2));
    buf = GlobalAlloc(GPTR, BUF_SIZE);

    /* Printer Name */
    popstring(buf);
    printerInfo.pPrinterName = GlobalAlloc(GPTR, ((lstrlen(buf)+1)*sizeof(TCHAR)));
    lstrcpy(printerInfo.pPrinterName, buf);

    /* Port Name */
    popstring(buf);
    printerInfo.pPortName = GlobalAlloc(GPTR, ((lstrlen(buf)+1)*sizeof(TCHAR)));
    lstrcpy(printerInfo.pPortName, buf);

    /* Printer Driver */
    popstring(buf);
    printerInfo.pDriverName = GlobalAlloc(GPTR, ((lstrlen(buf)+1)*sizeof(TCHAR)));
    lstrcpy(printerInfo.pDriverName, buf);

    hPrinter = AddPrinter(NULL, 2, (LPBYTE) &printerInfo);
    if (hPrinter == NULL) {
        err = GetLastError();
        pusherrormessage(buf, BUF_SIZE, _T("Unable to add printer"), err);
        setuservariable(INST_R0, _T("0"));
        goto cleanup;
    }

    setuservariable(INST_R0, _T("1"));

cleanup:
    ClosePrinter(hPrinter);
    GlobalFree(buf);
    GlobalFree(printerInfo.pDriverName);
    GlobalFree(printerInfo.pPortName);
    GlobalFree(printerInfo.pPrinterName);
}

void __declspec(dllexport)
nsDeletePrinter(HWND hwndParent, int string_size,
        LPTSTR variables, stack_t **stacktop)
{
    BOOL rv;
    DWORD err;

    HANDLE hPrinter = NULL;
    TCHAR *buf = NULL;

    PRINTER_DEFAULTS pd;
    pd.pDatatype = NULL;
    pd.pDevMode = NULL;
    pd.DesiredAccess = PRINTER_ALL_ACCESS;

    EXDLL_INIT();
    buf = GlobalAlloc(GPTR, BUF_SIZE);
    popstring(buf);
    rv = OpenPrinter(buf, &hPrinter, &pd);
    if (rv == FALSE) {
        err = GetLastError();
        pusherrormessage(buf, BUF_SIZE, _T("Unable to open printer"), err);
        setuservariable(INST_R0, _T("0"));
        goto cleanup;
    }

    rv = DeletePrinter(hPrinter);
    if (rv == FALSE) {
        err = GetLastError();
        pusherrormessage(buf, BUF_SIZE, _T("Unable to delete printer"), err);
        setuservariable(INST_R0, _T("-1"));
        goto cleanup;
    }

    setuservariable(INST_R0, _T("1"));

cleanup:
    ClosePrinter(hPrinter);
    GlobalFree(buf);
}

void __declspec(dllexport)
nsRedMonConfigurePort(HWND hwndParent, int string_size,
        LPTSTR variables, stack_t **stacktop)
{
    RECONFIG config;
    PRINTER_DEFAULTS pd;
    DWORD dwNeeded, dwStatus;
    DWORD err; BOOL rv;

    HANDLE hPrinter = NULL;
    TCHAR *buf = NULL;

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

    buf = GlobalAlloc(GPTR, BUF_SIZE);
    popstring(buf);
    lstrcpy(config.szPortName, buf);
    popstring(buf);
    lstrcpy(config.szCommand, buf);
    lstrcpy(config.szDescription, _T("Redirected Port"));

    rv = OpenPrinter(_T(",XcvMonitor Redirected Port"), &hPrinter, &pd);
    if (rv == FALSE) {
        err = GetLastError();
        pusherrormessage(buf, BUF_SIZE, _T("Unable to open XcvMonitor"), err);
        setuservariable(INST_R0, _T("0"));
        goto cleanup;
    }

    rv = XcvData(
            hPrinter,
            L"SetConfig",
            (PBYTE)(&config),
            sizeof(RECONFIG),
            NULL,
            0,
            &dwNeeded,
            &dwStatus
        );
    if (rv == FALSE) {
        err = GetLastError();
        pusherrormessage(buf, BUF_SIZE, _T("Unable to configure port"), err);
        setuservariable(INST_R0, _T("0"));
        goto cleanup;
    }

    setuservariable(INST_R0, _T("1"));

cleanup:
    ClosePrinter(hPrinter);
    GlobalFree(buf);
}

void __declspec(dllexport)
nsGetDefaultPrinter(HWND hwndParent, int string_size,
        LPTSTR variables, stack_t **stacktop)
{
    DWORD dwNeeded, err;
    BOOL rv;
    TCHAR *buf = NULL;

    EXDLL_INIT();
    buf = GlobalAlloc(GPTR, BUF_SIZE);

    dwNeeded = 0;
    GetDefaultPrinter(NULL, &dwNeeded);
    if (dwNeeded > BUF_SIZE) {
        pusherrormessage(buf, BUF_SIZE, _T("Not enough buffer space for default printer name"), 0);
        setuservariable(INST_R0, _T("0"));
        goto cleanup;
    }

    rv = GetDefaultPrinter(buf, &dwNeeded);
    if (rv == FALSE) {
        err = GetLastError();
        pusherrormessage(buf, BUF_SIZE, _T("Unable to get default printer"), err);
        setuservariable(INST_R0, _T("0"));
        goto cleanup;
    }

    pushstring(buf);
    setuservariable(INST_R0, _T("1"));

cleanup:
    GlobalFree(buf);
}

void __declspec(dllexport)
nsSetDefaultPrinter(HWND hwndParent, int string_size,
        LPTSTR variables, stack_t **stacktop)
{
    DWORD err;
    BOOL rv;
    TCHAR *buf = NULL;

    EXDLL_INIT();
    buf = GlobalAlloc(GPTR, BUF_SIZE);
    popstring(buf);
    rv = SetDefaultPrinter(buf);
    if (rv == FALSE) {
        err = GetLastError();
        pusherrormessage(buf, BUF_SIZE, _T("Unable to set default printer"), err);
        setuservariable(INST_R0, _T("0"));
        goto cleanup;
    }

    setuservariable(INST_R0, _T("1"));

cleanup:
    GlobalFree(buf);
}

BOOL WINAPI
DllMain(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
    g_hInstance = hInst;
    return TRUE;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
