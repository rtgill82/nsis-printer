;
; Created:  Sat 30 Apr 2016 03:26:07 PM PDT
; Modified: Mon 09 Mar 2020 03:46:10 PM PDT
;
; Copyright 2016 (C) Robert Gill
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.
;

!ifndef PRINTER_NSH
!define PRINTER_NSH

;;
; PrinterSelectDialog
; ~~~~~~~~~~~~~~~~~~~
;
;  Usage: ``${PrinterSelectDialog} INCLUDE_NONE DEFAULT RET``
;
; Displays a dialog allowing the user to select a printer from the printers
; available on the current machine. If ``INCLUDE_NONE`` is ``true`` then an
; option for '``None (Printing Disabled)``' is provided. The ``DEFAULT``
; parameter provides the name of a printer to be selected by default. When
; ``DEFAULT`` is an empty string (``""``) then the system default as returned
; by ``GetDefaultPrinter`` is selected. The selected printer is returned in
; register ``RET``.
;
!macro _PrinterSelectDialog _INCLUDE_NONE _DEFAULT _RET
Push "${_DEFAULT}"
Push "${_INCLUDE_NONE}"
Printer::nsPrinterSelectDialog
Pop ${_RET}
!macroend
!define PrinterSelectDialog "!insertmacro _PrinterSelectDialog"

;;
; EnumPrinters
; ~~~~~~~~~~~~
;
;  Usage: ``${EnumPrinters} RET``
;
; Enumerates the printers available on the current machine. The number of
; printers available are returned in register ``RET``. The names of the
; available printers remain on the stack to be popped off by the caller. If
; ``-1`` is returned then an error has occurred and the error message remains
; on the stack.
;
!macro _EnumPrinters _RET
Printer::nsEnumPrinters
Pop ${_RET}
!macroend
!define EnumPrinters "!insertmacro _EnumPrinters"

;;
; GetPrinterPort
; ~~~~~~~~~~~~~~
;
;  Usage: ``${GetPrinterPort} NAME RET``
;
; Returns the port used by the printer ``NAME``. If ``0`` is returned then an
; error has occurred and the error message remains on the stack.
;
!macro _GetPrinterPort _NAME _RET
Push "${_NAME}"
Printer::nsGetPrinterPort
Pop ${_RET}
!macroend
!define GetPrinterPort "!insertmacro _GetPrinterPort"

;;
; AddPrinter
; ~~~~~~~~~~
;
;  Usage: ``${AddPrinter} NAME PORT DRIVER RET``
;
; Installs a printer driver for the printer ``NAME`` using the port ``PORT``.
; The driver must have been previously installed and ``DRIVER`` provides the
; installed driver's name. A return value is returned in register ``RET``. It
; will be ``1`` on success or ``0`` on failure. If a failure occurs then an
; error message remains on the stack.
;
!macro _AddPrinter _NAME _PORT _DRIVER _RET
Push "${_DRIVER}"
Push "${_PORT}"
Push "${_NAME}"
Printer::nsAddPrinter
Pop ${_RET}
!macroend
!define AddPrinter "!insertmacro _AddPrinter"

;;
; DeletePrinter
; ~~~~~~~~~~~~~
;
;  Usage: ``${DeletePrinter} NAME RET``
;
; Deletes a printer that's available on this machine. ``NAME`` is the name of
; the printer to be deleted. A return value is returned in register ``RET``. It
; will be ``1`` on success or ``0`` on failure. If a failure occurs then an
; error message remains on the stack.
;
!macro _DeletePrinter _NAME _RET
Push "${_NAME}"
Printer::nsDeletePrinter
Pop ${_RET}
!macroend
!define DeletePrinter "!insertmacro _DeletePrinter"

;;
; EnumPorts
; ~~~~~~~~~
;
;  Usage: ``${EnumPorts} RET``
;
; Enumerates the ports available on the current machine. The number of ports
; available are returned in register ``RET``. The names of the available ports
; remain on the stack to be popped off by the caller. If ``-1`` is returned
; then an error has occurred and the error message remains on the stack.
;
!macro _EnumPorts _RET
Printer::nsEnumPorts
Pop ${_RET}
!macroend
!define EnumPorts "!insertmacro _EnumPorts"

