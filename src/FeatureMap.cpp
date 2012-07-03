/*  GRAPHITE2 LICENSING

    Copyright 2010, SIL International
    All rights reserved.

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should also have received a copy of the GNU Lesser General Public
    License along with this library in the file named "LICENSE".
    If not, write to the Free Software Foundation, 51 Franklin Street, 
    Suite 500, Boston, MA 02110-1335, USA or visit their web page on the 
    internet at http://www.fsf.org/licenses/lgpl.html.

Alternatively, the contents of this file may be used under the terms of the
Mozilla Public License (http://mozilla.org/MPL) or the GNU General Public
License, as published by the Free Software Foundation, either version 2
of the License or (at your option) any later version.
*/
#include <cstring>

#include "inc/Main.h"
#include "inc/bits.h"
#include "inc/Endian.h"
#include "inc/FeatureMap.h"
#include "inc/FeatureVal.h"
#include "graphite2/Font.h"
#include "inc/TtfUtil.h"
#include <cstdlib>
#include "inc/Face.h"


using namespace graphite2;

namespace
{
	static int cmpNameAndFeatures(const void *ap, const void *bp)
	{
		const NameAndFeatureRef & a = *static_cast<const NameAndFeatureRef *>(ap),
								& b = *static_cast<const NameAndFeatureRef *>(bp);
		return (a < b ? -1 : (b < a ? 1 : 0));
	}

	const size_t 	FEAT_HEADER		= sizeof(uint32) + 2*sizeof(uint16) + sizeof(uint32),
					FEATURE_SIZE    = sizeof(uint32)
									+ 2*sizeof(uint16)
									+ sizeof(uint32)
									+ 2*sizeof(uint16),
					FEATURE_SETTING_SIZE = sizeof(int16) + sizeof(uint16);

	uint16 readFeatureSettings(const byte * p, FeatureSetting * s, size_t num_settings)
	{
		uint16 max_val = 0;
        for (FeatureSetting * const end = s + num_settings; s != end; ++s)
        {
            new (s) FeatureSetting(be::read<int16>(p), be::read<uint16>(p));
            if (uint16(s->value()) > max_val) 	max_val = s->value();
        }

        return max_val;
	}
}

FeatureRef::FeatureRef(const Face & face,
	unsigned short & bits_offset, uint32 max_val,
	uint32 name, uint16 uiName, uint16 flags,
	FeatureSetting *settings, uint16 num_set) throw()
: m_pFace(&face),
  m_nameValues(settings),
  m_mask(mask_over_val(max_val)),
  m_max(max_val),
  m_id(name),
  m_nameid(uiName),
  m_flags(flags),
  m_numSet(num_set)
{
	const uint8 need_bits = bit_set_count(m_mask);
	m_index = (bits_offset + need_bits) / SIZEOF_CHUNK;
	if (m_index > bits_offset / SIZEOF_CHUNK)
		bits_offset = m_index*SIZEOF_CHUNK;
	m_bits = bits_offset % SIZEOF_CHUNK;
	bits_offset += need_bits;
	m_mask <<= m_bits;
}

//FeatureRef::FeatureRef(const FeatureRef & rhs)
//: m_pFace(rhs.m_pFace), m_nameValues((rhs.m_nameValues)? gralloc<FeatureSetting>(rhs.m_numSet) : NULL),
//  m_mask(rhs.m_mask), m_max(rhs.m_max), m_id(rhs.m_id),
//  m_nameid(rhs.m_nameid), m_flags(rhs.m_flags), m_numSet(rhs.m_numSet),
//  m_bits(rhs.m_bits), m_index(rhs.m_index)
//{
//    // most of the time these name values aren't used, so NULL might be acceptable
//    if (rhs.m_nameValues)
//    {
//        memcpy(m_nameValues, rhs.m_nameValues, sizeof(FeatureSetting) * m_numSet);
//    }
//}

FeatureRef::~FeatureRef() throw()
{
    free(m_nameValues);
}

