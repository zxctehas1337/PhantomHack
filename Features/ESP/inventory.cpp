#include "../../Precompiled.h"

void InvalidateGloveMaterial(C_CSGOViewModel* pViewModel)
{
    MaterialInfo_t* pMaterialInfo = pViewModel->m_MaterialInfo();
    for (std::uint32_t i = 0; i < pMaterialInfo->m_nCount; i++)
    {
        if (pMaterialInfo->m_pMaterialRecords[i].m_eIdentifier == MATERIAL_MAGIC_NUMBER_GLOVE)
        {
            pMaterialInfo->m_pMaterialRecords[i].m_uTypeIndex = 0xffffffff;
            break;
        }
    }
}

void InventoryChanger::SetGlove(CCSPlayerController* pController, C_CSPlayerPawn* pPawn, CCSPlayerInventory* pInventory)
{
    C_EconItemView* pGloveItemView = pPawn->m_EconGloves();
    if (!pGloveItemView)
        return;

    C_EconItemView* pItemViewLoadout = pInventory->GetItemInLoadout(pController->m_iTeamNum(), LOADOUT_SLOT_CLOTHING_HANDS);
    if (!pItemViewLoadout)
        return;

    CEconItemSchema* pEconItemSchema = Interfaces::m_pClient->GetEconItemSystem()->GetEconItemSchema();
    if (!pEconItemSchema)
        return;

    using CreateNewPaintKitfn = CPaintKit * (__fastcall*)(C_EconItemView*);
    static CreateNewPaintKitfn CreateNewPaintKit = reinterpret_cast<CreateNewPaintKitfn>(Memory::FindPattern(CLIENT_DLL, X("48 89 5C 24 10 56 48 83 EC 20 48 8B 01 FF 50 10 48 8B 1D ? ? ? ?")));

    using SetBodyGroupfn = void* (__fastcall*)(void*, int, int);
    static SetBodyGroupfn SetBodyGroup = reinterpret_cast<SetBodyGroupfn>(Memory::FindPattern(CLIENT_DLL, X("85 D2 0F 88 ? ? ? ? 53 55")));

    if (pGloveItemView->m_iItemID() != pItemViewLoadout->m_iItemID() || m_bNewRound || pPawn->m_flLastSpawnTimeIndex() != m_fllastSpawnTimeIndex)
    {
        pGloveItemView->m_iItemDefinitionIndex() = pItemViewLoadout->m_iItemDefinitionIndex();
        pGloveItemView->m_iItemID() = pItemViewLoadout->m_iItemID();
        pGloveItemView->m_iItemIDHigh() = pItemViewLoadout->m_iItemIDHigh();
        pGloveItemView->m_iItemIDLow() = pItemViewLoadout->m_iItemIDLow();
        pGloveItemView->m_iAccountID() = pItemViewLoadout->m_iAccountID();

        m_bNewRound = false;
        m_fllastSpawnTimeIndex = pPawn->m_flLastSpawnTimeIndex();

        CPaintKit* pPaintKit = CreateNewPaintKit(pItemViewLoadout);
        if (pPaintKit)
        {
            auto iCurrentPaintKit = pEconItemSchema->GetPaintKits().FindByKey(pGloveItemView->GetCustomPaintKitIndex());
            if (iCurrentPaintKit.has_value())
                pPaintKit->sName = iCurrentPaintKit.value()->sName;
        }

        SetBodyGroup(pPawn, 0, 1);
        pGloveItemView->m_bInitialized() = true;
        pPawn->m_bNeedToReApplyGloves() = true;
    }
}

void InventoryChanger::SetAgent(CCSPlayerController* pController, C_CSPlayerPawn* pPawn, CCSPlayerInventory* pInventory)
{
    if (!Interfaces::m_pEngine->IsInGame()) return;
    if (!Globals::m_pLocalPlayerController) return;
    if (!Globals::m_pLocalPlayerController->m_bPawnIsAlive()) return;

    C_EconItemView* pItemViewLoadout = pInventory->GetItemInLoadout(CCSPlayerController::GetLocalPlayerController()->m_iTeamNum(), LOADOUT_SLOT_CLOTHING_CUSTOMPLAYER);
    if (!pItemViewLoadout)
        return;

    CEconItemDefinition* pEconDefintion = pItemViewLoadout->GetStaticData();
    if (!pEconDefintion)
        return;

    CGameSceneNode* pGameSceneNode = pPawn->m_pGameSceneNode();
    if (!pGameSceneNode)
        return;

    CSkeletonInstance* pSkeletonInstance = pGameSceneNode->GetSkeletonInstance();
    if (!pSkeletonInstance)
        return;

    if (Config::b(g_Variables.m_Visuals.m_bCustomModelChanger))
        return;

    if (Globals::m_pLocalPlayerPawn)
        pPawn->SetModel(pEconDefintion->m_pszModelName());
}

