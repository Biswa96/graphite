#ifndef LOADEDFACE_INCLUDE
#define LOADEDFACE_INCLUDE

#include "GlyphFace.h"
#include "Silf.h"
#include "TtfUtil.h"
#include "Main.h"
#include "graphiteng/IFace.h"
#include "FeatureMap.h"

class Segment;
class Features;

#define ktiCmap MAKE_TAG('c','m','a','p')
#define ktiHead MAKE_TAG('h','e','a','d')
#define ktiGlyf MAKE_TAG('g','l','y','f')
#define ktiHdmx MAKE_TAG('h','d','m','x')
#define ktiHhea MAKE_TAG('h','h','e','a')
#define ktiHmtx MAKE_TAG('h','m','t','x')
#define ktiLoca MAKE_TAG('l','o','c','a')
#define ktiKern MAKE_TAG('k','e','r','n')
#define ktiMaxp MAKE_TAG('m','a','x','p')
#define ktiName MAKE_TAG('n','a','m','e')
#define ktiOs2  MAKE_TAG('O','S','/','2')
#define ktiPost MAKE_TAG('p','o','s','t')
#define ktiFeat MAKE_TAG('F','e','a','t')
#define ktiGlat MAKE_TAG('G','l','a','t')
#define ktiGloc MAKE_TAG('G','l','o','c')
#define ktiSilf MAKE_TAG('S','i','l','f')
#define ktiSile MAKE_TAG('S','i','l','e')
#define ktiSill MAKE_TAG('S','i','l','l')

class LoadedFace 	// an IFace loaded into an object
{
public:
    const void *getTable(unsigned int name, size_t *len) const { return m_face->getTable(name, len); }
    float advance(unsigned short id) const { return m_glyphs[id].advance().x; }
    const Silf *silf(int i) const { return ((i < m_numSilf) ? m_silfs + i : (const Silf *)NULL); }
    void runGraphite(Segment *seg, const Silf *silf) const;
    uint16 findPseudo(uint32 uid) const { return (m_numSilf) ? m_silfs[0].findPseudo(uid) : 0; }

public:
    LoadedFace(const IFace *face) : m_face(face), m_glyphs(NULL), m_silfs(NULL)  {}
    ~LoadedFace();
    const GlyphFace *glyph(unsigned short glyphid) const { return m_glyphs + glyphid; } // m_glyphidx[glyphid]; }
    float getAdvance(unsigned short glyphid, float scale) const { return advance(glyphid) * scale; }
    unsigned short upem() const { return m_upem; }
    unsigned short numGlyphs() const { return m_numGlyphs; }
    bool readGlyphs();
    bool readGraphite();
    bool readFeatures() { return m_features.readFont(m_face); }
    const Silf *chooseSilf(uint32 script) const;
    Features *newFeatures(uint32 lang) { return m_features.newFeatures(lang); }
    FeatureRef *feature(uint8 index) { return m_features.feature(index); }
    uint16 glyphAttr(uint16 gid, uint8 gattr) { return (gattr < m_numAttrs && gid < m_numGlyphs) ? m_glyphs[gid].getAttr(gattr) : 0; }

private:

    const IFace *m_face;                  // Where to get tables
    unsigned short m_numGlyphs;     // number of glyphs in the font
    // unsigned short *m_glyphidx;     // index for each glyph id in the font
    // unsigned short m_readglyphs;    // how many glyphs have we in m_glyphs?
    // unsigned short m_capacity;      // how big is m_glyphs
    GlyphFace *m_glyphs;            // list of actual glyphs indexed by glyphidx, 1 base
    unsigned short m_upem;          // design units per em
    unsigned short m_numAttrs;      // number of glyph attributes per glyph
    unsigned short m_numSilf;       // number of silf subtables in the silf table
    Silf *m_silfs;                   // silf subtables.
    FeatureMap m_features;
    
  private:		//defensive on m_glyphs and m_silfs
    LoadedFace(const LoadedFace&);
    LoadedFace& operator=(const LoadedFace&);
};

#endif // LOADEDFACE_INCLUDE