;;
; AddPort
; ~~~~~~~
;
;  Usage: ``${AddPort} PORTNAME XCVNAME RET``
;
; Adds a new port using the XcvMonitor interface ``XCVNAME``. The port will be
; named ``PORTNAME``. A return value is returned in register ``RET``. It will
; be ``1`` on success or ``0`` on failure. If a failure occurs then an error
; message remains on the stack.
;
!macro _AddPort _PORTNAME _XCVNAME _RET
Push "${_XCVNAME}"
Push "${_PORTNAME}"
Printer::nsAddPort
Pop ${_RET}
!macroend
!define AddPort "!insertmacro _AddPort"

;;
; DeletePort
; ~~~~~~~~~~
;
;  Usage: ``${DeletePort} PORTNAME XCVNAME RET``
;
; Deletes the port ``PORTNAME`` using the XcvMonitor interface ``XCVNAME``.
; A return value is returned in register ``RET``. It will be ``1`` on success
; or ``0`` on failure. If a failure occurs then an error message remains on the
; stack.
;
!macro _DeletePort _PORTNAME _XCVNAME _RET
Push "${_XCVNAME}"
Push "${_PORTNAME}"
Printer::nsDeletePort
Pop ${_RET}
!macroend
!define DeletePort "!insertmacro _DeletePort"

;;
; AddLocalPort
; ~~~~~~~~~~~~
;
;  Usage: ``${AddLocalPort} PORTNAME RET``
;
; Adds a new port to the local machine. The port will be named ``PORTNAME``.
; A return value is returned in register ``RET``. It will be ``1`` on success
; or ``0`` on failure. If a failure occurs then an error message remains on the
; stack.
;
!macro _AddLocalPort _PORTNAME _RET
Push ",XcvMonitor Local Port"
Push "${_PORTNAME}"
Printer::nsAddPort
Pop ${_RET}
!macroend
!define AddLocalPort "!insertmacro _AddLocalPort"

;;
; DeleteLocalPort
; ~~~~~~~~~~~~~~~
;
;  Usage: ``${DeleteLocalPort} PORTNAME RET``
;
; Deletes the local port ``PORTNAME``. A return value is returned in register
; ``RET``. It will be ``1`` on success or ``0`` on failure. If a failure occurs
; then an error message remains on the stack.
;
!macro _DeleteLocalPort _PORTNAME _RET
Push ",XcvMonitor Local Port"
Push "${_PORTNAME}"
Printer::nsDeletePort
Pop ${_RET}
!macroend
!define DeleteLocalPort "!insertmacro _DeleteLocalPort"

;;
; AddRedirectedPort
; ~~~~~~~~~~~~~~~~~
;
;  Usage: ``${AddRedirectedPort} PORTNAME RET``
;
; Adds a new redirected port to the local machine. The port will be named
; ``PORTNAME``. RedMon 1.9 is required. A return value is returned in register
; ``RET``. It will be ``1`` on success or ``0`` on failure. If a failure occurs
; then an error message remains on the stack.
;
!macro _AddRedirectedPort _PORTNAME _RET
Push ",XcvMonitor Redirected Port"
Push "${_PORTNAME}"
Printer::nsAddPort
Pop ${_RET}
!macroend
!define AddRedirectedPort "!insertmacro _AddRedirectedPort"

;;
; DeleteRedirectedPort
; ~~~~~~~~~~~~
;
;  Usage: ``${DeleteRedirectedPort} PORTNAME RET``
;
; Deletes the redirected port ``PORTNAME``. Redmon 1.9 is required. A return
; value is returned in register ``RET``. It will be ``1`` on success or ``0``
; on failure. If a failure occurs then an error message remains on the stack.
;
!macro _DeleteRedirectedPort _PORTNAME _RET
Push ",XcvMonitor Redirected Port"
Push "${_PORTNAME}"
Printer::nsDeletePort
Pop ${_RET}
!macroend
!define DeleteRedirectedPort "!insertmacro _DeleteRedirectedPort"

