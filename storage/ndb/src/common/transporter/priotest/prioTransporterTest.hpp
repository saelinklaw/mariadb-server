/* Copyright (c) 2003, 2005, 2006 MySQL AB
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

#ifndef PRIO_TRANSPORTER_TEST_HPP
#define PRIO_TRANSPORTER_TEST_HPP


enum TestType {
  TestTCP,
  TestSCI,
  TestSHM
};

extern int basePortTCP;

int prioTransporterTest(TestType tt, const char * pName, 
			 int argc, const char **argv);



#endif