void UpdateWeapon(C_BasePlayerWeapon* pWeapon)
{
    if (!pWeapon)
        return;

    void* pCompositeMaterial = reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(pWeapon) + 0x548);
    if (pCompositeMaterial)
    {
#ifdef CS_PARANOID
        CS_ASSERT(Functions::fnUpdateCompositeMaterial != nullptr);
#endif

#ifdef CS_PARANOID
        CS_ASSERT(Functions::fnRegenerateWeaponSkin != nullptr);
#endif

        ReturnAddressSpoofer::SpoofCall(ReturnAddressSpoofGadgets::m_pClientGadet, Functions::fnUpdateCompositeMaterial, pCompositeMaterial, 1);
        ReturnAddressSpoofer::SpoofCall(ReturnAddressSpoofGadgets::m_pClientGadet, Functions::fnRegenerateWeaponSkin, pWeapon, false);

        Memory::CallVFunc<void*, 7U>(pWeapon, 1);
        Memory::CallVFunc<void*, 100U>(pWeapon, 1);
    }

}

const bool IsValidWeapon(CCSWeaponBaseVData* pWeaponVData)
{
    switch (pWeaponVData->m_WeaponType())
    {
    case WEAPONTYPE_C4:
    case WEAPONTYPE_GRENADE:
    case WEAPONTYPE_EQUIPMENT:
    case WEAPONTYPE_STACKABLEITEM:
    case WEAPONTYPE_FISTS:
    case WEAPONTYPE_BREACHCHARGE:
    case WEAPONTYPE_BUMPMINE:
    case WEAPONTYPE_TABLET:
    case WEAPONTYPE_MELEE:
    case WEAPONTYPE_SHIELD:
    case WEAPONTYPE_ZONE_REPULSOR:
    case WEAPONTYPE_UNKNOWN:
        return false;
    default:
        return true;
    }
}

