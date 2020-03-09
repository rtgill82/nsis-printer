/*
 * Created:  Fri 12 Dec 2014 07:37:55 PM PST
 * Modified: Mon 09 Mar 2020 03:35:47 PM PDT
 *
 * Copyright (C) 2014-2018 Robert Gill
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <windows.h>
#include <winspool.h>
#include <tchar.h>

#include "printer.h"
#include "pluginapi.h"
#include "portmon.h"
#include "redmonrc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PRINTING_DISABLED (_T ("None (Printing Disabled)"))

#define DRIVER_INFO_LEVEL 3
#define DRIVER_INFO DRIVER_INFO_3

#define PRINTER_INFO_LEVEL 5
#define LPPRINTER_INFO LPPRINTER_INFO_5
#define PRINTER_INFO PRINTER_INFO_5

#define MAX(a,b) (((a)>(b))?(a):(b))

/*
 * string_size is a parameter passed to NSIS plugin functions,
 * the BUF_SIZE macro should only be used within those.
 */
#define BUF_SIZE (string_size * sizeof(TCHAR))

/*
 * Defined and used by Redmon 1.9.
 * Copyright (C) Ghostgum Software Pty Ltd.
 */
struct reconfig_s
{
  DWORD dwSize;                 /* sizeof this structure */
  DWORD dwVersion;              /* version number of RedMon */
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

struct printer_select_dialog_opts_s
{
  DWORD dwPrintersNum;
  LPPRINTER_INFO lpbPrinterInfo;
  BOOL include_none;
  LPTSTR default_printer;
};

static HINSTANCE g_hInstance;

#define ERRBUF_SIZE 1024
static TCHAR errbuf[ERRBUF_SIZE];

static void NSISCALL
pusherrormessage (LPCTSTR msg, DWORD err)
{
  TCHAR *p;
  int len;
  lstrcpy (errbuf, msg);

  if (err > 0)
    {
      lstrcat (errbuf, _T (": "));
      len = lstrlen (errbuf);
      FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                     NULL, err, 0, errbuf + len,
                     (ERRBUF_SIZE - len) * sizeof (TCHAR), NULL);
    }

  p = errbuf + lstrlen(errbuf) - 1;
  while (p != errbuf && isspace(*p)) p--;
  if (p != errbuf) *(p + 1) = '\0';

  pushstring (errbuf);
}

static LPTSTR
alloc_strcpy (LPTSTR str)
{
  LPTSTR newstr;

  newstr = GlobalAlloc (GPTR, (_tcslen (str) + 1) * sizeof (TCHAR));
  _tcscpy (newstr, str);

  return newstr;
}

static LPTSTR
dircpy (LPTSTR dest, LPCTSTR path)
{
  int i, dirlen = 0;

  for (i = 0; path[i] != '\0'; i++)
    if (path[i] == '\\')
      dirlen = i;

  _tcsncpy (dest, path, dirlen);
  dest[dirlen] = '\0';

  return dest;
}

