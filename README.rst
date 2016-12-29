NSIS Printer
============

The NSIS Printer plugin allows you to perform printer management from an NSIS
installer. It supports functionality such as enumerating printers, adding a new
printer, getting or setting the default printer, selecting a printer from the
list of available printers, and adding a redirected printer port (RedMon 1.9
required).

Requirements
------------

The UNICODE plugin requires at least NSIS 3.0. The ASCII plugin should work
with NSIS 2.xx. I haven't tested (much) at all with NSIS 2.xx, so if there are
issues using the plugin with that version, let me know.

Build Requirements
------------------

VS2015 project files have been included for building with VisualStudio.
The following are the suggested requirements for building with GCC:

* `Cygwin <https://www.cygwin.com/>`_
* ``mingw64-i686-gcc-core`` (Cygwin package)
* ``mingw64-i686-runtime`` (Cygwin package; pulled in automatically by
  ``mingw64-i686-gcc-core``)
* ``mingw64-i686-headers`` (Cygwin package; pulled in automatically by
  ``mingw64-i686-gcc-core``)
* ``make`` (Cygwin package)
* ``unzip`` (Cygwin package; optional; for unpacking distribution)
* ``zip`` (Cygwin package; optional; for creating distribution packages (``make dist``))

Currently a patched version of Mingw-w64 5.0.0 is required for building. The
required patch has been included in the source distribution.

Cygwin Build Instructions
-------------------------

Install `Cygwin <https://www.cygwin.com/>`_ including the
``mingw64-i686-gcc-core`` compiler package, which should pull in
``mingw64-i686-runtime``.

Some required symbols may be missing from the ``mingw64-i686-runtime`` package
distributed with Cygwin so installing a patched version of the runtime may be
required in order to build the NSIS Printer DLL. As of this writing the version
of ``mingw64-i686-runtime`` included with Cygwin was 5.0 and installing a
patched version was necessary.

If no patching is required, building should be as simple as running:

  ``make``

If patching is required, download the `Mingw-w64 sources`_ and unpack the
downloaded files somewhere within your Cygwin ``/home/$USER`` directory.  Copy
the supplied patch, ``mingw64-runtime-print-symbols.patch``, to the newly
unpacked ``mingw-w64-v5.0.0`` directory and apply it using:

 ``patch -p1 < mingw64-runtime-print-symbols.patch``

Then build the mingw64 runtime using the following commands from within the
same directory:

 ``./configure --host=i686-w64-mingw32 --prefix=/opt/mingw-w64-v5.0.0``
 ``make``
 ``make install``

After the patched mingw-w64 runtime has been installed, return to the
nsisprinter directory and execute the command:

 ``make LDFLAGS=-L/opt/mingw-w64-v5.0.0/lib``

Example Usage
-------------

The following examples provide a basic idea of how to use this plugin. All
examples assume the inclusion of the ``logiclib.nsh`` header.

EnumPrinters Example
~~~~~~~~~~~~~~~~~~~~

.. code::

  ${EnumPrinters} $R0
  ${If} $R0 == -1
    Pop $R0
    DetailPrint "Error: $R0"
    Abort
  ${EndIf}

  DetailPrint "Available Printers:"
  ${DoWhile} $R0 > 0
    Pop $0
    DetailPrint "  $0"
    IntOp $R0 $R0 - 1
  ${Loop}

AddPrinterDriver Example
~~~~~~~~~~~~~~~~~~~~~~~~

.. code::

  ; In Main Section
  SetOutPath "$PLUGINSDIR"
  File /r "Driver Name"

  ; In -Post Section
  ${AddPrinterDriver} "$PLUGINSDIR\Driver Name\DRIVER.INI" $R0
  ${If} $R0 == 0
    Pop $1
    DetailPrint "Error: $R0"
  ${EndIf}

AddPrinter Example
~~~~~~~~~~~~~~~~~~

.. code::

  ${AddPrinter} "Printer Name" "RPT1" "Driver Name" $R0
  ${If} $R0 == 0
    Pop $R0
    DetailPrint "Error: $R0"
    Abort
  ${EndIf}

API Documentation
-----------------

PrinterSelectDialog
~~~~~~~~~~~~~~~~~~~

 Usage: ``${PrinterSelectDialog} DEFAULT_NONE RET``

Displays a dialog allowing the user to select a printer from the printers
available on the current machine. If ``DEFAULT_NONE`` is ``true`` then an
option for '``None (Printing Disabled)``' is provided and selected by
default.  The selected printer is returned in register ``RET``.

EnumPrinters
~~~~~~~~~~~~

 Usage: ``${EnumPrinters} RET``

Enumerates the printers available on the current machine. The number of
printers available are returned in register ``RET``. The names of the
available printers remain on the stack to be popped off by the caller. If
``-1`` is returned then an error has occurred and the error message remains
on the stack.

AddPrinter
~~~~~~~~~~

 Usage: ``${AddPrinter} NAME PORT DRIVER RET``

Installs a printer driver for the printer ``NAME`` using the port ``PORT``.
The driver must have been previously installed and ``DRIVER`` provides the
installed driver's name. A return value is returned in register ``RET``. It
will be ``1`` on success or ``0`` on failure. If a failure occurs then an
error message remains on the stack.

DeletePrinter
~~~~~~~~~~~~~

 Usage: ``${DeletePrinter} NAME RET``

