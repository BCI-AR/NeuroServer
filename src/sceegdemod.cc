/* -*- Mode: c++ -*-
*******************************************************************************
*
* File:         elenco_fm_demo_no_gui.cc
* Description:  
*
*******************************************************************************
*/
/*
 * Copyright 2001 Free Software Foundation, Inc.
 * 
 * This file is part of GNU Radio
 * 
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

//
// This is a demo of a complete FM reception path.
//
// We start with a really cheap AM/FM radio kit, the Elenco Radio Kit
// AM-FM 108 (It's all discrete transistors, no ICs).  We suck the FM
// IF signal out at test point 9 (output of second IF amp), and feed
// it through a buffer board and from there into a 50 Ohm terminated
// input on the Measurement Computing PCI DAS4020/12 A/D board.
//
// Think of the buffer board as a low capacitance input, low output impedance 
// converter that can successfully drive IF signals through 2 meters of coax.
// [I ought to scan in the schematic]


#include <GrFileSource.h>
#include <GrFileSink.h>
#include <GrFFTSink.h>
#include <VrFixOffset.h>
#include <VrComplexFIRfilter.h>
#include <VrQuadratureDemod.h>
#include <VrRealFIRfilter.h>
#include <VrAudioSink.h>
#include <VrFileSink.h>
#include <VrAudioSink.h>
#include <VrConnect.h>
#include <VrMultiTask.h>


const int inputRate = 48000;
//const float FM_IF_Freq = 8.41797e6;	// this is the aliased FM IF from the ELENCO kit
const float FM_IF_Freq = 3000;	// this is the aliased FM IF from the ELENCO kit

const float IF_Freq = FM_IF_Freq;

const int chanTaps = 75;
const int CFIRdecimate = 125;
const float chanGain = 20.0;


const float FMdemodGain = 2200;

const int RFIRdecimate = 5;
const int ifTaps = 50;
const float ifGain = 1.0;

const int quadRate = inputRate / CFIRdecimate;
const int audioRate = quadRate / RFIRdecimate;


int main(int argc, char **argv)
{
  int i;
  float volume = 1.0;

  cerr << "Input Sampling Rate: " << inputRate << endl;
  cerr << "Complex FIR decimation factor: " << CFIRdecimate << endl;
  cerr << "QuadDemod Sampling Rate: " << quadRate << endl;
  cerr << "Real FIR decimation factor: " << RFIRdecimate << endl;
  cerr << "Audio Sampling Rate: " << audioRate << endl;


  // --> short
  GrFileSource *source = 
    new GrFileSource(2, inputRate, "40to280sweep_m.raw", false);

  // short --> short 
  VrFixOffset<short,short> *offset_fixer =
    new VrFixOffset<short,short>();

  // short --> VrComplex
  VrComplexFIRfilter<short>* chanFilter = 
    new VrComplexFIRfilter<short>(CFIRdecimate, chanTaps, IF_Freq, chanGain);

  // VrComplex --> float
  VrQuadratureDemod<float> *FMdemod =
    new VrQuadratureDemod<float>(volume * FMdemodGain);

  // float --> short
  VrRealFIRfilter<float,short> *ifFilter = 
    new VrRealFIRfilter<float,short>(RFIRdecimate, audioRate/2,
				     ifTaps, ifGain);

  // short --> nil
  GrFileSink *audio_sink = new GrFileSink(2, "demod.raw");

  //connect the modules together

  NWO_CONNECT (source, offset_fixer);
  NWO_CONNECT (offset_fixer, chanFilter);
  NWO_CONNECT (chanFilter, FMdemod);
  NWO_CONNECT (FMdemod, ifFilter);
  NWO_CONNECT (ifFilter, audio_sink);

  // add the sink to the active list

  VrMultiTask *m = new VrMultiTask ();
  m->add (audio_sink);
  m->start ();		// initialize

  //for (i = 0; i < 852480 ; i++) {
  for (i = 0; i < 852 ; i++) {
    try {
    m->process();
    } 
    catch (std::runtime_error e) {
      break;
    }
  }  
  delete(m);
  printf("Ok about done..\n");
}
