/*
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
                
*/
                

#ifndef __EDFMACROS_H
#define __EDFMACROS_H

#define DECODEHEADERFIELD(packet, fieldName) \
				packet+offsetof(struct EDFPackedHeader,fieldName), sizeof((struct EDFPackedHeader *)0)->fieldName

#define DECODECHANNELFIELD(packet, fieldName, whichChannel, totalChannels) \
				packet+totalChannels*offsetof(struct EDFPackedChannelHeader,fieldName) + whichChannel*sizeof(((struct EDFPackedChannelHeader *)0)->fieldName),sizeof((struct EDFPackedChannelHeader *)0)->fieldName

#define LOADHFI(m) \
	result->m = EDFUnpackInt(DECODEHEADERFIELD(packet, m))
#define LOADHFD(m) \
	result->m = EDFUnpackDouble(DECODEHEADERFIELD(packet, m))
#define LOADHFS(m) \
	strcpy(result->m, EDFUnpackString(DECODEHEADERFIELD(packet, m)))

#define LOADCFI(m) \
	result->m = EDFUnpackInt(DECODECHANNELFIELD(packet, m, whichChannel, totalChannels))
#define LOADCFD(m) \
	result->m = EDFUnpackDouble(DECODECHANNELFIELD(packet, m, whichChannel, totalChannels))
#define LOADCFS(m) \
	strcpy(result->m, EDFUnpackString(DECODECHANNELFIELD(packet, m, whichChannel, totalChannels)))

#define STORECFI(m, i) \
	storeEDFInt(DECODECHANNELFIELD(packet, m, whichChannel, totalChannels), i)
#define STORECFD(m, d) \
	storeEDFDouble(DECODECHANNELFIELD(packet, m, whichChannel, totalChannels), d)
#define STORECFS(m, s) \
	storeEDFString(DECODECHANNELFIELD(packet, m, whichChannel, totalChannels), s)

#define STOREHFI(m, i) \
	storeEDFInt(DECODEHEADERFIELD(packet, m), i)
#define STOREHFD(m, d) \
	storeEDFDouble(DECODEHEADERFIELD(packet, m), d)
#define STOREHFS(m, s) \
	storeEDFString(DECODEHEADERFIELD(packet, m), s)

#endif
