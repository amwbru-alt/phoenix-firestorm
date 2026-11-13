/** Copyright (c) 2026 ForMe.World contributors. LGPL-2.1-or-later. */
#include "llviewerprecompiledheaders.h"
#include "fsfloaterworldphysics.h"
#include "fsworldentities.h"
#include "fscreatoraccess.h"
#include "llselectmgr.h"
#include "llbutton.h"
#include "llspinctrl.h"
#include "lltextbox.h"
#include "llviewerobject.h"
#include "rlvhandler.h"

bool FSFloaterWorldPhysics::postBuild()
{
    getChild<LLButton>("refresh")->setClickedCallback([this](LLUICtrl*,const LLSD&) { refresh(); });
    getChild<LLButton>("apply")->setClickedCallback([this](LLUICtrl*,const LLSD&) { apply(); });
    return true;
}
void FSFloaterWorldPhysics::onOpen(const LLSD& key) { refresh(); }
void FSFloaterWorldPhysics::refresh()
{
    if (mPending) return;
    getChild<LLButton>("apply")->setEnabled(false);
    mTarget.setNull();
    getChild<LLTextBox>("entity_id")->setText(std::string()); getChild<LLTextBox>("revision")->setText(std::string());
    for (const char* field : {"density", "friction", "restitution"}) getChild<LLSpinCtrl>(field)->setEnabled(false);
    if (!ForMeCreatorAccess::allowed() || gRlvHandler.hasBehaviour(RLV_BHVR_EDIT)) { closeFloater(); return; }
    LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
    if (!object || object->isAvatar() || object->isAttachment() || !object->permModify())
    { getChild<LLTextBox>("status")->setText(getString("select_object")); return; }
    LLSD entity = ForMeWorldEntities::entity(object->getID());
    const LLSD& material = entity["components"]["physical_material"];
    if (!entity.isMap() || material["schemaVersion"].asInteger() != 1)
    { getChild<LLTextBox>("status")->setText(getString("await_entity")); return; }
    mTarget = object->getID();
    for (const char* field : {"density", "friction", "restitution"}) getChild<LLSpinCtrl>(field)->setEnabled(true);
    getChild<LLTextBox>("entity_id")->setText(mTarget.asString());
    getChild<LLTextBox>("revision")->setText(entity["revision"].asString());
    getChild<LLSpinCtrl>("density")->setValue(material["data"]["density"]);
    getChild<LLSpinCtrl>("friction")->setValue(material["data"]["friction"]);
    getChild<LLSpinCtrl>("restitution")->setValue(material["data"]["restitution"]);
    getChild<LLTextBox>("status")->setText(getString("ready"));
    getChild<LLButton>("apply")->setEnabled(true);
}
void FSFloaterWorldPhysics::apply()
{
    if (mPending || mTarget.isNull() || !ForMeCreatorAccess::allowed() || gRlvHandler.hasBehaviour(RLV_BHVR_EDIT)) return;
    LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
    if (!object || object->getID() != mTarget || !object->permModify()) { refresh(); return; }
    LLSD material = ForMeWorldEntities::entity(mTarget)["components"]["physical_material"]["data"];
    if (!material.isMap()) { refresh(); return; }
    material["density"] = getChild<LLSpinCtrl>("density")->getValue().asReal();
    material["friction"] = getChild<LLSpinCtrl>("friction")->getValue().asReal();
    material["restitution"] = getChild<LLSpinCtrl>("restitution")->getValue().asReal();
    mPending = true; getChild<LLButton>("apply")->setEnabled(false);getChild<LLTextBox>("status")->setText(getString("saving"));
    LLHandle<LLFloater> handle = getHandle();
    ForMeWorldEntities::setMaterial(mTarget, material, [handle](bool success,const std::string& code) {
        auto* self = dynamic_cast<FSFloaterWorldPhysics*>(handle.get()); if (!self) return;
        self->mPending = false;
        self->getChild<LLTextBox>("status")->setText(self->getString(success ? "saved" : code == "revision_conflict" ? "conflict" : "failed"));
        // Refresh explicitly after the next replica poll so unsaved edits are never overwritten in the background.
    });
}