;;
; GetDefaultPrinter
; ~~~~~~~~~~~~~~~~~
;
;  Usage: ``${GetDefaultPrinter} RET``
;
; Gets the currently set default printer on the current machine. The name of
; the printer is returned in register ``RET``. If an error occurs ``0`` is
; returned and the error message remains on the stack.
;
!macro _GetDefaultPrinter _RET
Printer::nsGetDefaultPrinter
Pop ${_RET}
!macroend
!define GetDefaultPrinter "!insertmacro _GetDefaultPrinter"

;;
; SetDefaultPrinter
; ~~~~~~~~~~~~~~~~~
;
;  Usage: ``${SetDefaultPrinter} NAME RET``
;
; Sets the default printer on the current machine to ``NAME``. If an error
; occurs ``0`` is returned and the error message remains on the stack.
;
; NOTE: **Windows 10** will use the last printer printed to as the default
; printer. This can be overridden by disabling ``LegacyDefaultPrinterMode`` in
; the registry before calling ``SetDefaultPrinter``.
;
!macro _SetDefaultPrinter _NAME _RET
Push "${_NAME}"
Printer::nsSetDefaultPrinter
Pop ${_RET}
!macroend
!define SetDefaultPrinter "!insertmacro _SetDefaultPrinter"

;;
; AddPrinterDriver
; ~~~~~~~~~~~~~~~~
;
;  Usage: ``${AddPrinterDriver} INIFILE RET``
;
; Adds a printer driver defined by ``INIFILE``. The driver INI file format is
; documented under `Driver INI File Documentation`_. If an error occurs ``0``
; is returned and the error message remains on the stack.
;
!macro _AddPrinterDriver _INIFILE _RET
Push "${_INIFILE}"
Printer::nsAddPrinterDriver
Pop ${_RET}
!macroend
!define AddPrinterDriver "!insertmacro _AddPrinterDriver"

;;
; DeletePrinterDriver
; ~~~~~~~~~~~~~~~~~~~
;
;  Usage: ``${DeletePrinterDriver} NAME RET``
;
; Deletes the printer driver named ``NAME``. If an error occurs ``0`` is
; returned and the error message remains on the stack.
;
!macro _DeletePrinterDriver _NAME _RET
Push "${_NAME}"
Printer::nsDeletePrinterDriver
Pop ${_RET}
!macroend
!define DeletePrinterDriver "!insertmacro _DeletePrinterDriver"

;;
; ConfigureRedirectedPort
; ~~~~~~~~~~~~~~~~~~~~~~~
;
;  Usage: ``${ConfigureRedirectedPort} NAME COMMAND RET``
;
; Configures a redirected port to redirect data to the specified command.
; ``NAME`` is the name of the port to configure, usually taking the form of
; ``RPT?``.  ``COMMAND`` is the command to be executed when data is received by
; the port. RedMon must have already been installed through some other means
; before this function can be called. If an error occurs ``0`` is returned and
; the error message remains on the stack.
;
!macro _ConfigureRedirectedPort _NAME _COMMAND _RET
Push "${_COMMAND}"
Push "${_NAME}"
Printer::nsConfigureRedirectedPort
Pop ${_RET}
!macroend
!define ConfigureRedirectedPort "!insertmacro _ConfigureRedirectedPort"

;;
; InstallPrinterDriverPackage
; ~~~~~~~~~~~~~~~~~~~~~~~~~~~
;
;  Usage: ``${InstallPrinterDriverPackage} NAME RET``
;
; Install one of the printer driver packages that are provided with Microsoft
; Windows. For example, the *Generic / Text Only* driver package.
;
; **NOTICE**: This function is only available when compiled with
;             MSVC / Visual Studio.
;
!macro _InstallPrinterDriverPackage _NAME _RET
Push "${_NAME}"
Printer::nsInstallPrinterDriverPackage
Pop ${_RET}
!macroend
!define InstallPrinterDriverPackage "!insertmacro _InstallPrinterDriverPackage"

!endif ; PRINTER_NSH