void InventoryChanger::Run()
{
    if (!Interfaces::m_pEngine->IsConnected() || !Interfaces::m_pEngine->IsInGame())
        return;

    CCSPlayerController* pLocalPlayerController = CCSPlayerController::GetLocalPlayerController();
    if (!pLocalPlayerController)
        return;

    C_CSPlayerPawn* pLocalPlayerPawn = pLocalPlayerController->m_hPlayerPawn().Get();
    if (!pLocalPlayerPawn)
        return;

    CCSPlayerInventory* pInventory = CCSPlayerInventory::GetPlayerInventory();
    if (!pInventory)
        return;

    const uint64_t steamID = pInventory->GetOwner().m_uID;
    auto* pC_CS2HudModelWeapon = pLocalPlayerPawn->GetViewModel();

    if (!pC_CS2HudModelWeapon)
        return;

    //// override glove
    //SetGlove(pLocalPlayerController, pLocalPlayerPawn, pInventory);
    //// agent changer
    //SetAgent(pLocalPlayerController, pLocalPlayerPawn, pInventory);

    InventoryChanger::SetCustomModel(pLocalPlayerController, pLocalPlayerPawn, pInventory);

    // weapon changer
    for (EntityObject_t& object : g_Entities->m_vecEntities)
    {
        if (object.m_eType != ENTITY_WEAPON)
            continue;

        C_CSWeaponBase* pWeapon = reinterpret_cast<C_CSWeaponBase*>(object.m_pEntity);
        if (!pWeapon || pWeapon->GetOriginalOwnerXuid() != steamID || !pWeapon->IsWeapon())
            continue;

        C_AttributeContainer* pAttributeContainer = pWeapon->m_AttributeManager();
        if (!pAttributeContainer)
            continue;

        C_EconItemView* pWeaponItemView = pAttributeContainer->m_Item();
        if (!pWeaponItemView)
            continue;

        CEconItemDefinition* pWeaponDefinition = pWeaponItemView->GetStaticData();
        if (!pWeaponDefinition)
            continue;

        CGameSceneNode* pWeaponSceneNode = pWeapon->m_pGameSceneNode();
        if (!pWeaponSceneNode)
            continue;

        // No idea how to check this faster with the new loadout system.
        C_EconItemView* pWeaponInLoadoutItemView = nullptr;
        if (pWeaponDefinition->IsWeapon())
        {
            for (int i = 0; i < LOADOUT_SLOT_COUNT; ++i)
            {
                C_EconItemView* pItemView = pInventory->GetItemInLoadout(pWeapon->m_iOriginalTeamNumber(), i);
                if (!pItemView)
                    continue;

                if (pItemView->m_iItemDefinitionIndex() == pWeaponDefinition->m_nDefIndex())
                {
                    pWeaponInLoadoutItemView = pItemView;
                    break;
                }
            }
        }
        else
            pWeaponInLoadoutItemView = pInventory->GetItemInLoadout(pWeapon->m_iOriginalTeamNumber(), pWeaponDefinition->LoadoutSlot());

        if (!pWeaponInLoadoutItemView)
            continue;

        bool is_knife = pWeaponDefinition->IsKnife(false);

        int current_weapon = get_skin_config(pWeaponItemView->m_iItemDefinitionIndex());
        if (!is_knife && current_weapon == WEAPONTYPE_UNKNOWN)
            continue;

        // Check if skin is added by us.
        std::vector<InventoryItem_t>::iterator it = std::find_if(m_vecAddedItems.begin(), m_vecAddedItems.end(), [pWeaponInLoadoutItemView](const InventoryItem_t& item) { return item.m_uItemID == pWeaponInLoadoutItemView->m_iItemID(); });
        if (it == m_vecAddedItems.end())
            continue;

        CEconItemDefinition* pWeaponInLoadoutDefinition = pWeaponInLoadoutItemView->GetStaticData();
        if (!pWeaponInLoadoutDefinition)
            continue;

        // Example: Will not equip FiveSeven skin on CZ. Not applies for knives.
        const bool bIsKnife = pWeaponInLoadoutDefinition->IsKnife(false);
        if (!bIsKnife && pWeaponInLoadoutDefinition->m_nDefIndex() != pWeaponDefinition->m_nDefIndex())
            continue;

        pWeaponItemView->m_iItemID() = pWeaponInLoadoutItemView->m_iItemID();
        pWeaponItemView->m_iItemIDHigh() = pWeaponInLoadoutItemView->m_iItemIDHigh();
        pWeaponItemView->m_iItemIDLow() = pWeaponInLoadoutItemView->m_iItemIDLow();
        pWeaponItemView->m_iAccountID() = uint32_t(steamID);
        pWeaponItemView->m_bDisallowSOC() = false;
        pWeaponItemView->m_bRestoreCustomMaterialAfterPrecache() = true;
        pWeaponItemView->m_iItemDefinitionIndex() = pWeaponInLoadoutItemView->m_iItemDefinitionIndex();

        // Displays nametag and stattrak on the gun.
        // Found by: https://www.unknowncheats.me/forum/members/2377851.html
       /* if (!pWeapon->m_bUIWeapon())
        {
            pWeapon->AddStattrakEntity();
            pWeapon->AddNametagEntity();
        }*/

        CBaseHandle hWeapon = pWeapon->GetRefEHandle();
        if (bIsKnife) {
            pWeaponItemView->m_iItemDefinitionIndex() = pWeaponInLoadoutDefinition->m_nDefIndex();

            //// set correct subclass ID
            //pWeapon->m_nSubclassID() = CUtlStringToken(std::to_string(pWeaponInLoadoutDefinition->m_uDefinitionIndex).c_str());
            //// update correct subclass
            //pWeapon->UpdateSubClass();

            //// set correct weapon vdata name
            //pWeaponVData->m_szName() = pWeaponInLoadoutDefinition->GetSimpleWeaponName();
            //// update weapon data
            //pWeaponVData->UpdateWeaponData();

            const char* szWantedModel = pWeaponInLoadoutDefinition->m_pszModelName();
            pWeapon->SetModel(szWantedModel);
        }
        else {
            // Use legacy weapon models only for skins that require them.
            // Probably need to cache this if you really care that much about
            // performance.
            std::optional<CPaintKit*> paintKit = Interfaces::m_pClient->GetEconItemSystem()->GetEconItemSchema()->GetPaintKits().FindByKey(pWeaponInLoadoutItemView->GetCustomPaintKitIndex());

            pWeaponSceneNode->SetMeshGroupMask(2);
            pC_CS2HudModelWeapon->m_pGameSceneNode()->SetMeshGroupMask(2);

            auto pCompositeMaterial = reinterpret_cast<void*>((PBYTE)pWeapon + 0x5F8);
            if (pCompositeMaterial)
                Functions::fnC_CSWeaponBase_UpdateCompositeMaterial(pCompositeMaterial, true);

            Functions::fnC_CSWeaponBase_UpdateSkin(pWeapon, true);
            pWeapon->PostDataUpdate();
        }
    }
}

static bool IsPaintKitForWeapon(CPaintKit* pPaintKit, const char* weapon_id)
{
    auto path = "panorama/images/econ/default_generated/" + std::string(weapon_id) + "_" + pPaintKit->sName + "_light_png.vtex_c";
    return Interfaces::m_pFileSystem->Exists(path.c_str(), "GAME");
}

