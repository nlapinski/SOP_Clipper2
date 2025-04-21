// ======= SOP_Clipper.cpp =======

#include "SOP_Clipper.h"
// clipper
#include <clipper2/clipper.h>
#include <deque>
#include <limits.h>
#include <map>

using namespace UT::Literal;
using namespace Clipper2Lib;

const UT_StringHolder SOP_Clipper::theSOPTypeName("clipper"_sh);

// Registration
void newSopOperator(OP_OperatorTable* table) {
  
  table->addOperator(new OP_Operator(
    SOP_Clipper::theSOPTypeName,   // Internal name
    "Clipper",                     // UI name
    SOP_Clipper::myConstructor,    // How to build the SOP
    SOP_Clipper::buildTemplates(), // My parameters
    1,                          // Min # of sources
    2,                          // Max # of sources
    nullptr,                    // Custom local variables (none)
    OP_FLAG_GENERATOR));        // Flag it as generator

}

// Constructor
OP_Node* SOP_Clipper::myConstructor(OP_Network* net, const char* name, OP_Operator* op) {
  return new SOP_Clipper(net, name, op);
}

SOP_Clipper::SOP_Clipper(OP_Network *net, const char *name, OP_Operator *op)
    : SOP_Node(net, name, op) {
  //
}

SOP_Clipper::~SOP_Clipper() {
  //
}

OP_ERROR SOP_Clipper::cookMySop(OP_Context &context) {
  return cookMyselfAsVerb(context);
}

////////////////////////////////////
/// Verb Class
////////////////////////////////////

class SOP_ClipperVerb : public SOP_NodeVerb
{
public:
  SOP_ClipperVerb() {}
  virtual ~SOP_ClipperVerb() {}
  virtual SOP_NodeParms* allocParms() const { return new SOP_ClipperParms(); }
  virtual UT_StringHolder name() const { return SOP_Clipper::theSOPTypeName; }
  virtual CookMode cookMode(const SOP_NodeParms* parms) const { return COOK_DUPLICATE; }
  virtual void cook(const CookParms& cookparms) const;
  static const SOP_NodeVerb::Register<SOP_ClipperVerb> theVerb;
};


const SOP_NodeVerb::Register<SOP_ClipperVerb> SOP_ClipperVerb::theVerb;

const SOP_NodeVerb*
SOP_Clipper::cookVerb() const
{
  
  return SOP_ClipperVerb::theVerb.get();
}

// Join Type - Square, Bevel, Round, Miter
// End type - Polygon, Joined, Butt, Square, Round
// Operation - Intersect, Union, Difference, Exclusive Or, Inflate

static const char* theDsFile = R"THEDSFILE(
{
    name	parameters

    parm {
        name    "operation"
        label   "operation"
        type    ordinal
        default { "0" }
        menu {
            "intersect" "intersect"
            "union"  "union"
            "difference"  "difference"
            "exclusive_or"      "exclusive_or"
            "inflate"  "inflate"
        }
        parmtag { "script_callback_language" "python" }
    }
    parm {
        name    "join_type"
        label   "join_type"
        type    ordinal
        default { "0" }
        menu {
            "square" "square"
            "bevel" "bevel"
            "round" "round"
            "miter" "miter"
        }
        parmtag { "script_callback_language" "python" }
    }
    parm {
        name    "end_type"
        label   "end_type"
        type    ordinal
        default { "0" }
        menu {
            "polygon" "polygon"
            "joined" "joined"
            "butt" "butt"
            "square" "square"
            "round" "round"
        }
        parmtag { "script_callback_language" "python" }
    }
    parm {
        name    "Scale"
        label   "Scale"
        type    float
        default { "1000" }
        range   { 1! 1000 }
        parmtag { "script_callback_language" "python" }
    }
    parm {
        name    "InflateWidth"
        label   "InflateWidth"
        type    float
        default { "10" }
        range   { 0! 1000 }
        parmtag { "script_callback_language" "python" }
    }
    parm {
        name    "inflate_merge"
        label   "inflate_merge"
        type    toggle
        default { "0" }
        parmtag { "script_callback_language" "python" }
    }

})THEDSFILE";;

PRM_Template*
SOP_Clipper::buildTemplates()
{
  static PRM_TemplateBuilder templ("SOP_Clipper.cpp"_sh, theDsFile);
  return templ.templates();
}



