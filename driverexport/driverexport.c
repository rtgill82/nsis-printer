// printexport.cpp : Defines the entry point for the console application.
//

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>
#include <windows.h>

#ifdef UNICODE
#define fopen_s(pFile, filename, mode) _wfopen_s(pFile, filename, mode)
#else
#define fopen_s(pFile, filename, mode) fopen_s(pFile, filename, mode)
#endif

#define DRIVER_INFO_LEVEL 3
#define DRIVER_INFO DRIVER_INFO_3

#define TMPBUF_SIZE 5192
TCHAR tmpbuf[TMPBUF_SIZE];

SYSTEM_INFO si;

LPTSTR basename(LPTSTR filepath)
{
	LPTSTR p, filename = NULL;

	for (p = filepath; *p; p++)
		if (*p == L'\\') filename = p + 1;

	return filename;
}

void PrintError(DWORD err)
{
	FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			err,
			0,
			tmpbuf,
			TMPBUF_SIZE,
			NULL
		);

	printf("Error: %S", tmpbuf);
}

void PrintDriverInfo(DRIVER_INFO *driver_info)
{
	printf("Driver Name: %S\n", driver_info->pName);
	printf("Driver Path: %S\n", driver_info->pDriverPath);
	printf("Data File: %S\n", driver_info->pDataFile);
	printf("Config File: %S\n", driver_info->pConfigFile);
	printf("Help File: %S\n", driver_info->pHelpFile);
	puts("Dependent Files:");

	LPTSTR filename = driver_info->pDependentFiles;
	while (TRUE) {
		int len = lstrlen(filename);
		if (len == 0) break;
		printf("\t%S\n", filename);
		filename = filename + len + 1;
	}

	puts("");
}

LPTSTR ArchName(void)
{
	LPTSTR archName;

	switch (si.wProcessorArchitecture) {
	case PROCESSOR_ARCHITECTURE_AMD64:
		archName = TEXT("x64");
		break;
	case PROCESSOR_ARCHITECTURE_INTEL:
		archName = TEXT("w32x86");
		break;
	default:
		archName = NULL;
	}

	return archName;
}

DWORD GetDriverBufSize(HANDLE hPrinter)
{
	DWORD pcbNeeded;

	GetPrinterDriver(
			hPrinter,
			NULL,
			DRIVER_INFO_LEVEL,
			NULL,
			0,
			&pcbNeeded
		);

	return pcbNeeded;
}

BOOL CreateOutputDir(DRIVER_INFO *driver_info)
{
	BOOL rv;

	printf("Outputting driver files to \"%S\\%S\".\n", driver_info->pName, ArchName());
	rv = CreateDirectory(driver_info->pName, NULL);
	if (rv == FALSE) return rv;

	StringCbPrintf(tmpbuf, TMPBUF_SIZE, TEXT("%s\\%s"), driver_info->pName, ArchName());
	rv = CreateDirectory(tmpbuf, NULL);
	if (rv == FALSE) return rv;

	return TRUE;
}

void CopyDriverFile(LPTSTR dir, LPTSTR driverfile)
{
	StringCbPrintf(tmpbuf, TMPBUF_SIZE, TEXT("%s\\%s\\%s"), dir, ArchName(), basename(driverfile));
	CopyFile(driverfile, tmpbuf, TRUE);
}

void WriteDriverIni(DRIVER_INFO *driver_info)
{
	FILE *inifile;

	StringCbPrintf(tmpbuf, TMPBUF_SIZE, TEXT("%s\\%s\\DRIVER.INI"), driver_info->pName, ArchName());
	fopen_s(&inifile, tmpbuf, TEXT("w"));
	fputs("[driver]\n", inifile);
	fprintf(inifile, "version=%ld\n", driver_info->cVersion);
	fprintf(inifile, "name=%S\n", driver_info->pName);
	fprintf(inifile, "environment=%S\n", driver_info->pEnvironment);
	if (driver_info->pDefaultDataType != NULL)
		fprintf(inifile, "datatype=%S\n", driver_info->pDefaultDataType);

	fprintf(inifile, "driver=%S\n", basename(driver_info->pDriverPath));
	fprintf(inifile, "datafile=%S\n", basename(driver_info->pDataFile));
	fprintf(inifile, "configfile=%S\n", basename(driver_info->pConfigFile));
	fprintf(inifile, "helpfile=%S\n", basename(driver_info->pHelpFile));
	fputs("depfiles=", inifile);

	LPTSTR filename = driver_info->pDependentFiles;
	while (TRUE) {
		int len = lstrlen(filename);
		if (len == 0) break;
		fprintf(inifile, "%S;", basename(filename));
		filename = filename + len + 1;
	}

	fputs("\n", inifile);
	fclose(inifile);
}

void CopyDriverFiles(DRIVER_INFO *driver_info)
{
	CopyDriverFile(driver_info->pName, driver_info->pDriverPath);
	CopyDriverFile(driver_info->pName, driver_info->pDataFile);
	CopyDriverFile(driver_info->pName, driver_info->pConfigFile);
	CopyDriverFile(driver_info->pName, driver_info->pHelpFile);

	LPTSTR filename = driver_info->pDependentFiles;
	while (TRUE) {
		int len = lstrlen(filename);
		if (len == 0) break;
		CopyDriverFile(driver_info->pName, filename);
		filename = filename + len + 1;
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	_TCHAR *printer = NULL;
	HANDLE hPrinter = NULL;
	DWORD pcbNeeded = 0;
	BYTE *driverBuf = NULL;
	BOOL rv = 0;
	DWORD err = 0;

	DRIVER_INFO *driver_info = NULL;
	PRINTER_DEFAULTS pd;
	pd.pDatatype = NULL;
	pd.pDevMode = NULL;
	pd.DesiredAccess = PRINTER_ALL_ACCESS;

	if (argc != 2) {
		puts("Usage: printexport <PRINTER NAME>");
		return EXIT_FAILURE;
	}

	printer = argv[1];
	GetNativeSystemInfo(&si);
	rv = OpenPrinter(printer, &hPrinter, &pd);
	if (rv == 0) {
		err = GetLastError();
		printf("Unable to open printer \"%S\".\n", printer);
		PrintError(err);
		return EXIT_FAILURE;
	}

	printf("Opened printer %S.\n", printer);
	pcbNeeded = GetDriverBufSize(hPrinter);
	driverBuf = (BYTE *) GlobalAlloc(GPTR, pcbNeeded*sizeof(BYTE));
	rv = GetPrinterDriver(hPrinter, NULL, DRIVER_INFO_LEVEL, driverBuf, pcbNeeded, &pcbNeeded);
	if (rv == 0) {
		err = GetLastError();
		puts("Error getting driver information.");
		PrintError(err);
		ClosePrinter(hPrinter);
		return EXIT_FAILURE;
	}

	driver_info = (DRIVER_INFO *) driverBuf;
	PrintDriverInfo(driver_info);
	rv = CreateOutputDir(driver_info);
	if (rv == 0) {
		err = GetLastError();
		printf("Error creating output directory: \"%S\".\n", driver_info->pName);
		PrintError(err);
		ClosePrinter(hPrinter);
		return EXIT_FAILURE;
	}
	
	WriteDriverIni(driver_info);
	CopyDriverFiles(driver_info);
	ClosePrinter(hPrinter);
	return EXIT_SUCCESS;
}
