
#include <string>
#include <thread>
#include <future>
#include <iostream>

#include "Cheats.h"
#include "Render.h"
#include "../Core/Config.h"
#include "../Features/GrenadePrediction.h"
#include "../Features/DroppedItemsESP.h"
#include "../Features/AntiFlash.h"
#include "../Features/AutoFire.h"
#include "../Features/WallbangIndicator.h"
#include "../Features/ProfileInfo.h"
#include "../Features/FovCircle.h"

#include "../Core/Init.h"

#include "../Features/ESP.h"
#include "../Core/GUI.h"
#include "../Features/BombTimer.h"
#include "../Features/SpectatorList.h"
#include "../Helpers/Logger.h"
#include "../Features/SoundESP.h"

int PreviousTotalHits = 0;

void Menu();
void Visual(const CEntity&);
void Radar(Base_Radar, const CEntity&);
void MiscFuncs(CEntity&);
void RenderCrosshair(ImDrawList*, const CEntity&);
void RadarSetting(Base_Radar&);

void Cheats::Run()
{	
	Menu();

	Misc::AutoAccept::UpdateAutoAccept();

#ifndef DBDEBUG
	if (!Init::Client::isGameWindowActive() && !MenuConfig::ShowMenu) {
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		return;
	}
#endif

	// Update matrix
	if (!memoryManager.ReadMemory(gGame.GetMatrixAddress(), gGame.View.Matrix,64))
		return;

	// Update EntityList Entry
	gGame.UpdateEntityListEntry();

	DWORD64 LocalControllerAddress = 0;
	DWORD64 LocalPawnAddress = 0;

	if (!memoryManager.ReadMemory(gGame.GetLocalControllerAddress(), LocalControllerAddress))
		return;
	if (!memoryManager.ReadMemory(gGame.GetLocalPawnAddress(), LocalPawnAddress))
		return;

	if (LocalPawnAddress == 0 || LocalControllerAddress == 0) {
        g_globalVars->UpdateGlobalvars();
        cachedResults.clear();
        return;
    }

	// LocalEntity
	CEntity LocalEntity;
	int LocalPlayerControllerIndex = 0;
	LocalEntity.UpdateClientData();
	if (!LocalEntity.UpdateController(LocalControllerAddress))
		return;
	if (!LocalEntity.UpdatePawn(LocalPawnAddress) && !MenuConfig::WorkInSpec)
		return;

	// Update m_currentTick
	bool success = memoryManager.ReadMemory<DWORD>(LocalEntity.Controller.Address + Offset.PlayerController.m_nTickBase, m_currentTick);
	if (!success) {
		m_currentTick = 0;
	}

	// aimbot data
	std::vector<Vec3> AimPosList;

	// radar data
	Base_Radar GameRadar;
	if ((MiscCFG::Radar && LocalEntity.Controller.TeamID != 0) || (MiscCFG::Radar && MenuConfig::ShowMenu))
		RadarSetting(GameRadar);

	// process entities
	auto entityResults = ProcessEntities(LocalEntity, LocalPlayerControllerIndex);
	
	// render, collect aim data
	HandleEnts(entityResults, LocalEntity, LocalPlayerControllerIndex, GameRadar, AimPosList);

	Visual(LocalEntity);
	Radar(GameRadar, LocalEntity);
	MiscFuncs(LocalEntity);

	int currentFPS = static_cast<int>(ImGui::GetIO().Framerate);
	if (currentFPS > MenuConfig::RenderFPS)
	{
		int FrameWait = round(1000.0f / MenuConfig::RenderFPS);
		std::this_thread::sleep_for(std::chrono::milliseconds(FrameWait));
	}
	
	// run trigger & aim every new tick
	if (m_currentTick != m_previousTick)
	{
		std::vector<CEntity> allEntities;
		for (const auto& pair : cachedResults) {
			allEntities.push_back(pair.second);
		}
		if (MiscCFG::SpecList)
		SpecList::GetSpectatorList(allEntities, LocalEntity);
		m_previousTick = m_currentTick;
	}
}