static INT_PTR CALLBACK
printer_select_dialog_proc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  struct printer_select_dialog_opts_s *opts;
  LPTSTR printerName, defaultPrinter;
  DWORD dwNeeded;
  unsigned int idx, len, idx_offset = 0;
  static HWND prnCombo = NULL;

  switch (msg)
    {
    case WM_INITDIALOG:
      /* Get System default printer. */
      GetDefaultPrinter (NULL, &dwNeeded);
      defaultPrinter = GlobalAlloc (GPTR, dwNeeded * sizeof (TCHAR));
      if (!GetDefaultPrinter (defaultPrinter, &dwNeeded))
        {
          GlobalFree (defaultPrinter);
          defaultPrinter = NULL;
        }

      /* Set default printer from parameter if supplied. */
      opts = (struct printer_select_dialog_opts_s *) lParam;
      if (opts->default_printer[0] != '\0')
        defaultPrinter = opts->default_printer;

      prnCombo = GetDlgItem (hwnd, IDC_PRINTER_COMBO);
      SendMessage (prnCombo, CB_RESETCONTENT, 0, 0);
      if (opts->include_none == TRUE)
        {
          SendMessage (prnCombo, CB_ADDSTRING, 0, (LPARAM) PRINTING_DISABLED);
          idx_offset += 1;
        }

      SendMessage (prnCombo, CB_SETCURSEL, 0, 0);
      for (idx = 0; idx < opts->dwPrintersNum; idx++)
        {
          SendMessage (prnCombo, CB_ADDSTRING, 0,
                       (LPARAM) opts->lpbPrinterInfo[idx].pPrinterName);
          if (defaultPrinter &&
              lstrcmp (opts->lpbPrinterInfo[idx].pPrinterName,
                       defaultPrinter) == 0)
            SendMessage (prnCombo, CB_SETCURSEL, idx + idx_offset, 0);
        }

      GlobalFree (defaultPrinter);
      return TRUE;

    case WM_COMMAND:
      switch (LOWORD (wParam))
        {
        case IDOK:
          idx = SendMessage (prnCombo, CB_GETCURSEL, 0, 0);
          len = SendMessage (prnCombo, CB_GETLBTEXTLEN, idx, 0);
          printerName =
            (LPTSTR) GlobalAlloc (GPTR, (len + 1) * sizeof (TCHAR));

          SendMessage (prnCombo, CB_GETLBTEXT, idx, (LPARAM) printerName);
          if (lstrcmp (PRINTING_DISABLED, printerName) == 0)
            lstrcpy (printerName, _T (""));

          prnCombo = NULL;
          EndDialog (hwnd, (INT_PTR) printerName);
          break;
        }
      break;

    default:
      return FALSE;
    }

  return TRUE;
}

/* Parse dependent files in driver ini file. */
static LPTSTR
parse_depfiles (LPTSTR str)
{
  LPTSTR multisz;
  LPTSTR p, pstr, pmultisz;
  size_t cnt = 0;

  multisz = GlobalAlloc (GPTR, (_tcslen (str) + 2) * sizeof (TCHAR));
  pstr = str;
  pmultisz = multisz;
  for (p = str; *p; p++)
    {
      if (*p == _T (';'))
        {
          _tcsncpy (pmultisz, pstr, cnt);
          pmultisz[cnt] = _T ('\0');
          pmultisz += cnt + 1;
          cnt = 0;
          pstr = p + 1;
        }
      else
        {
          cnt++;
        }
    }

  _tcscpy (pmultisz, _T (""));
  return multisz;
}

static void
read_driverini (LPTSTR inifile, DRIVER_INFO *di)
{
  LPTSTR buf;

  buf = GlobalAlloc (GPTR, MAX_PATH * sizeof (TCHAR));

  GetPrivateProfileString (_T ("driver"), _T ("name"), NULL, buf,
                           MAX_PATH, inifile);
  di->pName = alloc_strcpy (buf);

  GetPrivateProfileString (_T ("driver"), _T ("environment"), NULL, buf,
                           MAX_PATH, inifile);
  di->pEnvironment = alloc_strcpy (buf);

  GetPrivateProfileString (_T ("driver"), _T ("driver"), NULL, buf,
                           MAX_PATH, inifile);
  di->pDriverPath = alloc_strcpy (buf);

  GetPrivateProfileString (_T ("driver"), _T ("datafile"), NULL, buf,
                           MAX_PATH, inifile);
  di->pDataFile = alloc_strcpy (buf);

  GetPrivateProfileString (_T ("driver"), _T ("configfile"), NULL, buf,
                           MAX_PATH, inifile);
  di->pConfigFile = alloc_strcpy (buf);

  GetPrivateProfileString (_T ("driver"), _T ("helpfile"), NULL, buf,
                           MAX_PATH, inifile);
  di->pHelpFile = alloc_strcpy (buf);

  GetPrivateProfileString (_T ("driver"), _T ("depfiles"), NULL, buf,
                           MAX_PATH, inifile);
  di->pDependentFiles = parse_depfiles (buf);

  GlobalFree (buf);
}

