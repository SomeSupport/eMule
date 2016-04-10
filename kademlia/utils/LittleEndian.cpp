/*
Copyright (C)2003 Barry Dunne (http://www.emule-project.net)
 
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// Note To Mods //
/*
Please do not change anything here and release it..
There is going to be a new forum created just for the Kademlia side of the client..
If you feel there is an error or a way to improve something, please
post it in the forum first and let us look at it.. If it is a real improvement,
it will be added to the offical client.. Changing something without knowing
what all it does can cause great harm to the network if released in mass form..
Any mod that changes anything within the Kademlia side will not be allowed to advertise
there client on the eMule forum..
*/

#include "stdafx.h"
#include "./LittleEndian.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace Kademlia
{
	//NOTE: avoid those function whenever possible -> terribly slow
	uint16 le(uint16 uVal)
	{
		uint32 uB0 = (uVal      ) & 0xFF;
		uint32 uB1 = (uVal >>  8) & 0xFF;
		return (uint16) ((uB0 << 8) | uB1);
	}

	//NOTE: avoid those function whenever possible -> terribly slow
	uint32 le(uint32 uVal)
	{
		uint32 uB0 = (uVal      ) & 0xFF;
		uint32 uB1 = (uVal >>  8) & 0xFF;
		uint32 uB2 = (uVal >> 16) & 0xFF;
		uint32 uB3 = (uVal >> 24) & 0xFF;
		return (uB0 << 24) | (uB1 << 16) | (uB2 << 8) | uB3;
	}

	//NOTE: avoid those function whenever possible -> terribly slow
	uint64 le(uint64 uVal)
	{
		uint32 uB0 = (uint32)((uVal      ) & 0xFF);
		uint32 uB1 = (uint32)((uVal >>  8) & 0xFF);
		uint32 uB2 = (uint32)((uVal >> 16) & 0xFF);
		uint32 uB3 = (uint32)((uVal >> 24) & 0xFF);
		uint32 uB4 = (uint32)((uVal >> 32) & 0xFF);
		uint32 uB5 = (uint32)((uVal >> 40) & 0xFF);
		uint32 uB6 = (uint32)((uVal >> 48) & 0xFF);
		uint32 uB7 = (uint32)((uVal >> 56) & 0xFF);
		return ((uint64)uB0 << 56) | ((uint64)uB1 << 48) | ((uint64)uB2 << 40) | ((uint64)uB3 << 32) |
		       ((uint64)uB4 << 24) | ((uint64)uB5 << 16) | ((uint64)uB6 << 8) | (uint64)uB7;
	}
}