bool InventoryChanger::DumpAllSkins()
{
    CEconItemSchema* pItemSchema = Interfaces::m_pClient->GetEconItemSystem()->GetEconItemSchema();
    if (!pItemSchema)
        return false;

    const CUtlMap<int, CEconItemDefinition*> vecItems = pItemSchema->GetSortedItemDefinitionMap();
    const CUtlMap<int, CPaintKit*> vecPaintKits = pItemSchema->GetPaintKits();

    for (const auto& it : vecItems)
    {
        CEconItemDefinition* pItem = it.m_Value;
        if (!pItem)
            continue;

        const bool bIsWeapon = pItem->IsWeapon();
        const bool bIsKnife = pItem->IsKnife(true);
        const bool bIsGloves = pItem->IsGlove(true);
        const bool bIsAgent = pItem->IsAgent(true);

        if (!bIsWeapon && !bIsKnife && !bIsGloves && !bIsAgent)
            continue;

        // Some items don't have names.
        const char* itemBaseName = pItem->m_pszItemBaseName();
        if (!itemBaseName || itemBaseName[0] == '\0')
            continue;

        if (bIsAgent)
        {
            std::string strModelname = Interfaces::m_pLocalize->FindSafe(itemBaseName);
            if (strModelname.starts_with(X("#CSGO_CustomPlayer")))
                continue;

            size_t pos = strModelname.find(X(" | "));
            if (pos != std::string_view::npos)
                strModelname = strModelname.substr(0, pos);

            DumpedAgent_t dumpedAgent;
            dumpedAgent.m_strName = strModelname;
            dumpedAgent.m_strModel = pItem->m_pszModelName();
            dumpedAgent.m_nRarity = pItem->m_nItemRarity();
            dumpedAgent.m_nDefinitionIndex = pItem->m_nDefIndex();
            m_vecDumpedAgents.emplace_back(dumpedAgent);
            continue;
        }

        const std::uint16_t uDefinitionIndex = pItem->m_nDefIndex();
        DumpedItem_t dumpedItem;
        dumpedItem.m_strName = Interfaces::m_pLocalize->FindSafe(itemBaseName);
        dumpedItem.m_uDefinitionIndex = uDefinitionIndex;
        dumpedItem.m_nRarity = pItem->m_nItemRarity();
        dumpedItem.m_eItemType = bIsKnife ? ITEM_TYPE_KNIFE : bIsGloves ? ITEM_TYPE_GLOVE : /*bIsSticker ?  ITEM_TYPE_STICKER :*/ ITEM_TYPE_WEAPON;

        if (bIsKnife || bIsGloves)
            dumpedItem.m_bUnusualItem = true;

        // Add vanilla knives
        if (bIsKnife)
            dumpedItem.m_vecDumpedSkins.emplace_back(X("Vanilla"), 0, IR_ANCIENT);

        // We filter skins by guns.
        for (const auto& it : vecPaintKits)
        {
            CPaintKit* pPaintKit = it.m_Value;
            if (!pPaintKit || pPaintKit->nID == 0 || pPaintKit->nID == 9001)
                continue;

            std::string weaponName = pItem->m_pszWeaponName();
            std::string imagePath = std::format("panorama/images/econ/default_generated/{}_{}_light_png.vtex_c",
                weaponName, pPaintKit->sName);

            if (Interfaces::m_pFileSystem->Exists(imagePath.c_str(), nullptr))
            {
                DumpedSkin_t dumpedSkin;
                dumpedSkin.m_strName = Interfaces::m_pLocalize->FindSafe(pPaintKit->sDescriptionTag);
                dumpedSkin.m_nID = static_cast<int>(pPaintKit->nID);
                dumpedSkin.m_nRarity = pPaintKit->nRarity;
                dumpedItem.m_vecDumpedSkins.emplace_back(dumpedSkin);
            }
        }

        // Sort skins by rarity.
        if (!dumpedItem.m_vecDumpedSkins.empty() && bIsWeapon)
            std::sort(dumpedItem.m_vecDumpedSkins.begin(), dumpedItem.m_vecDumpedSkins.end(), [](const DumpedSkin_t& a, const DumpedSkin_t& b) { return a.m_nRarity > b.m_nRarity; });

        m_vecDumpedItems.emplace_back(dumpedItem);
    }

    return !m_vecDumpedItems.empty();
}

void InventoryChanger::UpdateHUD()
{
    // thanks https://www.unknowncheats.me/forum/4279609-post5.html

    if (m_bWantsHUDUpdate)
    {
        auto pCCSGO_HudWeaponSelection = (uintptr_t)(Utilities::FindHudElement(X("HudWeaponSelection")));
        if (pCCSGO_HudWeaponSelection) {
            using fnCCSGO_HudWeaponSelection_ClearHudWeaponIcon = int(__fastcall*)(uintptr_t, int, int);
            auto CCSGO_HudWeaponSelection_ClearHudWeaponIcon = (fnCCSGO_HudWeaponSelection_ClearHudWeaponIcon)(Memory::FindPattern(CLIENT_DLL, X("4C 8B DC 55 57 48 83 EC ? 48 63 41")));

            CCSGO_HudWeaponSelection_ClearHudWeaponIcon((uintptr_t)pCCSGO_HudWeaponSelection - 0x98, 0, 0);
        }

        m_bWantsHUDUpdate = false;
    }
}