Deletes a printer that's available on this machine. ``NAME`` is the name of
the printer to be deleted. A return value is returned in register ``RET``. It
will be ``1`` on success or ``0`` on failure. If a failure occurs then an
error message remains on the stack.

EnumPorts
~~~~~~~~~

 Usage: ``${EnumPorts} RET``

Enumerates the ports available on the current machine. The number of ports
available are returned in register ``RET``. The names of the available ports
remain on the stack to be popped off by the caller. If ``-1`` is returned
then an error has occurred and the error message remains on the stack.

AddPort
~~~~~~~

 Usage: ``${AddPort} NAME RET``

Adds a new port on the current machine. The port will be named ``NAME``. A
return value is returned in register ``RET``. It will be ``1`` on success or
``0`` on failure. If a failure occurs then an error message remains on the
stack.

NOTE: I've only been able to successfully add RedMon redirect (``RPT?``)
ports. More general ports (``LPT?``, ``COM?``, etc.) all seem to fail. This
function is most useful when installing and configuring RedMon.

DeletePort
~~~~~~~~~~

 Usage: ``${DeletePort} NAME RET``

Deletes the port ``NAME`` on the current machine. A return value is returned
in register ``RET``. It will be ``1`` on success or ``0`` on failure. If a
failure occurs then an error message remains on the stack.

GetDefaultPrinter
~~~~~~~~~~~~~~~~~

 Usage: ``${GetDefaultPrinter} RET``

Gets the currently set default printer on the current machine. The name of
the printer is returned in register ``RET``. If an error occurs ``0`` is
returned and the error message remains on the stack.

SetDefaultPrinter
~~~~~~~~~~~~~~~~~

 Usage: ``${SetDefaultPrinter} NAME RET``

Sets the default printer on the current machine to ``NAME``. If an error
occurs ``0`` is returned and the error message remains on the stack.

NOTE: **Windows 10** will use the last printer printed to as the default
printer. This can be overridden by disabling ``LegacyDefaultPrinterMode`` in
the registry before calling ``SetDefaultPrinter``.

AddPrinterDriver
~~~~~~~~~~~~~~~~

 Usage: ``${AddPrinterDriver} INIFILE RET``

Adds a printer driver defined by ``INIFILE``. The Driver INI file format is
documented under `Driver INI File Documentation`_. If an error occurs ``0``
is returned and the error message remains on the stack.

ConfigureRedMonPort
~~~~~~~~~~~~~~~~~~~

 Usage: ``${ConfigureRedMonPort} NAME COMMAND RET``

Configures a RedMon port to redirect data to the specified command. ``NAME``
is the name of the port to configure, usually taking the form of ``RPT?``.
``COMMAND`` is the command to be executed when data is received by the port.
RedMon must have already been installed through some other means before this
function can be called. If an error occurs ``0`` is returned and the error
message remains on the stack.

Driver INI File Documentation
-----------------------------

The driver INI file describes the files to be installed using the
``AddPinterDriver`` function. It should be included in the same directory as
the driver files it describes. The only required section it has is
``[driver]``. The following are valid settings within the section.

- ``version``: This must be set to ``3``.

- ``name``: Install the printer driver under this name. This is the name used
  to reference the driver when using fuctions such as ``AddPrinter``.

- ``environment``: Must be ``Windows NT x86`` or  ``Windows x64`` depending on
  whether the driver is for a 32-bit or 64-bit architecture respectively.

- ``driver``: The name of the drivers main DLL file.

- ``datafile``: The data file used by the driver.

- ``configfile``: The DLL that presents the driver configuration UI.

- ``helpfile``: The help file for the driver.

- ``depfiles``: Additional files used by the driver separated by
  semi-colons (;).

Driver INI Example
~~~~~~~~~~~~~~~~~~

The following is an example of a driver INI file.  The ``depfiles`` setting
should be on a single line with items separated by semi-colons (;). It has been
split across lines here for readability.

::

  [driver]
  version=3
  name=HP Color LaserJet 4550 PCL 5
  environment=Windows NT x86
  driver=UNIDRV.DLL
  datafile=HPMCPC25.GPD
  configfile=UNIDRVUI.DLL
  helpfile=UNIDRV.HLP
  depfiles=HPZLSLHN.DLL;HPZSSLHN.DLL;HPZUILHN.DLL;HPCDMCLH.DLL;HPZ5RLHN.DLL;
    HPZSMLHN.GPD;HPZSTLHN.DLL;HPMCPD25.CFG;HPZ5CLHN.INI;HPMCPDP5.XML;
    HPZSCLHN.DTD;HPZEVLHN.DLL;HPZIDR12.DLL;HPZINW12.DLL;HPZIPM12.DLL;
    HPZIPR12.DLL;HPZIPT12.DLL;HPZISN12.DLL;HPBMIAPI.DLL;HPBMINI.DLL;
    HPBOID.DLL;HPBOIDPS.DLL;HPBPRO.DLL;HPBPROPS.DLL;HPEACLHN.HPI;
    UNIRES.DLL;STDNAMES.GPD;STDDTYPE.GDL;STDSCHEM.GDL;STDSCHMX.GDL;

LICENSE
-------

Copyright (C) 2016 Robert Gill

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

This plugin incorporates data structures from RedMon, which is Copyright (C)
Ghostgum Software Pty Ltd. and also licensed under the GNU General Public
License.

.. _`Mingw-w64 sources`: https://sourceforge.net/projects/mingw-w64/files/mingw-w64/mingw-w64-release/mingw-w64-v5.0.0.tar.bz2/download
