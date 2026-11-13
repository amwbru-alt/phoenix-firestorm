/** Copyright (C) 2026, ForMe.World contributors.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "llviewerprecompiledheaders.h"
#include "fsmarketbrowser.h"
#include "formemarketpolicy.h"
#include "llavatariconctrl.h"
#include "fsavatarbadges.h"
#include "formegridpolicy.h"
#include "llagent.h"
#include "llbutton.h"
#include "llcoros.h"
#include "llcorehttputil.h"
#include "llfloaterreg.h"
#include "llstatusbar.h"
#include "lliconctrl.h"
#include "llnotificationsutil.h"
#include "llnotificationptr.h"
#include "llsearcheditor.h"
#include "llscrollcontainer.h"
#include "lluictrlfactory.h"
#include "lltextbox.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llviewertexture.h"
#include "llweb.h"
#include "llstartup.h"
#include "llsdutil.h"

namespace
{
std::string capability()
{
    const std::string url = gAgent.getRegionCapability("ForMeMarketplace");
    return ForMeGrid::allowedServiceURI(url) ? url : std::string();
}
LLSD state()
{
    if (LLStartUp::getStartupState() != STATE_STARTED || gAgent.getID().isNull())
    {
        LLSD empty; empty["items"] = LLSD::emptyArray(); return empty;
    }
    LLSD data = gSavedPerAccountSettings.getLLSD("ForMeNativeMarket");
    if (data["owner"].asUUID() != gAgent.getID())
    {
        data = LLSD::emptyMap(); data["owner"] = gAgent.getID();
    }
    if (!data["items"].isArray()) data["items"] = LLSD::emptyArray();
    return data;
}
void save(LLSD data) { data["owner"] = gAgent.getID(); gSavedPerAccountSettings.setLLSD("ForMeNativeMarket", data); }
std::string money(S64 value, const std::string& currency) { return std::to_string(value) + (currency.empty() ? "" : " " + currency); }
LLSD itemKey(const std::string& id) { LLSD key; key["item_id"] = id; return key; }
LLSD identifiers(const LLSD& items)
{
    LLSD ids = LLSD::emptyArray();
    for (const auto& item : llsd::inArray(items)) ids.append(itemKey(item["item_id"].asString()));
    return ids;
}
void removeItem(const std::string& id)
{
    LLSD data = state(), items = LLSD::emptyArray();
    for (const auto& item : llsd::inArray(data["items"])) if (item["item_id"].asString() != id) items.append(item);
    data["items"] = items; save(data);
}
}

bool FSMarketBrowser::postBuild()
{
    mMode = hasChild("catalog_grid") ? "catalog" : hasChild("cart_rows") ? "cart" : "item";
    getChild<LLButton>("refresh")->setClickedCallback([this](LLUICtrl*, const LLSD&) { refresh(); });
    if (auto* site = findChild<LLButton>("website")) site->setClickedCallback([](LLUICtrl*, const LLSD&) {
        LLWeb::loadURLExternal(std::string(ForMeGrid::LoginURI) + "web/marketplace.html");
    });
    if (mMode == "catalog")
    {
        getChild<LLSearchEditor>("query")->setCommitCallback([this](LLUICtrl*, const LLSD&) { mPage = 0; refresh(); });
        for (const auto& category : {std::make_pair("all", ""), std::make_pair("clothes", "apparel"), std::make_pair("home", "objects/home"), std::make_pair("accessories", "body/accessories")})
            getChild<LLButton>(category.first)->setClickedCallback([this, value = std::string(category.second)](LLUICtrl*, const LLSD&) { mCategory = value; mPage = 0; refresh(); });
        getChild<LLButton>("previous")->setClickedCallback([this](LLUICtrl*, const LLSD&) { if (!mBusy && mPage > 0) { --mPage; refresh(); } });
        getChild<LLButton>("next")->setClickedCallback([this](LLUICtrl*, const LLSD&) { if (!mBusy && (mPage + 1) * 3 < mTotal) { ++mPage; refresh(); } });
        getChild<LLButton>("cart")->setClickedCallback([](LLUICtrl*, const LLSD&) { LLFloaterReg::showInstance("market_cart"); });
        for (S32 i = 0; i < 3; ++i)
        {
            const auto suffix = std::to_string(i);
            getChild<LLButton>("open_" + suffix)->setClickedCallback([this, i](LLUICtrl*, const LLSD&) { if (i < mItems.size()) LLFloaterReg::showInstance("market_product", itemKey(mItems[i]["item_id"].asString())); });
            getChild<LLButton>("add_" + suffix)->setClickedCallback([this, i](LLUICtrl*, const LLSD&) { if (i < mItems.size()) add(mItems[i]); });
            getChild<LLButton>("favorite_" + suffix)->setClickedCallback([this, i](LLUICtrl*, const LLSD&) {
                if (i >= mItems.size()) return;
                LLSD data = state(); const auto id = mItems[i]["item_id"].asString();
                data["favorites"][id] = !data["favorites"][id].asBoolean(); save(data); render();
            });
        }
    }
    else if (mMode == "item")
    {
        getChild<LLButton>("add")->setClickedCallback([this](LLUICtrl*, const LLSD&) { add(mProduct); });
        getChild<LLButton>("buy")->setClickedCallback([this](LLUICtrl*, const LLSD&) { LLSD items = LLSD::emptyArray(); if (mProduct.has("item_id")) { items.append(mProduct); checkout(items); } });
        for (S32 i = 0; i < 3; ++i) getChild<LLButton>("photo_pick_" + std::to_string(i))->setClickedCallback([this, i](LLUICtrl*, const LLSD&) {
            if (i < mProduct["images"].size()) image(getChild<LLIconCtrl>("photo"), mProduct["images"][i].asString());
        });
    }
    else getChild<LLButton>("checkout")->setClickedCallback([this](LLUICtrl*, const LLSD&) { checkout(state()["items"]); });
    return true;
}
void FSMarketBrowser::onOpen(const LLSD& key)
{
    mItemId = key["item_id"].asString(); mAgent = gAgent.getID(); mSession = gAgent.getSessionID(); mCap = capability();
    ++mRevision; mBusy = false; mProduct = LLSD(); mItems = LLSD::emptyArray(); refresh();
}
void FSMarketBrowser::status(const std::string& message)
{
    auto* label = getChild<LLTextBox>("status");
    label->setText(message);
    if (mMode == "catalog")
    {
        const bool empty = !mItems.size();
        label->setShape(empty ? LLRect(36, 180, 644, 108) : LLRect(16, 68, 664, 44));
    }
}
void FSMarketBrowser::draw()
{
    if (mAgent != gAgent.getID() || mSession != gAgent.getSessionID() || mCap != capability())
    {
        ++mRevision; mBusy = false; mProduct = LLSD(); mItems = LLSD::emptyArray();
        mAgent = gAgent.getID(); mSession = gAgent.getSessionID(); mCap = capability();
        render(); status("Сессия или регион изменились. Нажмите «Обновить».");
    }
    if (mMode == "cart" && !llsd_equals(mRenderedCart, state()["items"])) render();
    if (mMode == "item")
    {
        const auto id = mProduct["seller_id"].asUUID();
        const auto badge = id.notNull() && RlvActions::canShowName(RlvActions::SNC_DEFAULT, id) ? ForMeAvatarBadges::get(id) : ForMeBadge::Data();
        auto* label = getChild<LLTextBox>("seller_badge"); label->setVisible(badge.valid);
        label->setText(badge.valid ? badge.label : std::string());
        if (badge.valid) label->setColor(ForMeAvatarBadges::color(badge.badgeColor));
    }
    if (auto* cart = findChild<LLButton>("cart")) cart->setLabel("Корзина (" + std::to_string(state()["items"].size()) + ")");
    if (auto* pay = findChild<LLButton>("checkout")) pay->setEnabled(!mBusy && !mCap.empty() && (state()["items"].size() || state()["pending"].isMap()));
    getChild<LLButton>("refresh")->setEnabled(!mBusy);
    LLFloater::draw();
}
void FSMarketBrowser::request(LLSD data, std::function<void(FSMarketBrowser*, const LLSD&)> done)
{
    if (mBusy) return;
    if (LLStartUp::getStartupState() != STATE_STARTED || mAgent != gAgent.getID() || mSession != gAgent.getSessionID() || mCap != capability()) { status("Сессия изменилась. Откройте маркет заново."); return; }
    if (mCap.empty()) { status("Нативный маркет ещё не включён на сервере. Доступен сайт."); return; }
    mBusy = true; status("Загрузка…");
    const auto agent = mAgent, session = mSession; const auto cap = mCap; const auto revision = ++mRevision; const auto handle = getHandle();
    LLCoros::instance().launch("ForMeMarket", [handle, agent, session, cap, revision, data, done]() {
        LLSD result;
        try {
            auto adapter = std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("ForMeMarket", LLCore::HttpRequest::DEFAULT_POLICY_ID);
            auto http = std::make_shared<LLCore::HttpRequest>();
            auto options = std::make_shared<LLCore::HttpOptions>(); options->setTimeout(30);
            result = adapter->postAndSuspend(http, cap, data, options);
        } catch (...) { result = LLSD(); }
        auto* view = dynamic_cast<FSMarketBrowser*>(handle.get());
        if (!view || view->mRevision != revision || agent != gAgent.getID() || session != gAgent.getSessionID() || cap != capability()) return;
        view->mBusy = false;
        const auto status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS]);
        if (!status) {
            result["ok"] = false;
            if (result["message"].asString().empty()) result["message"] = result["error"].asString().empty()
                ? data["action"].asString() == "checkout"
                    ? "Не удалось получить ответ. Незавершённый заказ сохранён для проверки."
                    : "Не удалось загрузить данные маркета. Нажмите «Обновить» или откройте сайт."
                : result["error"].asString();
        }
        view->status(result["message"].asString()); done(view, result);
    });
}
void FSMarketBrowser::refresh()
{
    if (mBusy) return;
    render();
    if (mMode == "cart") return;
    LLSD data; data["action"] = mMode == "item" ? "item" : "catalog";
    data["item_id"] = mItemId; data["category"] = mCategory; data["offset"] = std::to_string(mPage * 3);
    if (mMode == "catalog") data["query"] = getChild<LLSearchEditor>("query")->getValue().asString();
    request(data, [](FSMarketBrowser* view, const LLSD& result) {
        if (result["ok"].asBoolean()) {
            if (view->mMode == "item") view->mProduct = result["item"];
            else { view->mItems = result["items"]; view->mTotal = result["count"].asInteger(); }
            view->render();
            view->status(!result["enabled"].asBoolean() ? "Маркет отключён на сервере." : view->mMode == "catalog" && !view->mItems.size() ? "Ничего не найдено." : "");
        }
    });
}
void FSMarketBrowser::image(LLIconCtrl* target, const std::string& value)
{
    target->setValue("Forme_UI_Inventory");
    const std::string url = ForMeMarket::photoURL(value, ForMeGrid::LoginURI);
    if (url.empty()) return;
    auto* texture = LLViewerTextureManager::getFetchedTextureFromUrl(url, FTT_DEFAULT, false, LLGLTexture::BOOST_PREVIEW);
    texture->addTextureStats(512.f * 512.f);
    target->setImage(new LLUIImage("market_photo", texture));
}
void FSMarketBrowser::add(const LLSD& item)
{
    if (!item.has("item_id") || mAgent != gAgent.getID() || mSession != gAgent.getSessionID() || mCap != capability()) return;
    LLSD data = state();
    for (const auto& row : llsd::inArray(data["items"])) if (row["item_id"] == item["item_id"]) { status("Товар уже в корзине."); return; }
    if (data["items"].size() >= 100) { status("В корзине уже 100 товаров."); return; }
    data["items"].append(item); save(data); status("Товар добавлен в вашу корзину.");
}
void FSMarketBrowser::render()
{
    if (mMode == "catalog")
    {
        for (S32 i = 0; i < 3; ++i)
        {
            const auto s = std::to_string(i); const bool present = i < mItems.size();
            getChild<LLPanel>("card_" + s)->setVisible(present);
            if (!present) continue;
            const LLSD& item = mItems[i]; image(getChild<LLIconCtrl>("photo_" + s), item["image"].asString());
            getChild<LLTextBox>("name_" + s)->setText(item["name"].asString());
            getChild<LLTextBox>("seller_" + s)->setText(item["creator"].asString());
            getChild<LLAvatarIconCtrl>("seller_avatar_" + s)->setValue(LLSD(item["seller_id"].asUUID()));
            getChild<LLTextBox>("price_" + s)->setText(money(item["price"].asInteger(), item["currency"].asString()));
            getChild<LLButton>("favorite_" + s)->setToggleState(state()["favorites"][item["item_id"].asString()].asBoolean());
        }
        getChild<LLTextBox>("page")->setText(std::to_string(mPage + 1) + " / " + std::to_string(llmax(1, (mTotal + 2) / 3)));
        getChild<LLTextBox>("count")->setText(std::to_string(mTotal) + " товаров");
        getChild<LLButton>("previous")->setEnabled(!mBusy && mPage > 0);
        getChild<LLButton>("next")->setEnabled(!mBusy && (mPage + 1) * 3 < mTotal);
    }
    else if (mMode == "item")
    {
        getChild<LLTextBox>("name")->setText(mProduct["name"].asString());
        getChild<LLTextBox>("seller")->setText(mProduct["creator"].asString());
        getChild<LLAvatarIconCtrl>("seller_avatar")->setValue(LLSD(mProduct["seller_id"].asUUID()));
        getChild<LLTextBox>("description")->setText(mProduct["description"].asString());
        getChild<LLTextBox>("price")->setText(mProduct.has("price") ? money(mProduct["price"].asInteger(), mProduct["currency"].asString()) : "");
        getChild<LLTextBox>("permissions")->setText(mProduct["permissions"].asString());
        image(getChild<LLIconCtrl>("photo"), mProduct["images"].size() ? mProduct["images"][0].asString() : mProduct["image"].asString());
        for (S32 i = 0; i < 3; ++i) {
            const auto n = std::to_string(i); const bool present = i < mProduct["images"].size();
            getChild<LLIconCtrl>("photo_small_" + n)->setVisible(present); getChild<LLButton>("photo_pick_" + n)->setVisible(present);
            if (present) image(getChild<LLIconCtrl>("photo_small_" + n), mProduct["images"][i].asString());
        }
        const bool available = mProduct.has("item_id") && mProduct["deliverable"].asBoolean() && (!mProduct.has("available") || mProduct["available"].asInteger() != 0);
        getChild<LLButton>("buy")->setEnabled(!mBusy && available); getChild<LLButton>("add")->setEnabled(available);
    }
    else
    {
        auto* body = getChild<LLPanel>("cart_rows"); body->deleteAllChildren();
        LLSD items = state()["items"]; mRenderedCart = items; const S32 height = llmax(84, items.size() * 84); body->reshape(684, height); body->setOrigin(0, 0);
        S64 total = 0; std::string currency; bool mixed_currency = false; S32 i = 0; const auto handle = getHandle();
        for (const auto& item : llsd::inArray(items)) {
            const S32 top = height - i++ * 84; const auto id = item["item_id"].asString();
            LLIconCtrl::Params p; p.name = "photo_" + id; p.rect = LLRect(8, top - 8, 72, top - 72);
            auto* photo = LLUICtrlFactory::create<LLIconCtrl>(p); body->addChild(photo); image(photo, item["image"].asString());
            for (const auto& line : {std::make_pair("name", 12), std::make_pair("creator", 40)}) {
                LLTextBox::Params t; t.name = line.first + id; t.rect = LLRect(88, top - line.second, 480, top - line.second - 24); t.initial_value = item[line.first].asString(); t.use_ellipses = true;
                body->addChild(LLUICtrlFactory::create<LLTextBox>(t));
            }
            LLTextBox::Params cost; cost.name = "cost_" + id; cost.rect = LLRect(492, top - 28, 616, top - 52); cost.initial_value = money(item["price"].asInteger(), item["currency"].asString()); body->addChild(LLUICtrlFactory::create<LLTextBox>(cost));
            LLButton::Params remove; remove.name = "remove_" + id; remove.label = "×"; remove.rect = LLRect(636, top - 24, 672, top - 56);
            auto* button = LLUICtrlFactory::create<LLButton>(remove); body->addChild(button); button->setClickedCallback([handle, id](LLUICtrl*, const LLSD&) { removeItem(id); if (auto* view = dynamic_cast<FSMarketBrowser*>(handle.get())) view->render(); });
            if (i == 1) currency = item["currency"].asString();
            else if (currency != item["currency"].asString()) mixed_currency = true;
            total += item["price"].asInteger();
        }
        getChild<LLScrollContainer>("cart_scroll")->goToTop();
        getChild<LLTextBox>("total")->setText(mixed_currency ? std::string("Итог рассчитает сервер") : std::string("Предварительно: ") + money(total, currency));
        status(items.size() ? "Итоговую цену и скидки подтвердит сервер перед оплатой." : "Ваша корзина пуста.");
    }
}
void FSMarketBrowser::checkout(const LLSD& items)
{
    if (mBusy) return;
    LLSD pending = state()["pending"];
    if (pending.isMap()) {
        const auto handle = getHandle(); const auto agent = mAgent, session = mSession; const auto cap = mCap;
        LLNotificationsUtil::add("ForMeMarketRecover", LLSD(), LLSD(), [handle, agent, session, cap, pending](const LLSD& n, const LLSD& response) {
            auto* view = dynamic_cast<FSMarketBrowser*>(handle.get());
            if (view && agent == gAgent.getID() && session == gAgent.getSessionID() && cap == capability() && LLNotificationsUtil::getSelectedOption(n, response) == 0) view->submit(pending);
        }); return;
    }
    if (!items.size()) return;
    LLSD data; data["action"] = "quote"; data["items"] = identifiers(items);
    request(data, [items](FSMarketBrowser* view, const LLSD& quote) {
        if (!quote["ok"].asBoolean() || quote["fingerprint"].asString().empty() || !quote["total"].isInteger() || quote["total"].asInteger() < 0) return;
        LLSD args; args["PRICE"] = quote["total"].asString() + " " + quote["currency"].asString();
        const auto handle = view->getHandle(); const auto agent = view->mAgent, session = view->mSession; const auto cap = view->mCap; const auto revision = view->mRevision;
        LLNotificationsUtil::add("ForMeMarketConfirm", args, LLSD(), [handle, agent, session, cap, revision, items, quote](const LLSD& n, const LLSD& response) {
            auto* current = dynamic_cast<FSMarketBrowser*>(handle.get());
            if (!current || current->mBusy || current->mRevision != revision || agent != gAgent.getID() || session != gAgent.getSessionID() || cap != capability() || LLNotificationsUtil::getSelectedOption(n, response) != 0) return;
            LLSD state_data = state(); if (state_data["pending"].isMap()) return;
            LLSD order; order["action"] = "checkout"; order["items"] = identifiers(items); order["request_id"] = LLUUID::generateNewID().asString(); order["quote_fingerprint"] = quote["fingerprint"];
            state_data["pending"] = order; save(state_data); current->submit(order);
        });
    });
}
void FSMarketBrowser::submit(const LLSD& order)
{
    if (mBusy || order["request_id"].asString().empty() || !order["items"].isArray() || !order["items"].size() || state()["pending"]["request_id"] != order["request_id"]) return;
    // Persist the same receipt before any side effect, including after a retry
    // from another window. Inability to save recovery state blocks payment.
    const auto file = gSavedSettings.getString("PerAccountSettingsFile");
    if (file.empty() || !gSavedPerAccountSettings.saveToFile(file, true))
    {
        status("Не удалось сохранить идентификатор заказа. Оплата не отправлена."); return;
    }
    request(order, [order](FSMarketBrowser* view, const LLSD& result) {
        LLSD data = state();
        const S32 code = result["http_status"].asInteger();
        if (ForMeMarket::releaseReceipt(result["ok"].asBoolean(), code, result["code"].asString())) { data.erase("pending"); save(data); }
        if (result["ok"].asBoolean())
        {
            for (const auto& item : llsd::inArray(order["items"])) removeItem(item["item_id"].asString());
            LLStatusBar::sendMoneyBalanceRequest();
        }
        view->render(); view->status(result["ok"].asBoolean() ? "Покупка завершена. Товары доставлены в инвентарь." : result["message"].asString());
    });
}