void InventoryChanger::OnEquipItemInLoadout(int nTeam, int nSlot, std::uint64_t uItemID)
{
    std::vector<InventoryItem_t>::iterator it = std::find_if(m_vecAddedItems.begin(), m_vecAddedItems.end(), [uItemID](const InventoryItem_t& item) { return item.m_uItemID == uItemID; });
    if (it == m_vecAddedItems.end())
        return;

    CCSInventoryManager* pInventoryManager = CCSInventoryManager::GetInventoryManager();
    if (!pInventoryManager)
        return;

    CCSPlayerInventory* pInventory = CCSPlayerInventory::GetPlayerInventory();
    if (!pInventory)
        return;

    C_EconItemView* pItemViewToEquip = pInventory->GetItemViewForItem(it->m_uItemID);
    if (!pItemViewToEquip)
        return;

    C_EconItemView* pItemInLoadout = pInventory->GetItemInLoadout(nTeam, nSlot);
    if (!pItemInLoadout)
        return;

    CEconItemDefinition* pItemInLoadoutStaticData = pItemInLoadout->GetStaticData();
    if (!pItemInLoadoutStaticData)
        return;

    if (pItemInLoadoutStaticData->IsGlove(false) ||
        pItemInLoadoutStaticData->IsKnife(false) ||
        pItemInLoadoutStaticData->m_nDefIndex() == pItemViewToEquip->m_iItemDefinitionIndex())
        return;

    // Equip default item. If you would have bought Deagle and you previously
    // had R8 equipped it will now give you a Deagle.
    const std::uint64_t uDefaultItemID = (std::uint64_t(0xF) << 60) | pItemViewToEquip->m_iItemDefinitionIndex();
    pInventoryManager->EquipItemInLoadout(nTeam, nSlot, uDefaultItemID);
    CEconItem* pItemInLoadoutSOCData = pItemInLoadout->GetSOCData();
    if (!pItemInLoadoutSOCData)
        return;

    // Mark old item as unequipped.
    pInventory->SOUpdated(pInventory->GetOwner(), reinterpret_cast<CSharedObject*>(pItemInLoadoutSOCData), eSOCacheEvent_Incremental);
}

void InventoryChanger::OnSetModel(C_BaseModelEntity* pEntity, const char*& szModel)
{
    // When you're lagging you may see the default knife for one second and this
    // function fixes that.
    if (!pEntity || !pEntity->IsViewModel())
        return;

    C_BaseViewModel* pViewModel = reinterpret_cast<C_BaseViewModel*>(pEntity);
    CCSPlayerInventory* pInventory = CCSPlayerInventory::GetPlayerInventory();
    if (!pInventory)
        return;

    const uint64_t uSteamID = pInventory->GetOwner().m_uID;
    C_BasePlayerWeapon* pWeapon = pViewModel->m_hWeapon().Get();
    if (!pWeapon || !pWeapon->IsWeapon() || pWeapon->GetOriginalOwnerXuid() != uSteamID)
        return;

    C_CSWeaponBase* pWeaponBase = reinterpret_cast<C_CSWeaponBase*>(pWeapon);
    if (!pWeaponBase)
        return;

    C_AttributeContainer* pAttributeContainer = pWeapon->m_AttributeManager();
    if (!pAttributeContainer)
        return;

    C_EconItemView* pWeaponItemView = pAttributeContainer->m_Item();
    if (!pWeaponItemView)
        return;

    CEconItemDefinition* pWeaponDefinition = pWeaponItemView->GetStaticData();
    if (!pWeaponDefinition)
        return;

    C_EconItemView* pWeaponInLoadoutItemView = pInventory->GetItemInLoadout(pWeaponBase->m_iOriginalTeamNumber(), pWeaponDefinition->LoadoutSlot());
    if (!pWeaponInLoadoutItemView)
        return;

    // Check if skin is added by us.
    std::vector<InventoryItem_t>::iterator it = std::find_if(m_vecAddedItems.begin(), m_vecAddedItems.end(), [pWeaponInLoadoutItemView](const InventoryItem_t& item) { return item.m_uItemID == pWeaponInLoadoutItemView->m_iItemID(); });
    if (it == m_vecAddedItems.end())
        return;

    CEconItemDefinition* pWeaponInLoadoutDefinition = pWeaponInLoadoutItemView->GetStaticData();
    if (!pWeaponInLoadoutDefinition || !pWeaponInLoadoutDefinition->IsKnife(true))
        return;

    szModel = pWeaponInLoadoutDefinition->m_pszModelName();
}

void InventoryChanger::AddEconItemToList(InventoryItem_t item)
{
    m_vecAddedItems.emplace_back(item);
}

void InventoryChanger::Restore()
{
    CCSPlayerInventory* pInventory = CCSPlayerInventory::GetPlayerInventory();
    if (!pInventory)
        return;

    for (InventoryItem_t& item : m_vecAddedItems)
        pInventory->RemoveEconItem(pInventory->GetSOCDataForItem(item.m_uItemID));
}

