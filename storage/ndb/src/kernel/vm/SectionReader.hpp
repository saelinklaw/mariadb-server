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

#ifndef SECTION_READER_HPP
#define SECTION_READER_HPP

#include <ndb_types.h>

class SectionReader {
public:
  SectionReader(struct SegmentedSectionPtr &,
		class SectionSegmentPool &);

  void reset();
  bool step(Uint32 len);
  bool getWord(Uint32 * dst);
  bool peekWord(Uint32 * dst) const ;
  bool peekWords(Uint32 * dst, Uint32 len) const;
  Uint32 getSize() const;
  bool getWords(Uint32 * dst, Uint32 len);

private:
  Uint32 m_pos;
  Uint32 m_len;
  class SectionSegmentPool & m_pool;
  class SectionSegment * m_head;
  class SectionSegment * m_currentSegment;
};

inline
Uint32 SectionReader::getSize() const
{
  return m_len;
}

#endif
