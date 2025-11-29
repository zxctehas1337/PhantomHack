#pragma once

enum EItemType : std::uint16_t
{
    ITEM_TYPE_NONE = 0,
    ITEM_TYPE_WEAPON,
    ITEM_TYPE_KNIFE,
    ITEM_TYPE_GLOVE,
    ITEM_TYPE_AGENT,
    ITEM_TYPE_STICKER,
    ITEM_TYPE_MUSIC,
    ITEM_TYPE_GRAFFITI,
    ITEM_TYPE_PATCH,
    ITEM_TYPE_CASE,
    ITEM_TYPE_KEY,
    ITEM_TYPE_PASS,
};

struct DumpedSkin_t
{
    std::string m_strName = X("");
    int m_nID = 0;
    int m_nRarity = 0;
    Color m_rgbaColor[4] = {};
};

struct DumpedItem_t
{
    std::string m_strName = X("");
    std::uint16_t m_uDefinitionIndex = 0U;
    int m_nRarity = 0;
    bool m_bUnusualItem = false;
    std::vector<DumpedSkin_t> m_vecDumpedSkins = {};
    DumpedSkin_t* m_pSelectedSkin = nullptr;
    EItemType m_eItemType = EItemType::ITEM_TYPE_NONE;
};

class CMaterialRecord {
public:
    std::uint32_t m_nUnk;
    std::uint32_t m_nMagicNumber;
    std::uint32_t m_nHandle;
    std::uint32_t m_nType;
};

class CMaterialData_Skin {
public:
    CMaterialRecord* m_pRecords;
    std::int32_t m_nRecordsSize;
};

struct DumpedAgent_t
{
    std::string m_strName = X("");
    std::string m_strModel = X("");
    int m_nRarity = 0;
    int m_nDefinitionIndex = 0;
};

struct InventoryItem_t
{
    std::string m_strItemName = X("");
    std::string m_strSkinName = X("");

    std::int32_t m_nPaintKit = 0;
    std::uint16_t m_uItemDefinitionIndex = 0U;
    std::uint16_t m_uItemRarity = 0U;

    std::int32_t m_nStatTrak = 0;
    std::int32_t m_nSeed = 0;
    float m_flWear = 0.0f;
    std::string m_strCustomName = X("");

    std::uint64_t m_uItemID = 0U;

    std::int32_t m_nEquippedSlot = LOADOUT_SLOT_INVALID;
    bool m_bEquippedCT = false;
    bool m_bEquippedT = false;
};

namespace InventoryChanger
{
    // run
    void Run();
    // set glove
    void SetGlove(CCSPlayerController* pController, C_CSPlayerPawn* pPawn, CCSPlayerInventory* pInventory);
    // set agent
    void SetAgent(CCSPlayerController* pController, C_CSPlayerPawn* pPawn, CCSPlayerInventory* pInventory);
    // dump all skins
    bool DumpAllSkins();

    inline bool m_bWantsHUDUpdate = false;
    void UpdateHUD();

    // hooks
    void OnEquipItemInLoadout(int nTeam, int nSlot, std::uint64_t uItemID);
    void OnSetModel(C_BaseModelEntity* pEntity, const char*& szModel);

    // add item to our list
    void AddEconItemToList(InventoryItem_t item);

    // remove item from our list
    void Restore();

    // fix events
    void OnFireEvent(CGameEvent* pEvent);

    // store items in file
    bool OnConfigSave(std::string_view szFileName);
    // add items to inventory and equip them
    bool OnConfigLoad(std::string_view szFileName);
    // remove configuration
    void OnConfigRemove(std::string_view szFileName);
    // refresh all configurations
    void OnConfigRefresh();

    inline std::vector<InventoryItem_t> m_vecAddedItems = {};

    inline std::vector<DumpedItem_t> m_vecDumpedItems = {};
    inline DumpedItem_t* m_pSelectedItem = nullptr;

    inline std::vector<DumpedAgent_t> m_vecDumpedAgents;
    inline DumpedAgent_t* m_pSelectedAgent = nullptr;

    inline bool m_bNewRound = true;
    inline GameTime_t m_fllastSpawnTimeIndex = 0.0f;

    void SetCustomModel(CCSPlayerController* pController, C_CSPlayerPawn* pPawn, CCSPlayerInventory* pInventory);

    inline bool m_bShouldUpdateCustomModel = false;
    inline std::string m_szCustomModelPath = "error.vmdl";
};
