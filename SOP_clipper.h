#pragma once
// ======= SOP_Clipper.h =======
#include "hdk_helper.h" // all HDK stuff
#include <SOP_Clipper.proto.h> // for parm proto
class SOP_Clipper : public SOP_Node {
public:
  static OP_Node* myConstructor(OP_Network* net, const char* name, OP_Operator* op);
  static PRM_Template    myTemplateList[];
  static const UT_StringHolder theSOPTypeName;
  const SOP_NodeVerb* cookVerb() const override;
  static PRM_Template* buildTemplates();

protected:
  SOP_Clipper(OP_Network* net, const char* name, OP_Operator* op);
  ~SOP_Clipper() override;
  OP_ERROR cookMySop(OP_Context& context) override;
};

void newSopOperator(OP_OperatorTable* table);