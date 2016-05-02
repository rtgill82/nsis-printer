;
; Created:  Sat 30 Apr 2016 03:26:07 PM PDT
; Modified: Sun 01 May 2016 05:04:26 PM PDT
;
; Copyright (C) 2016  Robert Gill <locke@sdf.lonestar.org>
;

!ifndef PRINTER_NSH
!define PRINTER_NSH

!macro _PrinterSelectDialog _RET
Printer::nsPrinterSelectDialog
Pop ${_RET}
!macroend
!define PrinterSelectDialog "!insertmacro _PrinterSelectDialog"

!macro _EnumPrinters _RET
Printer::nsEnumPrinters
Pop ${_RET}
!macroend
!define EnumPrinters "!insertmacro _EnumPrinters"

!macro _EnumPorts _RET
Printer::nsEnumPorts
Pop ${_RET}
!macroend
!define EnumPorts "!insertmacro _EnumPorts"

!macro _AddPort _PORT _MONITOR _RET
Push ${_PORT}
Push ${_MONITOR}
Printer::nsAddPort
Pop ${_RET}
!macroend
!define AddPort "!insertmacro _AddPort"

!macro _AddPrinterDriver _ARCH _DIR _RET
Push ${_DIR}
Push ${_ARCH}
Printer::nsAddPrinterDriver
Pop ${_RET}
!macroend
!define AddPrinterDriver "!insertmacro _AddPrinterDriver"

!macro _AddPrinter _NAME _PORT _DRIVER _RET
Push ${_DRIVER}
Push ${_PORT}
Push ${_NAME}
Printer::nsAddPrinter
Pop ${_RET}
!macroend
!define AddPrinter "!insertmacro _AddPrinter"

!macro _DeletePrinter _NAME _RET
Push ${_NAME}
Printer::nsDeletePrinter
Pop ${_RET}
!macroend
!define DeletePrinter "!insertmacro _DeletePrinter"

!macro _GetDefaultPrinter _RET
Printer::nsGetDefaultPrinter
Pop ${_RET}
!macroend
!define GetDefaultPrinter "!insertmacro _GetDefaultPrinter"

!macro _SetDefaultPrinter _NAME _RET
Push ${_NAME}
Printer::nsSetDefaultPrinter
Pop ${_RET}
!macroend
!define SetDefaultPrinter "!insertmacro _SetDefaultPrinter"

!macro _ConfigureRedMonPort _NAME _COMMAND _RET
Push ${_COMMAND}
Push ${_NAME}
Printer::nsConfigureRedMonPort
Pop ${_RET}
!macroend
!define ConfigureRedMonPort "!insertmacro _ConfigureRedMonPort"

!endif ;PRINTER_NSH
