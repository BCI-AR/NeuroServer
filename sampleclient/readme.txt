NeuroServer

A collection of programs to translate between EEG data and TCP network
messages. This is a part of the OpenEEG project, see http://openeeg.sf.net
for details.

Copyright (C) 2003, 2004 Rudi Cilibrasi (cilibrar@ofb.net)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA


This is a small sample application to show how to connect to the NeuroServer.
It is written using GTK 2.0.

It is just supposed to graphically display a signal waveform onscreen.

It tries to connect to localhost:8336

It doesn't do any fancy EDF stuff in the interest of keeping this
example small; for a real application, you will need to pay attention
to the fields in the EDF header (that are printed out).  You can use
your own EDF reading routines, or use the NeuroServer openedf.c.

Good luck,

Rudi Cilibrasi

cilibrar@ofb.net

