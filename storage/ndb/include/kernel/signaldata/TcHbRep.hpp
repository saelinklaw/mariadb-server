/* Copyright (c) 2003-2005 MySQL AB
   Use is subject to license terms

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1335  USA */

#ifndef TC_HB_REP_H
#define TC_HB_REP_H

#include "SignalData.hpp"

/**
 * @class TcHbRep
 * @brief Order tc refresh(exetend) the timeout counters for this 
 *        transaction
 *
 * - SENDER:    API
 * - RECEIVER:  TC
 */
class TcHbRep {
  /**
   * Receiver(s)
   */
  friend class Dbtc;         // Receiver

  /**
   * Sender(s)
   */
  friend class NdbTransaction;      

  /**
   * For printing
   */
  friend bool printTC_HBREP(FILE *, const Uint32 *, Uint32, Uint16);

public:
  /**
   * Length of signal
   */
  STATIC_CONST( SignalLength = 3 );

private:

  /**
   * DATA VARIABLES
   */

  Uint32 apiConnectPtr;       // DATA 0
  UintR transId1;             // DATA 1
  UintR transId2;             // DATA 2
};


#endif