bool FeatureMap::readFeats(const Face & face)
{
    size_t feat_len;
    const byte *p = face.getTable(TtfUtil::Tag::Feat, &feat_len);
    if (!p) return true;
    if (feat_len < FEAT_HEADER) return false;

    const byte *const feat_start = p,
    		   *const feat_end = p + feat_len;

    const uint32 version = be::read<uint32>(p);
    m_numFeats = be::read<uint16>(p);
    be::skip<uint16>(p);
    be::skip<uint32>(p);

    // Sanity checks
    if (m_numFeats == 0) 	return true;
    if (version < 0x00010000 ||
    	p + m_numFeats*FEATURE_SIZE > feat_end)
    {   //defensive
    	m_numFeats = 0;
    	return false;
    }

    m_feats = gralloc<FeatureRef>(m_numFeats);
    uint16 * const	defVals = gralloc<uint16>(m_numFeats);
    unsigned short bits = 0;     //to cause overflow on first Feature

    for (int i = 0, ie = m_numFeats; i != ie; i++)
    {
        const uint32 	label   = version < 0x00020000 ? be::read<uint16>(p) : be::read<uint32>(p);
        const uint16 	num_settings = be::read<uint16>(p);
        if (version >= 0x00020000)
        	be::skip<uint16>(p);
        const byte * const feat_setts = feat_start + be::read<uint32>(p);
        const uint16 	flags  = be::read<uint16>(p),
        				uiName = be::read<uint16>(p);

        if (feat_setts + num_settings * FEATURE_SETTING_SIZE > feat_end)
        {
            free(defVals);
            return false;
        }

        FeatureSetting *uiSet;
        uint32 maxVal;
        if (num_settings != 0)
        {
			uiSet = gralloc<FeatureSetting>(num_settings);
			maxVal = readFeatureSettings(feat_setts, uiSet, num_settings);
			defVals[i] = uiSet[0].value();
        }
        else
        {
        	uiSet = 0;
        	maxVal = 0xffffffff;
        	defVals[i] = 0;
        }

		::new (m_feats + i) FeatureRef (face, bits, maxVal,
									   label, uiName, flags,
									   uiSet, num_settings);
    }
    m_defaultFeatures = new Features(bits/(sizeof(uint32)*8) + 1, *this);
    m_pNamedFeats = new NameAndFeatureRef[m_numFeats];
    for (int i = 0; i < m_numFeats; ++i)
    {
    	m_feats[i].applyValToFeature(defVals[i], *m_defaultFeatures);
        m_pNamedFeats[i] = m_feats+i;
    }
    
    free(defVals);

    qsort(m_pNamedFeats, m_numFeats, sizeof(NameAndFeatureRef), &cmpNameAndFeatures);

    return true;
}

bool SillMap::readFace(const Face & face)
{
    if (!m_FeatureMap.readFeats(face)) return false;
    if (!readSill(face)) return false;
    return true;
}


bool SillMap::readSill(const Face & face)
{
    size_t lSill;
    const byte *pSill = face.getTable(TtfUtil::Tag::Sill, &lSill);
    const byte *pBase = pSill;

    if (!pSill) return true;
    if (lSill < 12) return false;
    if (be::read<uint32>(pSill) != 0x00010000UL) return false;
    m_numLanguages = be::read<uint16>(pSill);
    m_langFeats = new LangFeaturePair[m_numLanguages];
    if (!m_langFeats || !m_FeatureMap.m_numFeats) { m_numLanguages = 0; return true; }        //defensive

    pSill += 6;     // skip the fast search
    if (lSill < m_numLanguages * 8U + 12) return false;

    for (int i = 0; i < m_numLanguages; i++)
    {
        uint32 langid = be::read<uint32>(pSill);
        uint16 numSettings = be::read<uint16>(pSill);
        uint16 offset = be::read<uint16>(pSill);
        if (offset + 8U * numSettings > lSill && numSettings > 0) return false;
        Features* feats = new Features(*m_FeatureMap.m_defaultFeatures);
        const byte *pLSet = pBase + offset;

        // Apply langauge specific settings
        for (int j = 0; j < numSettings; j++)
        {
            uint32 name = be::read<uint32>(pLSet);
            uint16 val = be::read<uint16>(pLSet);
            pLSet += 2;
            const FeatureRef* pRef = m_FeatureMap.findFeatureRef(name);
            if (pRef) 	pRef->applyValToFeature(val, *feats);
        }
        // Add the language id feature which is always feature id 1
        const FeatureRef* pRef = m_FeatureMap.findFeatureRef(1);
        if (pRef)	pRef->applyValToFeature(langid, *feats);

        m_langFeats[i].m_lang = langid;
        m_langFeats[i].m_pFeatures = feats;
    }
    return true;
}


Features* SillMap::cloneFeatures(uint32 langname/*0 means default*/) const
{
    if (langname)
    {
        // the number of languages in a font is usually small e.g. 8 in Doulos
        // so this loop is not very expensive
        for (uint16 i = 0; i < m_numLanguages; i++)
        {
            if (m_langFeats[i].m_lang == langname)
                return new Features(*m_langFeats[i].m_pFeatures);
        }
    }
    return new Features (*m_FeatureMap.m_defaultFeatures);
}



const FeatureRef *FeatureMap::findFeatureRef(uint32 name) const
{
    NameAndFeatureRef *it;
    
    for (it = m_pNamedFeats; it < m_pNamedFeats + m_numFeats; ++it)
        if (it->m_name == name)
            return it->m_pFRef;
    return NULL;
}

bool FeatureRef::applyValToFeature(uint32 val, Features & pDest) const
{ 
    if (val>maxVal() || !m_pFace)
      return false;
    if (pDest.m_pMap==NULL)
      pDest.m_pMap = &m_pFace->theSill().theFeatureMap();
    else
      if (pDest.m_pMap!=&m_pFace->theSill().theFeatureMap())
        return false;       //incompatible
    pDest.reserve(m_index);
    pDest[m_index] &= ~m_mask;
    pDest[m_index] |= (uint32(val) << m_bits);
    return true;
}

uint32 FeatureRef::getFeatureVal(const Features& feats) const
{ 
  if (m_index < feats.size() && &m_pFace->theSill().theFeatureMap()==feats.m_pMap) 
    return (feats[m_index] & m_mask) >> m_bits; 
  else
    return 0;
}


