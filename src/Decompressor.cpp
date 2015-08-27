/*  Copyright (c) 2012, Siyuan Fu <fusiyuan2010@gmail.com>
    Copyright (c) 2015, SIL International
    All rights reserved.
    
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    
    1. Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
    
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    
    3. Neither the name of the copyright holder nor the names of its
       contributors may be used to endorse or promote products derived from
       this software without specific prior written permission.
    
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
    POSSIBILITY OF SUCH DAMAGE.
*/
#include <cassert>

#include "inc/Decompressor.h"
#include "inc/Compression.h"

using namespace lz4;

namespace {

inline
u32 read_literal(u8 const * &s, u8 const * const e, u32 l) {
    if (unlikely(l == 15) && likely(s != e))
    {
        u8 b = 0;
        do { l += b = *s++; } while(b==0xff && s != e);
    }
    return l;
}

bool read_sequence(u8 const * &src, u8 const * const end, u8 const * &literal, u32 & literal_len, u32 & match_len, u32 & match_dist)
{
    u8 const token = *src++;
    
    literal_len = read_literal(src, end, token >> 4);
    literal = src;
    src += literal_len;
    
    if (unlikely(src == end))
        return false;
    
    match_dist  = *src++ << 8;
    match_dist |= *src++;
    match_len = read_literal(src, end, token & 0xf);
    
    return true;
}

}

int lz4::decompress(void const *in, size_t in_size, void *out, size_t out_size)
{
    u8 const *       src     = static_cast<u8 const *>(in),
             *       literal = 0,
             * const src_end = src + in_size;

    u8 *       dst     = static_cast<u8*>(out),
       * const dst_end = dst + out_size;
    
    u32 literal_len = 0,
        match_len = 0,
        match_dist = 0;
    
    while (read_sequence(src, src_end, literal, literal_len, match_len, match_dist))
    {
        // Copy in literal
        if (unlikely(literal + literal_len + sizeof(unsigned long) > src_end
                  || dst + literal_len + sizeof(unsigned long) > dst_end)) 
            return -1;
        dst = memcpy_nooverlap(dst, literal, literal_len);
        
        // Copy, possibly repeating, match from earlier in the
        //  decoded output.
        u8 const * const pcpy = dst - match_dist;
        if (unlikely(pcpy < static_cast<u8*>(out) 
                  || dst + match_len + MINMATCH  + sizeof(unsigned long) > dst_end)) 
            return -1;
        dst = memcpy_(dst, pcpy, match_len + MINMATCH);
    }
    
    if (unlikely(literal + literal_len > src_end
              || dst + literal_len > dst_end)) 
        return -1;
    dst = memcpy_nooverlap_surpass(dst, literal, literal_len);
    
    return dst - (u8*)out;
}