void InventoryChanger::OnFireEvent(CGameEvent* pEvent)
{
    FNV1A_t szEventName = FNV1A::Hash(pEvent->GetName());
    switch (szEventName)
    {
    case FNV1A::HashConst("round_start"):
        g_Misc->AutoPurchase();
        g_Chams->CleanupOldModels();
        m_bNewRound = true;
        break;

    case FNV1A::HashConst("spawn_player"):
        m_bNewRound = true;
        break;

    case FNV1A::HashConst("player_death"):
    {
        CCSPlayerController* pAttacker = reinterpret_cast<CCSPlayerController*>(pEvent->GetPlayerController(X("attacker")));
        C_BasePlayerPawn* pAttackerPawn = reinterpret_cast<C_CSPlayerPawn*>(pEvent->GetPlayerPawn(X("attacker")));

        CCSPlayerController* pVictim = reinterpret_cast<CCSPlayerController*>(pEvent->GetPlayerController(X("userid")));
        if (pAttacker && pAttacker != pVictim && pAttacker->m_bIsLocalPlayerController() && pAttackerPawn)
        {
            CPlayer_WeaponServices* pWeaponServices = pAttackerPawn->m_pWeaponServices();
            if (!pWeaponServices)
                return;

            C_BasePlayerWeapon* pWeapon = pWeaponServices->m_hActiveWeapon().Get();
            if (!pWeapon)
                return;

            C_AttributeContainer* pAttributeManager = pWeapon->m_AttributeManager();
            if (!pAttributeManager)
                return;

            C_EconItemView* pEconItem = pAttributeManager->m_Item();
            if (!pEconItem)
                return;

            pEvent->SetString(X("weapon"), X("rifle"));
        }
        break;
    }
    case FNV1A::HashConst("player_hurt"):
    {
        CCSPlayerController* pAttacker = reinterpret_cast<CCSPlayerController*>(pEvent->GetPlayerController(X("attacker")));
        C_BasePlayerPawn* pAttackerPawn = reinterpret_cast<C_CSPlayerPawn*>(pEvent->GetPlayerPawn(X("attacker")));

        CCSPlayerController* pVictim = reinterpret_cast<CCSPlayerController*>(pEvent->GetPlayerController(X("userid")));
        if (pAttacker && pAttacker != pVictim && pAttacker->m_bIsLocalPlayerController() && pAttackerPawn)
        {
            CPlayer_WeaponServices* pWeaponServices = pAttackerPawn->m_pWeaponServices();
            if (!pWeaponServices)
                return;

            C_BasePlayerWeapon* pWeapon = pWeaponServices->m_hActiveWeapon().Get();
            if (!pWeapon)
                return;

            C_AttributeContainer* pAttributeManager = pWeapon->m_AttributeManager();
            if (!pAttributeManager)
                return;

            C_EconItemView* pEconItem = pAttributeManager->m_Item();
            if (!pEconItem)
                return;

            pEvent->SetString(X("weapon"), X("rifle"));
        }
        break;
    }
    }
}

bool InventoryChanger::OnConfigSave(std::string_view szFileName)
{
    CCSPlayerInventory* pInventory = CCSPlayerInventory::GetPlayerInventory();
    if (!pInventory)
        return false;

    std::filesystem::path fsFilePath(szFileName);
    if (fsFilePath.extension() != X(".json"))
        fsFilePath.replace_extension(X(".json"));

    const std::string szFile = std::filesystem::path(Config::fsInventoryPath / fsFilePath).string();
    nlohmann::json config = { };

    try
    {
        for (InventoryItem_t& item : m_vecAddedItems)
        {
            if (!&item)
                continue;

            item.m_bEquippedCT = false;
            item.m_bEquippedT = false;
            item.m_nEquippedSlot = LOADOUT_SLOT_INVALID;

            for (int i = 0; i <= LOADOUT_SLOT_COUNT; ++i)
            {
                for (int j = TEAM_TT; j <= TEAM_CT; j++)
                {
                    C_EconItemView* pItemView = pInventory->GetItemInLoadout(j, i);
                    if (!pItemView)
                        continue;

                    if (pItemView->m_iItemID() == item.m_uItemID)
                    {
                        if (!item.m_bEquippedCT)
                            item.m_bEquippedCT = j == TEAM_CT;

                        if (!item.m_bEquippedT)
                            item.m_bEquippedT = j == TEAM_TT;

                        item.m_nEquippedSlot = i;
                    }
                }
            }

            nlohmann::json entry = { };

            entry[X("item-name")] = item.m_strItemName;
            entry[X("skin-name")] = item.m_strSkinName;

            entry[X("paint-kit-id")] = item.m_nPaintKit;
            entry[X("item-definition-index")] = item.m_uItemDefinitionIndex;
            entry[X("item-rarity")] = item.m_uItemRarity;

            entry[X("stat-track")] = item.m_nStatTrak;
            entry[X("seed")] = item.m_nSeed;
            entry[X("wear")] = item.m_flWear;
            entry[X("custom-name")] = item.m_strCustomName;

            entry[X("item-id")] = item.m_uItemID;

            entry[X("equipped-slot")] = item.m_nEquippedSlot;
            entry[X("equipped-ct")] = item.m_bEquippedCT;
            entry[X("equipped-t")] = item.m_bEquippedT;

            config.emplace_back(entry);
        }
    }
    catch (const nlohmann::detail::exception& ex)
    {
        Logging::PushConsoleColor(FOREGROUND_RED);
        Logging::Print(X("[error] json save failed: {}"), ex.what());
        Logging::PopConsoleColor();
        return false;
    }

    std::ofstream ofsOutFile(szFile, std::ios::out | std::ios::trunc);
    if (!ofsOutFile.good())
        return false;

    try
    {
        ofsOutFile << config.dump(4);
        ofsOutFile.close();
    }
    catch (std::ofstream::failure& ex)
    {
        Logging::PushConsoleColor(FOREGROUND_RED);
        Logging::Print(X("[error] failed to save inventory: {}"), ex.what());
        Logging::PopConsoleColor();
        return false;
    }

    Logging::Print(X("saved inventory at: {}"), szFile);
    return true;
}