// collect entity data
std::vector<std::pair<int, CEntity>> Cheats::CollectEntityData(CEntity& localEntity, int& localPlayerControllerIndex)
{
	// update only on new tick
	//if (m_currentTick == m_previousTick)
	//{
	//	return cachedResults;
	//}

	std::vector<EntityBatchData> batchData;
	batchData.reserve(64);

	// collect entity addresses
	for (int entityIndex = 0; entityIndex < 64; ++entityIndex)
	{
		DWORD64 entityAddress = 0;
		if (!memoryManager.ReadMemory<DWORD64>(gGame.GetEntityListEntry() + (entityIndex + 1) * 0x70, entityAddress))
		{
			continue;
		}

		// skip local player
		if (entityAddress == localEntity.Controller.Address)
		{
			localPlayerControllerIndex = entityIndex;
			continue;
		}

		// get pawn address
		CEntity tempEntity;
		tempEntity.Controller.Address = entityAddress;
		DWORD64 pawnAddress = tempEntity.Controller.GetPlayerPawnAddress();
		
		if (pawnAddress != 0)
		{
			batchData.emplace_back(entityIndex, entityAddress, pawnAddress);
		}
	}

	if (batchData.empty())
	{
		return {};
	}

	// process all entities in batch
	std::vector<std::pair<int, CEntity>> entities;
	EntityBatchProcessor processor;
	if (!processor.ProcessAllEntities(entities, batchData))
	{
		return {};
	}

	// update cache
	cachedResults = entities;

	return cachedResults;
}

// process, prepare results
std::vector<EntityResult> Cheats::ProcessEntities(CEntity& localEntity, int& localPlayerControllerIndex)
{
	// get batch-processed entities
	auto entities = CollectEntityData(localEntity, localPlayerControllerIndex);
	std::vector<EntityResult> results;
	results.reserve(entities.size());

	// process each entity
	for (auto& [entityIndex, entity] : entities)
	{
		EntityResult result;
		result.entityIndex = entityIndex;
		result.entity = entity;

		if (!entity.IsAlive())
			continue;

		// skip teammates if team check enabled
		if (MenuConfig::TeamCheck && entity.Controller.TeamID == localEntity.Controller.TeamID)
			continue;

		// check if in screen
		result.isInScreen = entity.IsInScreen();

		// calculate distance
		result.distance = static_cast<int>(entity.Pawn.Pos.DistanceTo(localEntity.Pawn.Pos) / 100);

		// calculate esp box rect
		if (ESPConfig::ESPenabled && result.isInScreen)
			result.espRect = ESP::GetBoxRect(entity, ESPConfig::BoxType);

		// sound esp
		if (ESPConfig::ESPenabled && ESPConfig::EnemySound && result.entity.Controller.Address != localEntity.Controller.Address)
			SoundESP::ProcessSound(result.entity, localEntity);

		// process dropped items for dead players
		DroppedItemsESP::ProcessEntityWeapons(result.entity);

		// Process anti-flash for local player
		if (result.entity.Controller.Address == localEntity.Controller.Address)
			AntiFlash::Process(result.entity);

		// Process FOV Circle for local player
		if (result.entity.Controller.Address == localEntity.Controller.Address)
			FovCircle::Process(result.entity);

		// Process AutoFire for all entities
		if (result.entity.Controller.Address != localEntity.Controller.Address)
			AutoFire::Process({result.entity}, localEntity);

		result.isValid = true;
		results.push_back(result);
	}
	
	// process grenade entities (for all grenades in game) - call after collecting all entities
	GrenadePrediction::ProcessGrenadeEntities(Cheats::cachedResults);
	
	// process wallbang targets
	WallbangIndicator::Process(Cheats::cachedResults, localEntity);
	
	// process profile info
	ProfileInfo::Process(Cheats::cachedResults, localEntity);
	
	return results;
}