static void
cleanup_driverinfo (DRIVER_INFO *di)
{
  GlobalFree (di->pName);
  GlobalFree (di->pEnvironment);
  GlobalFree (di->pDriverPath);
  GlobalFree (di->pDataFile);
  GlobalFree (di->pConfigFile);
  GlobalFree (di->pHelpFile);
  GlobalFree (di->pDependentFiles);
}

static size_t
max_driverfile_name (DRIVER_INFO *di)
{
  LPTSTR filename;
  size_t filemax = 0;

  filemax = MAX (filemax, _tcslen (di->pDriverPath));
  filemax = MAX (filemax, _tcslen (di->pDataFile));
  filemax = MAX (filemax, _tcslen (di->pConfigFile));
  filemax = MAX (filemax, _tcslen (di->pHelpFile));

  filename = di->pDependentFiles;
  while (TRUE)
    {
      size_t len = _tcslen (filename);
      if (len == 0)
        break;
      filemax = MAX (filemax, len);
      filename = filename + len + 1;
    }

  return filemax;
}

static DWORD
copy_driverfiles (LPTSTR srcdir, DRIVER_INFO *di)
{
  LPTSTR filename;
  size_t srcbuflen, destbuflen, filemax;
  DWORD dwNeeded;

  LPTSTR dest = NULL, src = NULL, driverdir = NULL;
  DWORD err = 0;

  GetPrinterDriverDirectory (NULL, di->pEnvironment, 1, NULL, 0, &dwNeeded);
  driverdir = GlobalAlloc (GPTR, dwNeeded * sizeof (TCHAR));
  GetPrinterDriverDirectory (NULL, di->pEnvironment, 1, (LPBYTE) driverdir,
                             dwNeeded, &dwNeeded);

  filemax = max_driverfile_name (di);
  srcbuflen = (_tcslen (srcdir) + filemax + 2) * sizeof (TCHAR);
  src = GlobalAlloc (GPTR, srcbuflen);
  destbuflen = (_tcslen (driverdir) + filemax + 2) * sizeof (TCHAR);
  dest = GlobalAlloc (GPTR, destbuflen);

  _sntprintf (src, srcbuflen, _T ("%s\\%s"), srcdir, di->pDriverPath);
  _sntprintf (dest, destbuflen, _T ("%s\\%s"), driverdir, di->pDriverPath);
  if (!CopyFile (src, dest, FALSE))
    {
      err = GetLastError ();
      goto cleanup;
    }

  _sntprintf (src, srcbuflen, _T ("%s\\%s"), srcdir, di->pDataFile);
  _sntprintf (dest, destbuflen, _T ("%s\\%s"), driverdir, di->pDataFile);
  if (!CopyFile (src, dest, FALSE))
    {
      err = GetLastError ();
      goto cleanup;
    }

  _sntprintf (src, srcbuflen, _T ("%s\\%s"), srcdir, di->pConfigFile);
  _sntprintf (dest, destbuflen, _T ("%s\\%s"), driverdir, di->pConfigFile);
  if (!CopyFile (src, dest, FALSE))
    {
      err = GetLastError ();
      goto cleanup;
    }

  _sntprintf (src, srcbuflen, _T ("%s\\%s"), srcdir, di->pHelpFile);
  _sntprintf (dest, destbuflen, _T ("%s\\%s"), driverdir, di->pHelpFile);
  if (!CopyFile (src, dest, FALSE))
    {
      err = GetLastError ();
      goto cleanup;
    }

  filename = di->pDependentFiles;
  while (TRUE)
    {
      size_t len = _tcslen (filename);
      if (len == 0)
        break;
      _sntprintf (src, srcbuflen, _T ("%s\\%s"), srcdir, filename);
      _sntprintf (dest, destbuflen, _T ("%s\\%s"), driverdir, filename);
      if (!CopyFile (src, dest, FALSE))
        {
          err = GetLastError ();
          goto cleanup;
        }
      filename = filename + len + 1;
    }

cleanup:
  GlobalFree (driverdir);
  GlobalFree (dest);
  GlobalFree (src);

  return err;
}