bool InventoryChanger::OnConfigLoad(std::string_view szFileName)
{
    CCSPlayerInventory* pInventory = CCSPlayerInventory::GetPlayerInventory();
    if (!pInventory)
        return false;

    CCSInventoryManager* pInventoryManager = CCSInventoryManager::GetInventoryManager();
    if (!pInventoryManager)
        return false;

    // first remove everything
    for (InventoryItem_t& item : m_vecAddedItems)
        pInventory->RemoveEconItem(pInventory->GetSOCDataForItem(item.m_uItemID));

    // clear the item list
    m_vecAddedItems.clear();

    // now fill it again from config
    {
        // get utf-8 full path to config
        const std::string szFile = std::filesystem::path(Config::fsInventoryPath / szFileName).string();
        nlohmann::json config = { };

        // open input config file
        std::ifstream ifsInputFile(szFile, std::ios::in);

        if (!ifsInputFile.good())
            return false;

        try
        {
            // parse saved variables
            config = nlohmann::json::parse(ifsInputFile, nullptr, false);

            // check is json parse failed
            if (config.is_discarded())
                return false;

            ifsInputFile.close();
        }
        catch (std::ifstream::failure& ex)
        {
            Logging::PushConsoleColor(FOREGROUND_RED);
            Logging::Print(X("[error] failed to load configuration: {}"), ex.what());
            Logging::PopConsoleColor();
            return false;
        }

        try
        {
            for (const auto& item : config)
            {
                InventoryItem_t inventoryItem;

                // start filling the item
                inventoryItem.m_strItemName = item[X("item-name")].get<std::string>();
                inventoryItem.m_strSkinName = item[X("skin-name")].get<std::string>();

                inventoryItem.m_nPaintKit = item[X("paint-kit-id")].get<std::int32_t>();
                inventoryItem.m_uItemDefinitionIndex = item[X("item-definition-index")].get<std::uint16_t>();
                inventoryItem.m_uItemRarity = item[X("item-rarity")].get<std::uint16_t>();

                inventoryItem.m_nStatTrak = item[X("stat-track")].get<std::int32_t>();
                inventoryItem.m_nSeed = item[X("seed")].get<std::int32_t>();
                inventoryItem.m_flWear = item[X("wear")].get<float>();
                inventoryItem.m_strCustomName = item[X("custom-name")].get<std::string>();

                inventoryItem.m_uItemID = item[X("item-id")].get<uint64_t>();

                inventoryItem.m_nEquippedSlot = item[X("equipped-slot")].get<std::int32_t>();
                inventoryItem.m_bEquippedCT = item[X("equipped-ct")].get<bool>();
                inventoryItem.m_bEquippedT = item[X("equipped-t")].get<bool>();

                // create the item
                CEconItem* pItem = CEconItem::CreateInstance();
                if (pItem)
                {
                    CCSPlayerInventory* pInventory = CCSPlayerInventory::GetPlayerInventory();
                    auto HighestIDs = pInventory->GetHighestIDs();
                    pItem->m_ulID = HighestIDs.first + 1;
                    pItem->m_unInventory = HighestIDs.second + 1;
                    pItem->m_unAccountID = static_cast<std::uint32_t>(pInventory->GetOwner().m_uID);
                    pItem->m_unDefIndex = inventoryItem.m_uItemDefinitionIndex;

                    pItem->m_nRarity = inventoryItem.m_uItemRarity;

                    if (inventoryItem.m_strCustomName != X("Agent"))
                    {
                        pItem->SetPaintKit(static_cast<float>(inventoryItem.m_nPaintKit));
                        pItem->SetPaintSeed(static_cast<float>(inventoryItem.m_nSeed));
                        pItem->SetPaintWear(inventoryItem.m_flWear);

                        if (inventoryItem.m_nStatTrak > 0)
                        {
                            pItem->SetStatTrak(inventoryItem.m_nStatTrak);
                            pItem->SetStatTrakType(0);

                            // Applied automatically on knives.
                            if (pItem->m_nQuality != IQ_UNUSUAL)
                                pItem->m_nQuality = IQ_STRANGE;
                        }
                    }

                    if (pInventory->AddEconItem(pItem))
                        InventoryChanger::AddEconItemToList
                        (
                            InventoryItem_t(
                                inventoryItem.m_strItemName,
                                inventoryItem.m_strSkinName,
                                inventoryItem.m_nPaintKit,
                                inventoryItem.m_uItemDefinitionIndex,
                                inventoryItem.m_uItemRarity,
                                inventoryItem.m_nStatTrak,
                                inventoryItem.m_nSeed,
                                inventoryItem.m_flWear,
                                inventoryItem.m_strCustomName,
                                pItem->m_ulID,
                                inventoryItem.m_nEquippedSlot,
                                inventoryItem.m_bEquippedCT,
                                inventoryItem.m_bEquippedT
                            )

                        );

                    // equip item on CT slot if it should be equipped
                    if (inventoryItem.m_bEquippedCT)
                        pInventoryManager->EquipItemInLoadout(TEAM_CT, inventoryItem.m_nEquippedSlot, pItem->m_ulID);

                    // equip item on T slot if it should be equipped
                    if (inventoryItem.m_bEquippedT)
                        pInventoryManager->EquipItemInLoadout(TEAM_TT, inventoryItem.m_nEquippedSlot, pItem->m_ulID);
                }
            }
        }
        catch (const nlohmann::detail::exception& ex)
        {
            Logging::PushConsoleColor(FOREGROUND_RED);
            Logging::Print(X("[error] json load failed: {}"), ex.what());
            Logging::PopConsoleColor();
            return false;
        }

        Logging::Print(X("loaded inventory at: {}"), szFile);
        return true;
    }
}