// render, collect aim data
void Cheats::HandleEnts(const std::vector<EntityResult>& entities, CEntity& localEntity, 
	int localPlayerControllerIndex, Base_Radar& gameRadar, std::vector<Vec3>& aimPosList)
{
	// healthbar map (static)
	static std::map<DWORD64, Render::HealthBar> HealthBarMap;

	// aimbot data
	float MaxAimDistance = 100000;

	for (const auto& result : entities)
	{
		if (!result.isValid)
		{
			if (HealthBarMap.count(result.entity.Controller.Address))
				HealthBarMap.erase(result.entity.Controller.Address);
			continue;
		}

		const auto& entity = result.entity;
		const int entityIndex = result.entityIndex;

		// add entity to radar
		if (MiscCFG::Radar && localEntity.Controller.TeamID != 0)
		{
			gameRadar.AddPoint(localEntity.Pawn.Pos, localEntity.Pawn.ViewAngle.y, 
				entity.Pawn.Pos, ImColor(237, 85, 106, 200), RadarCFG::RadarType, entity.Pawn.ViewAngle.y);
		}


        // skip not in screen
		if (!result.isInScreen)
		{
			continue;
		}

		// render esp
		// Improved ESP rendering with better validation
		if (ESPConfig::ESPenabled)
		{
			// Additional validation to prevent ESP issues
			if (entity.Pawn.Health > 0 && entity.Pawn.Health <= 100 && 
				entity.Pawn.Address != 0 && entity.Controller.Address != 0 &&
				(entity.Pawn.Pos.x != 0 || entity.Pawn.Pos.y != 0 || entity.Pawn.Pos.z != 0))
			{
				// Flash check with fallback
				bool shouldRender = true;
				if (ESPConfig::FlashCheck && localEntity.Pawn.FlashDuration > 0.1f) {
					shouldRender = false; // Skip ESP if local player is flashed
				}
				
				if (shouldRender) {
					const ImVec4& Rect = result.espRect;
					const int distance = result.distance;

					// Distance check with validation
					if (MenuConfig::RenderDistance == 0 || (distance <= MenuConfig::RenderDistance && MenuConfig::RenderDistance > 0))
					{
						ESP::RenderPlayerESP(localEntity, entity, Rect, localPlayerControllerIndex, entityIndex);
						Render::DrawDistance(localEntity, entity, Rect);

						// healthbar
						if (ESPConfig::ShowHealthBar)
						{
							ImVec2 HealthBarPos = { Rect.x - 6.f, Rect.y };
							ImVec2 HealthBarSize = { 4, Rect.w };
							Render::DrawHealthBar(entity.Controller.Address, 100, entity.Pawn.Health, HealthBarPos, HealthBarSize);
						}

						// ammo
						// When player is using knife or nade, Ammo = -1.
						if (ESPConfig::AmmoBar && entity.Pawn.Ammo != -1)
						{
							ImVec2 AmmoBarPos = { Rect.x, Rect.y + Rect.w + 2 };
							ImVec2 AmmoBarSize = { Rect.z, 4 };
							Render::DrawAmmoBar(entity.Controller.Address, entity.Pawn.Ammo + entity.Pawn.ShotsFired, 
								entity.Pawn.Ammo, AmmoBarPos, AmmoBarSize);
						}

						// armor
						// It is meaningless to render a empty bar
						if (ESPConfig::ArmorBar && entity.Pawn.Armor > 0)
						{
							bool HasHelmet;
							ImVec2 ArmorBarPos;
							memoryManager.ReadMemory(entity.Controller.Address + Offset.PlayerController.HasHelmet, HasHelmet);
							
							if (ESPConfig::ShowHealthBar)
								ArmorBarPos = { Rect.x - 12.f, Rect.y };
							else
								ArmorBarPos = { Rect.x - 6.f, Rect.y };
							
							ImVec2 ArmorBarSize = { 4, Rect.w };
							Render::DrawArmorBar(entity.Controller.Address, 100, entity.Pawn.Armor, HasHelmet, ArmorBarPos, ArmorBarSize);
						}
					}
				}
			}
		}
	}
}

void Menu() 
{
	if (MenuConfig::ShowMenu)
		GUI::DrawGui();
}

void Visual(const CEntity& LocalEntity)
{
	// Fov circle
	if (LocalEntity.Controller.TeamID != 0 && !MenuConfig::ShowMenu)
		Render::DrawFovCircle(ImGui::GetBackgroundDrawList(), LocalEntity);

	// HeadShoot Line
	Render::HeadShootLine(LocalEntity, MiscCFG::HeadShootLineColor);

	RenderCrosshair(ImGui::GetBackgroundDrawList(), LocalEntity);
}

void Radar(Base_Radar Radar, const CEntity& LocalEntity)
{
	// Radar render
	if ((MiscCFG::Radar && LocalEntity.Controller.TeamID != 0) || (MiscCFG::Radar && MenuConfig::ShowMenu))
	{
		Radar.Render();

		MenuConfig::RadarWinPos = ImGui::GetWindowPos();
		ImGui::End();
	}
}