static DWORD
delete_driverfiles (DRIVER_INFO *di)
{
  LPTSTR filename;
  size_t filemax, buflen;
  DWORD dwNeeded;

  LPTSTR driverdir = NULL, filepath = NULL;
  DWORD err = 0;

  GetPrinterDriverDirectory (NULL, di->pEnvironment, 1, NULL, 0, &dwNeeded);
  driverdir = GlobalAlloc (GPTR, dwNeeded * sizeof (TCHAR));
  GetPrinterDriverDirectory (NULL, di->pEnvironment, 1, (LPBYTE) driverdir,
                             dwNeeded, &dwNeeded);

  filemax = max_driverfile_name (di);
  buflen = (_tcslen (driverdir) + filemax + 2) * sizeof (TCHAR);
  filepath = GlobalAlloc (GPTR, buflen);
  _sntprintf (filepath, buflen, _T ("%s\\%s"), driverdir, di->pDriverPath);
  if (!DeleteFile (filepath))
    {
      err = GetLastError ();
      goto cleanup;
    }

  _sntprintf (filepath, buflen, _T ("%s\\%s"), driverdir, di->pDataFile);
  if (!DeleteFile (filepath))
    {
      err = GetLastError ();
      goto cleanup;
    }

  _sntprintf (filepath, buflen, _T ("%s\\%s"), driverdir, di->pConfigFile);
  if (!DeleteFile (filepath))
    {
      err = GetLastError ();
      goto cleanup;
    }

  _sntprintf (filepath, buflen, _T ("%s\\%s"), driverdir, di->pHelpFile);
  if (!DeleteFile (filepath))
    {
      err = GetLastError ();
      goto cleanup;
    }

  filename = di->pDependentFiles;
  while (TRUE)
    {
      size_t len = _tcslen (filename);
      if (len == 0)
        break;
      _sntprintf (filepath, buflen, _T ("%s\\%s"), driverdir, filename);
      if (!DeleteFile (filepath))
        {
          err = GetLastError ();
          goto cleanup;
        }
      filename = filename + len + 1;
    }

cleanup:
  GlobalFree (driverdir);
  GlobalFree (filepath);

  return err;
}

void DLLEXPORT
nsPrinterSelectDialog (HWND hwndParent, int string_size, LPTSTR variables,
                       stack_t **stacktop)
{
  LPTSTR printerName, buf = NULL;
  DWORD dwNeeded;
  struct printer_select_dialog_opts_s opts;

  EXDLL_INIT ();
  ZeroMemory (&opts, sizeof (struct printer_select_dialog_opts_s));
  opts.include_none = FALSE;

  /* Check "INCLUDE_NONE" parameter. */
  buf = GlobalAlloc (GPTR, BUF_SIZE);
  popstring (buf);
  if (lstrcmpi (buf, _T ("true")) == 0)
    opts.include_none = TRUE;

  /* Get "DEFAULT" parameter. */
  popstring (buf);
  opts.default_printer = buf;

  EnumPrinters (PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL,
                PRINTER_INFO_LEVEL, NULL, 0, &dwNeeded, &opts.dwPrintersNum);

  opts.lpbPrinterInfo = (LPPRINTER_INFO)
    GlobalAlloc (GPTR, dwNeeded *sizeof (TCHAR));
  EnumPrinters (PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL,
                PRINTER_INFO_LEVEL, (LPBYTE) opts.lpbPrinterInfo, dwNeeded,
                &dwNeeded, &opts.dwPrintersNum);

  printerName = (LPTSTR) DialogBoxParam (g_hInstance,
                                         MAKEINTRESOURCE (IDD_PRINTER_SELECT),
                                         hwndParent,
                                         printer_select_dialog_proc,
                                         (LPARAM) &opts);

  pushstring (printerName);
  GlobalFree (opts.lpbPrinterInfo);
  GlobalFree (printerName);
  GlobalFree (buf);
}

