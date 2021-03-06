/* Copyright (c) 2003-2006 MySQL AB
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

#include <ndb_global.h>
#include <my_sys.h>
#include <NdbMutex.h>

NdbMutex *g_ndb_connection_mutex = NULL;

void
ndb_init_internal()
{
  if (!g_ndb_connection_mutex)
    g_ndb_connection_mutex = NdbMutex_Create();
}

int
ndb_init()
{
  if (my_init()) {
    const char* err = "my_init() failed - exit\n";
    write(2, err, strlen(err));
    exit(1);
  }
  ndb_init_internal();
  return 0;
}

void
ndb_end_internal()
{
  if (g_ndb_connection_mutex)
    NdbMutex_Destroy(g_ndb_connection_mutex);
}

void
ndb_end(int flags)
{
  my_end(flags);
  ndb_end_internal();
}