void MiscFuncs(CEntity& LocalEntity)
{
    if (MiscCFG::SpecList)
        SpecList::SpectatorWindowList(LocalEntity);
    if (MiscCFG::bmbTimer)
        bmb::RenderWindow(LocalEntity.Controller.TeamID);
    if (ESPConfig::EnemySound)
        SoundESP::Render();
    
    // Render grenade prediction
    GrenadePrediction::Render();
    
    // Render dropped items
    DroppedItemsESP::Render();
    
    // Render anti-flash
    AntiFlash::Render();
    
    // Render Wallbang Indicator
    WallbangIndicator::Render();
    
    // Render Profile Info
    ProfileInfo::Render();
    
    // Render FOV Circle
    FovCircle::Render();
    
    // Render AutoFire
    AutoFire::Render();

    Misc::HitManager(LocalEntity, PreviousTotalHits);
    Misc::BunnyHop(LocalEntity);
    Misc::Watermark(LocalEntity);
    Misc::FastStop();
    Misc::AntiAFKKickUpdate();
    if (MiscCFG::AutoKnife && !MenuConfig::ShowMenu) {
        std::vector<CEntity> enemyList;
        enemyList.reserve(Cheats::cachedResults.size());
        for (const auto& r : Cheats::cachedResults) enemyList.push_back(r.second);
        Misc::KnifeBot(LocalEntity, enemyList);
    }
    if (MiscCFG::AutoZeus && !MenuConfig::ShowMenu) {
        Misc::ZeusBot(LocalEntity);
    }
}

void RadarSetting(Base_Radar& Radar)
{
	// Radar window
	ImGui::SetNextWindowBgAlpha(RadarCFG::RadarBgAlpha);
	ImGui::Begin("Radar", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
	ImGui::SetWindowSize({ RadarCFG::RadarRange * 2,RadarCFG::RadarRange * 2 });
	ImGui::SetWindowPos(MenuConfig::RadarWinPos, ImGuiCond_Once);

	if (MenuConfig::RadarWinChengePos)
	{
		ImGui::SetWindowPos("Radar", MenuConfig::RadarWinPos);
		MenuConfig::RadarWinChengePos = false;
	}

	if (!RadarCFG::customRadar)
	{
		RadarCFG::ShowRadarCrossLine = false;
		RadarCFG::Proportion = 2700.f;
		RadarCFG::RadarPointSizeProportion = 1.f;
		RadarCFG::RadarRange = 125.f;
		RadarCFG::RadarBgAlpha = 0.1f;
	}


	// Radar.SetPos({ Gui.Window.Size.x / 2,Gui.Window.Size.y / 2 });
	Radar.SetDrawList(ImGui::GetWindowDrawList());
	Radar.SetPos({ ImGui::GetWindowPos().x + RadarCFG::RadarRange, ImGui::GetWindowPos().y + RadarCFG::RadarRange });
	Radar.SetProportion(RadarCFG::Proportion);
	Radar.SetRange(RadarCFG::RadarRange);
	Radar.SetSize(RadarCFG::RadarRange * 2);
	Radar.SetCrossColor(RadarCFG::RadarCrossLineColor);

	Radar.ArcArrowSize *= RadarCFG::RadarPointSizeProportion;
	Radar.ArrowSize *= RadarCFG::RadarPointSizeProportion;
	Radar.CircleSize *= RadarCFG::RadarPointSizeProportion;

	Radar.ShowCrossLine = RadarCFG::ShowRadarCrossLine;
	Radar.Opened = true;
}

void RenderCrosshair(ImDrawList* drawList, const CEntity& LocalEntity)
{
	if (!MiscCFG::SniperCrosshair || LocalEntity.Controller.TeamID == 0 || MenuConfig::ShowMenu)
		return;

	// Check if player has sniper rifle
	std::string weaponName = LocalEntity.Pawn.WeaponName;
	bool isSniper = weaponName.find("awp") != std::string::npos || 
					weaponName.find("ssg08") != std::string::npos ||
					weaponName.find("scar20") != std::string::npos ||
					weaponName.find("g3sg1") != std::string::npos;

	if (!isSniper)
		return;

	bool isScoped;
	memoryManager.ReadMemory<bool>(LocalEntity.Pawn.Address + Offset.Pawn.isScoped, isScoped);

	// Show crosshair when not scoped with sniper rifle
	if (!isScoped)
		Render::DrawCrossHair(drawList, ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y / 2), MiscCFG::SniperCrosshairColor);
}

std::string Cheats::GetCurrentMapName() {
    if (!g_globalVars || !g_globalVars->g_cCurrentMap) {
        return "";
    }

    char currentMap[256] = { 0 };
    if (!memoryManager.ReadMemory(reinterpret_cast<DWORD64>(g_globalVars->g_cCurrentMap),
        currentMap, sizeof(currentMap) - 1)) {
        return "";
    }

    currentMap[255] = '\0';
    return std::string(currentMap);
}