void SOP_ClipperVerb::cook(const CookParms& cookparms) const
{
  auto&& parms = cookparms.parms<SOP_ClipperParms>();
  GU_Detail* gdp = cookparms.gdh().gdpNC();
  const GU_Detail* gdp_2 = cookparms.inputGeoHandle(1).gdp(); // second input geometry, if any

  // Check inputs
  if (!cookparms.hasInput(0))
  {
    // mark a fatal error on parm 0 and stop
    auto r = cookparms.sopAddError(SOP_MESSAGE, "Clipper: Missing first input geometry.");
    return;
  }
  
  // Scale factor to convert world floats to integer coordinates
  double FIXED_SCALE = parms.getScale();
  //just debug info
  {
    char buf[256];
    snprintf(buf,256,"Fixed point scale = %g", FIXED_SCALE);
    auto r = cookparms.sopAddWarning(SOP_MESSAGE, buf);
  }

  // Extract subject paths
  Paths64 subject;
  GA_Range prims1 = gdp->getPrimitiveRange();
  for (GA_Offset primOff : prims1)
  {
    const GU_PrimPoly* prim = static_cast<const GU_PrimPoly*>(gdp->getPrimitive(primOff));
    if (!prim || prim->getVertexCount() < 3)
      continue;

    Path64 path;
    for (int i = 0; i < prim->getVertexCount(); ++i)
    {
      GA_Offset vtxOff = prim->getVertexOffset(i);
      GA_Offset ptOff = vtxOff;
      UT_Vector3 pos = gdp->getPos3(ptOff);

      int64_t ix = int64_t(pos.x() * FIXED_SCALE);
      int64_t iy = int64_t(pos.y() * FIXED_SCALE);
      path.emplace_back(ix, iy);
    }
    subject.push_back(std::move(path));
  }

  // Extract clip paths (if provided)
  Paths64 clip;
  if (gdp_2)
  {
    GA_Range prims2 = gdp_2->getPrimitiveRange();
    for (GA_Offset primOff : prims2)
    {
      const GU_PrimPoly* prim = static_cast<const GU_PrimPoly*>(gdp_2->getPrimitive(primOff));
      if (!prim || prim->getVertexCount() < 3)
        continue;

      Path64 path;
      for (int i = 0; i < prim->getVertexCount(); ++i)
      {
        GA_Offset vtxOff = prim->getVertexOffset(i);
        GA_Offset ptOff = vtxOff;
        UT_Vector3 pos = gdp_2->getPos3(ptOff);

        int64_t ix = int64_t(pos.x() * FIXED_SCALE);
        int64_t iy = int64_t(pos.y() * FIXED_SCALE);
        path.emplace_back(ix, iy);
      }
      clip.push_back(std::move(path));
    }
  }

  // Perform operation

  Paths64 solution;
  switch (parms.getOperation())
  {
  case SOP_ClipperEnums::Operation::INTERSECT: // Intersect
    solution = Intersect(subject, clip, FillRule::NonZero);
    break;
  case SOP_ClipperEnums::Operation::UNION: // Union
    solution = Union(subject, clip, FillRule::EvenOdd);
    break;
  case SOP_ClipperEnums::Operation::DIFFERENCE: // Sub
    solution = Difference(subject, clip, FillRule::NonZero);
    break;
  case SOP_ClipperEnums::Operation::EXCLUSIVE_OR: // XOR
    solution = Clipper2Lib::Xor(subject, clip, FillRule::EvenOdd);
    break;
  case SOP_ClipperEnums::Operation::INFLATE: // Curve buffering
    // merge with 2nd input if available
    Clipper2Lib::JoinType joinType = parms.getJoin_type() == SOP_ClipperEnums::Join_type::SQUARE ? Clipper2Lib::JoinType::Square :
      parms.getJoin_type() == SOP_ClipperEnums::Join_type::BEVEL ? Clipper2Lib::JoinType::Bevel :
      parms.getJoin_type() == SOP_ClipperEnums::Join_type::ROUND ? Clipper2Lib::JoinType::Round : Clipper2Lib::JoinType::Miter;

    Clipper2Lib::EndType endType = parms.getEnd_type() == SOP_ClipperEnums::End_type::POLYGON ? Clipper2Lib::EndType::Polygon :
      parms.getEnd_type() == SOP_ClipperEnums::End_type::JOINED ? Clipper2Lib::EndType::Joined :
      parms.getEnd_type() == SOP_ClipperEnums::End_type::BUTT ? Clipper2Lib::EndType::Butt :
      parms.getEnd_type() == SOP_ClipperEnums::End_type::SQUARE ? Clipper2Lib::EndType::Square : Clipper2Lib::EndType::Round;


    bool inflate_merge = parms.getInflate_merge();
    if(inflate_merge) {
      // merge subject and clip before inflating
      auto tmp = Union(subject, clip, FillRule::EvenOdd);
      solution = Clipper2Lib::InflatePaths(tmp, (double)parms.getInflatewidth(), joinType, endType);
    }
    else {
      // no merge, just inflate subject - these might need to be restructured at some point 
      solution = Clipper2Lib::InflatePaths(subject, (double)parms.getInflatewidth(), joinType, endType);
    }
    break;
  }

  // Clear existing gdp/geo for write back
  gdp->clearAndDestroy();

  // Write solution paths back as polys
  for (auto& solPath : solution)
  {
    int npt = solPath.size();
    GU_PrimPoly* curve = GU_PrimPoly::build(gdp, npt, GU_POLY_CLOSED, 0);

    for (size_t i = 0; i < solPath.size(); ++i)
    {
      double fx = double(solPath[i].x) / FIXED_SCALE;
      double fy = double(solPath[i].y) / FIXED_SCALE;

      //new point
      GA_Offset po = gdp->appendPoint();
      gdp->setPos3(po, UT_Vector3(fx, fy, 0.0f));
      curve->setVertexPoint(i, po);
    }
  }

  gdp->getP()->bumpDataId();
  UT_AutoInterrupt boss("ClipperInterrupt");
  if (boss.wasInterrupted()) return;
}

