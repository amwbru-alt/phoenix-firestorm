/** Copyright (c) 2026 ForMe.World contributors. LGPL-2.1-or-later. */
#include "llviewerprecompiledheaders.h"
#include "fsfloaterworldphysics.h"
#include "fsworldentities.h"
#include "formeworldphysics.h"
#include "fscreatoraccess.h"
#include "llselectmgr.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llcheckboxctrl.h"
#include "llspinctrl.h"
#include "lltextbox.h"
#include "llviewerobject.h"
#include "rlvhandler.h"

bool FSFloaterWorldPhysics::postBuild()
{
    getChild<LLButton>("refresh")->setClickedCallback([this](LLUICtrl*,const LLSD&) { refresh(); });
    getChild<LLButton>("apply")->setClickedCallback([this](LLUICtrl*,const LLSD&) { apply(); });
    getChild<LLComboBox>("preset")->setCommitCallback([this](LLUICtrl*,const LLSD&) { preset(); });
    return true;
}
void FSFloaterWorldPhysics::onOpen(const LLSD& key) { refresh(); }
void FSFloaterWorldPhysics::controls(bool enabled)
{
    for (const char* field : {"density", "friction", "restitution", "gravity"}) getChild<LLSpinCtrl>(field)->setEnabled(enabled);
    for (const char* field : {"mode", "shape", "preset"}) getChild<LLComboBox>(field)->setEnabled(enabled);
    getChild<LLCheckBoxCtrl>("collidable")->setEnabled(enabled);
    getChild<LLButton>("apply")->setEnabled(enabled);
}
void FSFloaterWorldPhysics::populate(const LLSD& entity)
{
    const LLSD& profile = entity["components"]["custom.rigid_profile"]["data"];
    mEntity = entity;
    controls(profile.isMap() && ForMeCreatorAccess::allowed());
    if (!profile.isMap()) { getChild<LLTextBox>("status")->setText(getString("await_entity")); return; }
    getChild<LLTextBox>("entity_id")->setText(entity["entityId"].asString());
    getChild<LLTextBox>("revision")->setText(entity["revision"].asString());
    getChild<LLComboBox>("mode")->setValue(profile["mode"]);
    getChild<LLComboBox>("shape")->setValue(profile["shape"]);
    getChild<LLComboBox>("preset")->setValue("custom");
    getChild<LLSpinCtrl>("density")->setValue(profile["density"]);
    getChild<LLSpinCtrl>("friction")->setValue(profile["friction"]);
    getChild<LLSpinCtrl>("restitution")->setValue(profile["restitution"]);
    getChild<LLSpinCtrl>("gravity")->setValue(profile["gravityModifier"]);
    getChild<LLCheckBoxCtrl>("collidable")->setValue(!profile["phantom"].asBoolean());
}
void FSFloaterWorldPhysics::refresh()
{
    if (mPending) return;
    controls(false); mEntity = LLSD(); mTarget.setNull();
    getChild<LLTextBox>("entity_id")->setText(std::string()); getChild<LLTextBox>("revision")->setText(std::string());
    if (!ForMeCreatorAccess::allowed() || gRlvHandler.hasBehaviour(RLV_BHVR_EDIT)) { closeFloater(); return; }
    LLViewerObject* selected = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
    LLViewerObject* object = selected ? selected->getRootEdit() : nullptr;
    if (!object || object->isAvatar() || object->isAttachment() || !object->permModify())
    { getChild<LLTextBox>("status")->setText(getString("select_object")); return; }
    mTarget = object->getID(); mPending = true;
    getChild<LLTextBox>("status")->setText(getString("loading"));
    LLHandle<LLFloater> handle = getHandle(); LLUUID target = mTarget;
    ForMeWorldEntities::readPhysicalObject(target, [handle,target](bool success,const LLSD& entity) {
        auto* self = dynamic_cast<FSFloaterWorldPhysics*>(handle.get()); if (!self || self->mTarget != target) return;
        self->mPending = false;
        if (success) self->populate(entity);
        self->getChild<LLTextBox>("status")->setText(self->getString(success ? "ready" : "failed"));
    });
}
void FSFloaterWorldPhysics::preset()
{
    const std::string id = getChild<LLComboBox>("preset")->getValue().asString();
    const auto* material=ForMeWorldPhysics::preset(id);if(!material)return;
    getChild<LLSpinCtrl>("density")->setValue(material->density);getChild<LLSpinCtrl>("friction")->setValue(material->friction);getChild<LLSpinCtrl>("restitution")->setValue(material->restitution);
}
void FSFloaterWorldPhysics::apply()
{
    if (mPending || mTarget.isNull() || !mEntity.isMap() || !ForMeCreatorAccess::allowed() || gRlvHandler.hasBehaviour(RLV_BHVR_EDIT)) return;
    LLViewerObject* selected = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
    LLViewerObject* object = selected ? selected->getRootEdit() : nullptr;
    if (!object || object->getID() != mTarget || !object->permModify()) { refresh(); return; }
    LLSD profile;
    profile["mode"] = getChild<LLComboBox>("mode")->getValue();
    profile["shape"] = getChild<LLComboBox>("shape")->getValue();
    profile["density"] = getChild<LLSpinCtrl>("density")->getValue().asReal();
    profile["friction"] = getChild<LLSpinCtrl>("friction")->getValue().asReal();
    profile["restitution"] = getChild<LLSpinCtrl>("restitution")->getValue().asReal();
    profile["gravityModifier"] = getChild<LLSpinCtrl>("gravity")->getValue().asReal();
    profile["phantom"] = !getChild<LLCheckBoxCtrl>("collidable")->getValue().asBoolean();
    const std::string mode=profile["mode"].asString(),shape=profile["shape"].asString();
    const ForMeWorldPhysics::Profile validated{mode,shape,profile["density"].asReal(),profile["friction"].asReal(),profile["restitution"].asReal(),profile["gravityModifier"].asReal(),profile["phantom"].asBoolean()};
    if(!validated.valid()){getChild<LLTextBox>("status")->setText(getString("failed"));return;}
    mPending = true; controls(false);getChild<LLTextBox>("status")->setText(getString("saving"));
    LLHandle<LLFloater> handle = getHandle(); LLUUID target = mTarget;
    ForMeWorldEntities::setPhysicsProfile(target,mEntity["revision"].asString(),profile,[handle,target](bool success,const LLSD& response) {
        auto* self = dynamic_cast<FSFloaterWorldPhysics*>(handle.get()); if (!self || self->mTarget != target) return;
        self->mPending = false;
        if (success) self->populate(response);
        const std::string code = response["code"].asString();
        const char* message = success ? "saved" : code == "revision_conflict" ? "conflict" : code == "physics_limits" ? "limits"
            : code == "physics_backend" || code == "physics_unavailable" || code == "physics_rejected" ? "unsupported"
            : code == "temporary_object" ? "temporary" : code == "volume_detect" ? "volume" : "failed";
        self->getChild<LLTextBox>("status")->setText(self->getString(message));
    });
}