void DLLEXPORT
nsEnumPrinters (HWND hwndParent, int string_size, LPTSTR variables,
                stack_t **stacktop)
{
  int i;
  DWORD dwNeeded, dwPrintersNum;
  DWORD err;

  LPPRINTER_INFO lpbPrinterInfo = NULL;

  EXDLL_INIT ();
  EnumPrinters (PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL,
                PRINTER_INFO_LEVEL, NULL, 0, &dwNeeded, &dwPrintersNum);

  lpbPrinterInfo = (LPPRINTER_INFO)
    GlobalAlloc (GPTR, dwNeeded * sizeof (TCHAR));
  if (!EnumPrinters (PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL,
                     PRINTER_INFO_LEVEL, (LPBYTE) lpbPrinterInfo, dwNeeded,
                     &dwNeeded, &dwPrintersNum))
    {
      err = GetLastError ();
      pusherrormessage (_T ("Unable to enumerate printers"), err);
      pushint (-1);
      goto cleanup;
    }

  for (i = dwPrintersNum - 1; i >= 0; i--)
    pushstring (lpbPrinterInfo[i].pPrinterName);

  pushint (dwPrintersNum);

cleanup:
  GlobalFree (lpbPrinterInfo);
}

void DLLEXPORT
nsGetPrinterPort (HWND hwndParent, int string_size, LPTSTR variables,
                  stack_t **stacktop)
{
  DWORD dwNeeded;

  HANDLE hPrinter = NULL;
  LPPRINTER_INFO printerInfo = NULL;
  LPTSTR buf = NULL;

  EXDLL_INIT ();
  buf = GlobalAlloc (GPTR, BUF_SIZE);

  /* Printer Name */
  popstring (buf);

  if (!OpenPrinter (buf, &hPrinter, NULL))
    {
      pusherrormessage (_T ("Unable to open printer"), GetLastError ());
      pushint (0);
      goto cleanup;
    }

  GetPrinter (hPrinter, PRINTER_INFO_LEVEL, NULL, 0, &dwNeeded);
  printerInfo = GlobalAlloc (GPTR, dwNeeded * sizeof (TCHAR));
  if (!GetPrinter (hPrinter, PRINTER_INFO_LEVEL, (LPBYTE) printerInfo,
                   dwNeeded, &dwNeeded))
    {
      pusherrormessage (_T ("Unable to get printer information"),
                        GetLastError ());
      pushint (0);
      goto cleanup;
    }

  pushstring (printerInfo->pPortName);

cleanup:
  ClosePrinter (hPrinter);
  GlobalFree (buf);
  GlobalFree (printerInfo);
}

void DLLEXPORT
nsAddPrinter (HWND hwndParent, int string_size, LPTSTR variables,
              stack_t **stacktop)
{
  PRINTER_INFO_2 printerInfo;
  DWORD err;

  HANDLE hPrinter = NULL;
  LPTSTR buf = NULL;

  EXDLL_INIT ();
  ZeroMemory (&printerInfo, sizeof (PRINTER_INFO_2));
  buf = GlobalAlloc (GPTR, BUF_SIZE);

  /* Printer Name */
  popstring (buf);
  printerInfo.pPrinterName =
    GlobalAlloc (GPTR, ((lstrlen (buf) + 1) * sizeof (TCHAR)));
  lstrcpy (printerInfo.pPrinterName, buf);

  /* Port Name */
  popstring (buf);
  printerInfo.pPortName =
    GlobalAlloc (GPTR, ((lstrlen (buf) + 1) * sizeof (TCHAR)));
  lstrcpy (printerInfo.pPortName, buf);

  /* Printer Driver */
  popstring (buf);
  printerInfo.pDriverName =
    GlobalAlloc (GPTR, ((lstrlen (buf) + 1) * sizeof (TCHAR)));
  lstrcpy (printerInfo.pDriverName, buf);

  hPrinter = AddPrinter (NULL, 2, (LPBYTE) &printerInfo);
  if (hPrinter == NULL)
    {
      err = GetLastError ();
      pusherrormessage (_T ("Unable to add printer"), err);
      pushint (0);
      goto cleanup;
    }

  pushint (1);

cleanup:
  ClosePrinter (hPrinter);
  GlobalFree (buf);
  GlobalFree (printerInfo.pDriverName);
  GlobalFree (printerInfo.pPortName);
  GlobalFree (printerInfo.pPrinterName);
}

