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

