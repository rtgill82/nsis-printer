;
; Created:  Sat 30 Apr 2016 03:26:07 PM PDT
; Modified: Wed 28 Dec 2016 07:02:26 PM PST
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
;  Usage: ``${PrinterSelectDialog} DEFAULT_NONE RET``
;
; Displays a dialog allowing the user to select a printer from the printers
; available on the current machine. If ``DEFAULT_NONE`` is ``true`` then an
; option for '``None (Printing Disabled)``' is provided and selected by
; default.  The selected printer is returned in register ``RET``.
;
!macro _PrinterSelectDialog _DEFAULT_NONE _RET
Push "${_DEFAULT_NONE}"
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
;  Usage: ``${AddPort} NAME RET``
;
; Adds a new port on the current machine. The port will be named ``NAME``. A
; return value is returned in register ``RET``. It will be ``1`` on success or
; ``0`` on failure. If a failure occurs then an error message remains on the
; stack.
;
; NOTE: I've only been able to successfully add RedMon redirect (``RPT?``)
; ports. More general ports (``LPT?``, ``COM?``, etc.) all seem to fail. This
; function is most useful when installing and configuring RedMon.
;
!macro _AddPort _NAME _RET
Push "${_NAME}"
Printer::nsAddPort
Pop ${_RET}
!macroend
!define AddPort "!insertmacro _AddPort"

;;
; DeletePort
; ~~~~~~~~~~
;
;  Usage: ``${DeletePort} NAME RET``
;
; Deletes the port ``NAME`` on the current machine. A return value is returned
; in register ``RET``. It will be ``1`` on success or ``0`` on failure. If a
; failure occurs then an error message remains on the stack.
;
!macro _DeletePort _NAME _RET
Push "${_NAME}"
Printer::nsDeletePort
Pop ${_RET}
!macroend
!define DeletePort "!insertmacro _DeletePort"

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
; Adds a printer driver defined by ``INIFILE``. The Driver INI file format is
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
; ConfigureRedMonPort
; ~~~~~~~~~~~~~~~~~~~
;
;  Usage: ``${ConfigureRedMonPort} NAME COMMAND RET``
;
; Configures a RedMon port to redirect data to the specified command. ``NAME``
; is the name of the port to configure, usually taking the form of ``RPT?``.
; ``COMMAND`` is the command to be executed when data is received by the port.
; RedMon must have already been installed through some other means before this
; function can be called. If an error occurs ``0`` is returned and the error
; message remains on the stack.
;
!macro _ConfigureRedMonPort _NAME _COMMAND _RET
Push "${_COMMAND}"
Push "${_NAME}"
Printer::nsConfigureRedMonPort
Pop ${_RET}
!macroend
!define ConfigureRedMonPort "!insertmacro _ConfigureRedMonPort"

!endif ; PRINTER_NSH