void DLLEXPORT
nsDeletePrinter (HWND hwndParent, int string_size, LPTSTR variables,
                 stack_t **stacktop)
{
  DWORD err;

  HANDLE hPrinter = NULL;
  LPTSTR buf = NULL;
  PRINTER_DEFAULTS pd = { NULL, NULL, PRINTER_ALL_ACCESS };

  EXDLL_INIT ();
  buf = GlobalAlloc (GPTR, BUF_SIZE);

  /* Printer Name */
  popstring (buf);
  if (!OpenPrinter (buf, &hPrinter, &pd))
    {
      err = GetLastError ();
      pusherrormessage (_T ("Unable to open printer"), err);
      pushint (0);
      goto cleanup;
    }

  if (!DeletePrinter (hPrinter))
    {
      err = GetLastError ();
      pusherrormessage (_T ("Unable to delete printer"), err);
      pushint (0);
      goto cleanup;
    }

  pushint (1);

cleanup:
  ClosePrinter (hPrinter);
  GlobalFree (buf);
}

void DLLEXPORT
nsEnumPorts (HWND hwndParent, int string_size, LPTSTR variables,
             stack_t **stacktop)
{
  int i;
  DWORD dwNeeded, dwPortsNum;
  DWORD err;

  PORT_INFO_1 *portinfo = NULL;

  EXDLL_INIT ();
  EnumPorts (NULL, 1, NULL, 0, &dwNeeded, &dwPortsNum);
  portinfo = GlobalAlloc (GPTR, dwNeeded * sizeof (TCHAR));

  if (!EnumPorts (NULL, 1, (PBYTE) portinfo, dwNeeded, &dwNeeded, &dwPortsNum))
    {
      err = GetLastError ();
      pusherrormessage (_T ("Unable to enumerate ports"), err);
      pushint (-1);
      goto cleanup;
    }

  for (i = dwPortsNum - 1; i >= 0; i--)
    pushstring (portinfo[i].pName);

  pushint (dwPortsNum);

cleanup:
  GlobalFree (portinfo);
}

void DLLEXPORT
nsAddPort (HWND hwndParent, int string_size, LPTSTR variables,
           stack_t **stacktop)
{
  DWORD dwNeeded, dwStatus, err;

  HANDLE iface = NULL;
  LPTSTR port_name = NULL;
  LPTSTR xcv_name = NULL;
  PRINTER_DEFAULTS pd = { NULL, NULL, SERVER_ACCESS_ADMINISTER };

  EXDLL_INIT ();
  port_name = GlobalAlloc (GPTR, BUF_SIZE);
  xcv_name = GlobalAlloc (GPTR, BUF_SIZE);

  popstring (port_name);        /* Pop port name */
  popstring (xcv_name);
  if (!OpenPrinter (xcv_name, &iface, &pd))
    {
      err = GetLastError ();
      pusherrormessage (_T ("Unable to open Xcv interface"), err);
      pushint (0);
      goto cleanup;
    }

  if (!XcvData (iface, L"AddPort", (PBYTE) port_name,
                ((lstrlen (port_name) + 1) * sizeof (TCHAR)), NULL, 0,
                &dwNeeded, &dwStatus))
    {
      err = GetLastError ();
      pusherrormessage (_T ("Unable to add port"), err);
      pushint (0);
      goto cleanup;
    }

  pushint (1);

cleanup:
  ClosePrinter (iface);
  GlobalFree (port_name);
  GlobalFree (xcv_name);
}