void InventoryChanger::OnConfigRemove(std::string_view szFileName)
{
    const std::string szFile = std::filesystem::path(Config::fsInventoryPath / szFileName).string();

    if (std::filesystem::remove(szFile))
        Logging::Print(X("removed inventory file at: {}"), szFile);
}

void InventoryChanger::OnConfigRefresh()
{
    Config::vecInventoryFileNames.clear();

    for (const auto& it : std::filesystem::directory_iterator(Config::fsInventoryPath))
    {
        if (it.path().filename().extension() == X(".json"))
        {
            Logging::Print(X("found inventory file: {}"), it.path().filename().string());
            Config::vecInventoryFileNames.emplace_back(it.path().filename().string());
        }
    }
}

void InventoryChanger::SetCustomModel(CCSPlayerController* pController, C_CSPlayerPawn* pPawn, CCSPlayerInventory* pInventory)
{
    if (!Config::b(g_Variables.m_Visuals.m_bCustomModelChanger))
        return;

    if (!Interfaces::m_pEngine->IsInGame())
        return;

    if (!Globals::m_pLocalPlayerController)
        return;

    if (!Globals::m_pLocalPlayerController->m_bPawnIsAlive())
        return;

    if (!pController || !pPawn || !pInventory)
        return;

    if (pPawn != Globals::m_pLocalPlayerPawn)
        return;

    C_EconItemView* pItemViewLoadout = pInventory->GetItemInLoadout(CCSPlayerController::GetLocalPlayerController()->m_iTeamNum(), LOADOUT_SLOT_CLOTHING_CUSTOMPLAYER);
    if (!pItemViewLoadout)
        return;

    CEconItemDefinition* pEconDefintion = pItemViewLoadout->GetStaticData();
    if (!pEconDefintion)
        return;

    CGameSceneNode* pGameSceneNode = pPawn->m_pGameSceneNode();
    if (!pGameSceneNode)
        return;

    CSkeletonInstance* pSkeletonInstance = pGameSceneNode->GetSkeletonInstance();
    if (!pSkeletonInstance)
        return;

    if (!m_bShouldUpdateCustomModel)
        return;

    const char* szModel = m_szCustomModelPath.c_str();
    if (!szModel || strlen(szModel) == 0)
        return;

    try {
        Interfaces::m_pResourceSystem->LoadResource(CBufferString(szModel, 'ldmv'), "");

        if (pPawn->m_iHealth() > 0) {
            Globals::m_pLocalPlayerPawn->SetModel(szModel);
        }

        m_bShouldUpdateCustomModel = false;
    }
    catch (...) {
        m_bShouldUpdateCustomModel = false;
        return;
    }
}