void DLLEXPORT
nsDeletePort (HWND hwndParent, int string_size, LPTSTR variables,
              stack_t **stacktop)
{
  DWORD dwNeeded, dwStatus, err;

  HANDLE iface = NULL;
  LPTSTR port_name = NULL;
  LPTSTR xcv_name = NULL;
  PRINTER_DEFAULTS pd = { NULL, NULL, SERVER_ACCESS_ADMINISTER };

  EXDLL_INIT ();
  port_name = GlobalAlloc (GPTR, BUF_SIZE);
  xcv_name = GlobalAlloc (GPTR, BUF_SIZE);

  popstring (port_name);        /* Pop port name */
  popstring (xcv_name);
  if (!OpenPrinter (xcv_name, &iface, &pd))
    {
      err = GetLastError ();
      pusherrormessage (_T ("Unable to open Xcv interface"), err);
      pushint (0);
      goto cleanup;
    }

  if (!XcvData (iface, L"DeletePort", (PBYTE) port_name,
                ((lstrlen (port_name) + 1) * sizeof (TCHAR)), NULL, 0,
                &dwNeeded, &dwStatus))
    {
      err = GetLastError ();
      pusherrormessage (_T ("Unable to delete port"), err);
      pushint (0);
      goto cleanup;
    }

  pushint (1);

cleanup:
  ClosePrinter (iface);
  GlobalFree (port_name);
  GlobalFree (xcv_name);
}

void DLLEXPORT
nsGetDefaultPrinter (HWND hwndParent, int string_size, LPTSTR variables,
                     stack_t **stacktop)
{
  DWORD dwNeeded, err;
  LPTSTR buf = NULL;

  EXDLL_INIT ();
  buf = GlobalAlloc (GPTR, BUF_SIZE);

  GetDefaultPrinter (NULL, &dwNeeded);
  if (dwNeeded > BUF_SIZE)
    {
      pusherrormessage (
        _T ("Not enough buffer space for default printer name"), 0);
      pushint (0);
      goto cleanup;
    }

  if (!GetDefaultPrinter (buf, &dwNeeded))
    {
      err = GetLastError ();
      pusherrormessage (_T ("Unable to get default printer"), err);
      pushint (0);
      goto cleanup;
    }

  pushstring (buf);

cleanup:
  GlobalFree (buf);
}

void DLLEXPORT
nsSetDefaultPrinter (HWND hwndParent, int string_size, LPTSTR variables,
                     stack_t **stacktop)
{
  DWORD err;
  LPTSTR buf = NULL;

  EXDLL_INIT ();
  buf = GlobalAlloc (GPTR, BUF_SIZE);

  /* Printer Name */
  popstring (buf);
  if (!SetDefaultPrinter (buf))
    {
      err = GetLastError ();
      pusherrormessage (_T ("Unable to set default printer"), err);
      pushint (0);
      goto cleanup;
    }

  pushint (1);

cleanup:
  GlobalFree (buf);
}

void DLLEXPORT
nsAddPrinterDriver (HWND hwndParent, int string_size, LPTSTR variables,
                    stack_t **stacktop)
{
  DWORD err;
  DRIVER_INFO di;
  LPTSTR inifile = NULL, driverdir = NULL;

  EXDLL_INIT ();
  ZeroMemory (&di, sizeof (DRIVER_INFO));
  inifile = GlobalAlloc (GPTR, BUF_SIZE);
  driverdir = GlobalAlloc (GPTR, BUF_SIZE);

  /* Driver INI File */
  popstring (inifile);
  dircpy (driverdir, inifile);
  read_driverini (inifile, &di);
  err = copy_driverfiles (driverdir, &di);
  if (err)
    {
      pusherrormessage (_T ("Unable to copy driver files"), err);
      pushint (0);
      goto cleanup;
    }

  if (!AddPrinterDriver (NULL, DRIVER_INFO_LEVEL, (LPBYTE) &di))
    {
      err = GetLastError ();
      pusherrormessage (_T ("Unable to add printer driver"), err);
      pushint (0);
      goto cleanup;
    }

  delete_driverfiles (&di);
  pushint (1);

cleanup:
  cleanup_driverinfo (&di);
  GlobalFree (inifile);
  GlobalFree (driverdir);
}

void DLLEXPORT
nsDeletePrinterDriver (HWND hwndParent, int string_size, LPTSTR variables,
                       stack_t **stacktop)
{
  LPTSTR name;
  DWORD err;

  EXDLL_INIT ();
  name = GlobalAlloc (GPTR, BUF_SIZE);
  popstring (name);
  if (!DeletePrinterDriverEx (NULL, NULL, name, DPD_DELETE_ALL_FILES, 0))
    {
      err = GetLastError ();
      pusherrormessage (_T ("Unable to delete printer driver"), err);
      pushint(0);
      goto cleanup;
    }

  pushint (1);

cleanup:
  GlobalFree (name);
}

void DLLEXPORT
nsConfigureRedirectedPort (HWND hwndParent, int string_size, LPTSTR variables,
                           stack_t **stacktop)
{
  DWORD dwNeeded, dwStatus, err;
  RECONFIG config;
  HANDLE iface = NULL;
  LPTSTR buf = NULL;
  PRINTER_DEFAULTS pd = { NULL, NULL, SERVER_ACCESS_ADMINISTER };

  EXDLL_INIT ();
  ZeroMemory (&config, sizeof (RECONFIG));
  config.dwSize = sizeof (RECONFIG);
  config.dwVersion = VERSION_NUMBER;
  config.dwOutput = OUTPUT_SELF;
  config.dwShow = FALSE;
  config.dwRunUser = TRUE;
  config.dwDelay = DEFAULT_DELAY;
  config.dwLogFileDebug = FALSE;

  buf = GlobalAlloc (GPTR, BUF_SIZE);

  /* Port Name */
  popstring (buf);
  lstrcpy (config.szPortName, buf);

  if (!OpenPrinter (_T (",XcvMonitor Redirected Port"), &iface, &pd))
    {
      err = GetLastError ();
      pusherrormessage (_T ("Unable to open Xcv interface"), err);
      pushint (0);
      goto cleanup;
    }

  /* Command */
  popstring (buf);
  lstrcpy (config.szCommand, buf);
  lstrcpy (config.szDescription, _T ("Redirected Port"));

  if (!XcvData (iface, L"SetConfig", (PBYTE) (&config), sizeof (RECONFIG),
                NULL, 0, &dwNeeded, &dwStatus))
    {
      err = GetLastError ();
      pusherrormessage (_T ("Unable to configure port"), err);
      pushint (0);
      goto cleanup;
    }

  pushint (1);

cleanup:
  ClosePrinter (iface);
  GlobalFree (buf);
}

#ifdef _MSC_VER
void DLLEXPORT
nsInstallPrinterDriverPackage (HWND hwndParent, int string_size,
                               LPTSTR variables, stack_t** stacktop)
{
    LPTSTR name;
    HRESULT hresult;

    EXDLL_INIT ();
    name = GlobalAlloc (GPTR, BUF_SIZE);
    popstring (name);

    hresult = InstallPrinterDriverFromPackage (NULL, NULL, name, NULL,
                                               IPDFP_COPY_ALL_FILES);

    if (hresult != S_OK)
    {
        pusherrormessage (_T ("Unable to install printer driver package"),
                          hresult);
        pushint (0);
        goto cleanup;
    }

    pushint (1);

cleanup:
    GlobalFree (name);
}
#endif

BOOL WINAPI
DllMain (HINSTANCE hinstDLL, DWORD fdwReadon, LPVOID lpvReserved)
{
  g_hInstance = hinstDLL;
  return TRUE;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
