/* =============================================================
TO DO:
• Merge human sql template with alliance template
• As Barbz suggested: Rename to character_template the module
    and all related files (to be less confusing and less generic)
• As Barbz suggested: Scaling system for twink servers
================================================================ */

#include "TemplateNPC.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "Chat.h"

void sTemplateNPC::LearnPlateMailSpells(Player* player)
{
    switch (player->getClass())
    {
    case CLASS_WARRIOR:
    case CLASS_PALADIN:
    case CLASS_DEATH_KNIGHT:
        player->learnSpell(PLATE_MAIL);
        break;
    case CLASS_SHAMAN:
    case CLASS_HUNTER:
        player->learnSpell(MAIL);
        break;
    default:
        break;
    }
}

void sTemplateNPC::ApplyBonus(Player* player, Item* item, EnchantmentSlot slot, uint32 bonusEntry)
{
    if (!item)
        return;

    if (!bonusEntry || bonusEntry == 0)
        return;

    item->SetEnchantment(slot, bonusEntry, 0, 0);
}

void sTemplateNPC::ApplyGlyph(Player* player, uint8 slot, uint32 glyphID)
{
    if (GlyphPropertiesEntry const* gp = sGlyphPropertiesStore.LookupEntry(glyphID))
    {
        if (uint32 oldGlyph = player->GetGlyph(slot))
        {
            player->RemoveAurasDueToSpell(sGlyphPropertiesStore.LookupEntry(oldGlyph)->SpellId);
            player->SetGlyph(slot, 0, true);
        }
        player->CastSpell(player, gp->SpellId, true);
        player->SetGlyph(slot, glyphID, true);
    }
}
void sTemplateNPC::RemoveAllGear(Player* player)
{
    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        player->DestroyItem(INVENTORY_SLOT_BAG_0, i, true);
    }
    player->GetSession()->SendAreaTriggerMessage("Your equipped gear has been destroyed.");
}
void sTemplateNPC::RemoveAllGlyphs(Player* player)
{
    for (uint8 i = 0; i < MAX_GLYPH_SLOT_INDEX; ++i)
    {
        if (uint32 glyph = player->GetGlyph(i))
        {
            if (GlyphPropertiesEntry const* gp = sGlyphPropertiesStore.LookupEntry(glyph))
            {
                if (GlyphSlotEntry const* gs = sGlyphSlotStore.LookupEntry(player->GetGlyphSlot(i)))
                {
                    player->RemoveAurasDueToSpell(sGlyphPropertiesStore.LookupEntry(glyph)->SpellId);
                    player->SetGlyph(i, 0, true);
                    player->SendTalentsInfoData(false); // this is somewhat an in-game glyph realtime update (apply/remove)
                }
            }
        }
    }
}

bool sTemplateNPC::UnequipAllGear(Player* player)
{
    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        uint16 dest = ((INVENTORY_SLOT_BAG_0 << 8) | i);
        Item*  pDstItem = player->GetItemByPos(dest);
        if (pDstItem)
        {
            uint8   dstbag  = pDstItem->GetBagSlot();
            uint8   dstslot = pDstItem->GetSlot();
            InventoryResult msg = player->CanUnequipItem(dest, false);
            if (msg != EQUIP_ERR_OK)
            {
                player->SendEquipError(msg, pDstItem, nullptr);
                return false;
            }
            // check dest->src move possibility
            ItemPosCountVec sSrc;
            msg                  = player->CanStoreItem(NULL_BAG, NULL_SLOT, sSrc, pDstItem, true);

            if (msg != EQUIP_ERR_OK)
            {
                player->SendEquipError(msg, pDstItem, nullptr);
                return false;
            }
            // now do moves, remove...
            player->RemoveItem(dstbag, dstslot, true, true);
            // add to src
            player->StoreItem(sSrc, pDstItem, true);
            player->AutoUnequipOffhandIfNeed();
            // Xinef: Call this here after all needed items are equipped
            player->RemoveItemDependentAurasAndCasts((Item*) nullptr);
        }

    }
    return true;
}

void sTemplateNPC::GiveAndEquipItem(Player* player, Item* item)
{
    uint16          dest;
    InventoryResult msg = player->CanEquipItem(NULL_SLOT, dest, item, !item->IsBag());
    if (msg != EQUIP_ERR_OK)
    {
        player->SendEquipError(msg, item, nullptr);
        return;
    }

    Item* pDstItem = player->GetItemByPos(dest);
    if (!pDstItem) // empty slot, simple case
    {
        player->EquipItem(dest, item, true);
        player->AutoUnequipOffhandIfNeed();
    }
    else // have currently equipped item, not simple case
    {
        uint8 dstbag  = pDstItem->GetBagSlot();
        uint8 dstslot = pDstItem->GetSlot();

        msg = player->CanUnequipItem(dest, !item->IsBag());
        if (msg != EQUIP_ERR_OK)
        {
            player->SendEquipError(msg, pDstItem, nullptr);
            return;
        }

        // check dest->src move possibility
        ItemPosCountVec sSrc;
        msg = player->CanStoreItem(NULL_BAG, NULL_SLOT, sSrc, pDstItem, true);

        if (msg != EQUIP_ERR_OK)
        {
            player->SendEquipError(msg, pDstItem, item);
            return;
        }

        // now do moves, remove...
        player->RemoveItem(dstbag, dstslot, true, true);

        // add to dest
        player->EquipItem(dest, item, true);

        // add to src
        player->StoreItem(sSrc, pDstItem, true);

        player->AutoUnequipOffhandIfNeed();

        // Xinef: Call this here after all needed items are equipped
        player->RemoveItemDependentAurasAndCasts((Item*) nullptr);
    }
}

void sTemplateNPC::CreateItemAndGive(Player* player, uint32 entry, uint32 e1, uint32 e2, uint32 e3, uint32 e4, uint32 e5, uint32 e6)
{
    Item* newItem = new Item;
    if (!newItem->Create(sObjectMgr->GetGenerator<HighGuid::Item>().Generate(), entry, player))
    {
        delete newItem;
        return;
    }

    ApplyBonus(player, newItem, PERM_ENCHANTMENT_SLOT, e1);
    ApplyBonus(player, newItem, SOCK_ENCHANTMENT_SLOT, e2);
    ApplyBonus(player, newItem, SOCK_ENCHANTMENT_SLOT_2, e3);
    ApplyBonus(player, newItem, SOCK_ENCHANTMENT_SLOT_3, e4);
    ApplyBonus(player, newItem, BONUS_ENCHANTMENT_SLOT, e5);
    ApplyBonus(player, newItem, PRISMATIC_ENCHANTMENT_SLOT, e6);

    sTemplateNpcMgr->GiveAndEquipItem(player, newItem);
}

void sTemplateNPC::CopyItemAndGive(Player* player, Item* original)
{
    sTemplateNpcMgr->CreateItemAndGive(player, original->GetEntry(),
        original->GetEnchantmentId(PERM_ENCHANTMENT_SLOT),
        original->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT),
        original->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2),
        original->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3),
        original->GetEnchantmentId(BONUS_ENCHANTMENT_SLOT),
        original->GetEnchantmentId(PRISMATIC_ENCHANTMENT_SLOT));
}

void sTemplateNPC::Copy(Player* target, Player* src)
{

    target->resetTalents(true);

    // copy talents from both specs only learn spells from the active one
    target->ActivateSpec(src->GetActiveSpec());
    const PlayerTalentMap& talentMap = src->GetTalentMap();
    for (PlayerTalentMap::const_iterator itr = talentMap.begin(); itr != talentMap.end(); ++itr)
    {
        if (itr->second->State == PLAYERSPELL_REMOVED)
            continue;

        // xinef: talent not in new spec
        if (!(itr->second->specMask & src->GetActiveSpecMask()))
            continue;

        if (itr->second->IsInSpec(src->GetActiveSpec()))
        {
            target->addTalent(itr->first, src->GetActiveSpecMask(), 0);
            if (itr->second->inSpellBook)
            {
                target->learnSpellHighRank(itr->first);
            }
        }
    }
    target->SyncSpentTalents();
    target->SetFreeTalentPoints(0);
    target->SendTalentsInfoData(false);

    RemoveAllGlyphs(target);

    // copy glyphs
    for (uint8 slot = 0; slot < MAX_GLYPH_SLOT_INDEX; ++slot) ApplyGlyph(target, slot, src->GetGlyph(slot));


    if (sTemplateNpcMgr->UnequipAllGear(target))
    {
        // copy gear
        // empty slots will be ignored
        for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
        {
            Item* equippedItem = src->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
            if (!equippedItem)
                continue;
            sTemplateNpcMgr->CopyItemAndGive(target, equippedItem);
        }
    }
}

void sTemplateNPC::LearnTemplateTalents(Player* player, std::string& playerSpecStr)
{
    for (TalentContainer::const_iterator itr = m_TalentContainer.begin(); itr != m_TalentContainer.end(); ++itr)
    {
        if ((*itr)->playerClass == GetClassString(player).c_str() && (*itr)->playerSpec == playerSpecStr)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo((*itr)->talentId);
            if (!SpellMgr::CheckSpellValid(spellInfo, (*itr)->talentId, true))
                continue;

            TalentSpellPos const* talentPos = GetTalentSpellPos((*itr)->talentId);
            if (!talentPos)
                continue;

            TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentPos->talent_id);
            if (!talentInfo)
                continue;

            player->addTalent((*itr)->talentId, player->GetActiveSpecMask(), 0);

            bool inBook = talentInfo->addToSpellBook && !spellInfo->HasAttribute(SPELL_ATTR0_PASSIVE) && !spellInfo->HasEffect(SPELL_EFFECT_LEARN_SPELL);
            if (inBook)
            {
                player->learnSpellHighRank((*itr)->talentId);
            }  
        }
    }
    player->SyncSpentTalents();
    player->SetFreeTalentPoints(0);
    player->SendTalentsInfoData(false);
}

void sTemplateNPC::LearnTemplateGlyphs(Player* player, std::string& playerSpecStr)
{
    for (GlyphContainer::const_iterator itr = m_GlyphContainer.begin(); itr != m_GlyphContainer.end(); ++itr)
    {
        if ((*itr)->playerClass == GetClassString(player).c_str() && (*itr)->playerSpec == playerSpecStr)
            ApplyGlyph(player, (*itr)->slot, (*itr)->glyph);
    }
    player->SendTalentsInfoData(false);
}

void sTemplateNPC::EquipTemplateGear(Player* player, std::string& playerSpecStr)
{
    if (sTemplateNpcMgr->UnequipAllGear(player))
    {
        if (player->getRace() == RACE_HUMAN)
        {
            // reverse sort so we equip items from trinket to helm so we avoid issue with meta gems
            std::sort(m_HumanGearContainer.begin(), m_HumanGearContainer.end(), std::greater<HumanGearTemplate*>());

            for (HumanGearContainer::const_iterator itr = m_HumanGearContainer.begin(); itr != m_HumanGearContainer.end(); ++itr)
            {
                if ((*itr)->playerClass == GetClassString(player).c_str() && (*itr)->playerSpec == playerSpecStr)
                {
                    sTemplateNpcMgr->CreateItemAndGive(player, (*itr)->itemEntry, (*itr)->enchant, (*itr)->socket1, (*itr)->socket2, (*itr)->socket3, (*itr)->bonusEnchant, (*itr)->prismaticEnchant);
                }
            }
        }
        else if (player->GetTeamId() == TEAM_ALLIANCE && player->getRace() != RACE_HUMAN)
        {
            // reverse sort so we equip items from trinket to helm so we avoid issue with meta gems
            std::sort(m_AllianceGearContainer.begin(), m_AllianceGearContainer.end(), std::greater<AllianceGearTemplate*>());

            for (AllianceGearContainer::const_iterator itr = m_AllianceGearContainer.begin(); itr != m_AllianceGearContainer.end(); ++itr)
            {
                if ((*itr)->playerClass == GetClassString(player).c_str() && (*itr)->playerSpec == playerSpecStr)
                {
                    sTemplateNpcMgr->CreateItemAndGive(player, (*itr)->itemEntry, (*itr)->enchant, (*itr)->socket1, (*itr)->socket2, (*itr)->socket3, (*itr)->bonusEnchant, (*itr)->prismaticEnchant);
                }
            }
        }
        else if (player->GetTeamId() == TEAM_HORDE)
        {
            // reverse sort so we equip items from trinket to helm so we avoid issue with meta gems
            std::sort(m_HordeGearContainer.begin(), m_HordeGearContainer.end(), std::greater<HordeGearTemplate*>());

            for (HordeGearContainer::const_iterator itr = m_HordeGearContainer.begin(); itr != m_HordeGearContainer.end(); ++itr)
            {
                if ((*itr)->playerClass == GetClassString(player).c_str() && (*itr)->playerSpec == playerSpecStr)
                {
                    sTemplateNpcMgr->CreateItemAndGive(player, (*itr)->itemEntry, (*itr)->enchant, (*itr)->socket1, (*itr)->socket2, (*itr)->socket3, (*itr)->bonusEnchant, (*itr)->prismaticEnchant);
                }
            }
        }
    }
}

void sTemplateNPC::LoadTalentsContainer()
{
    for (TalentContainer::const_iterator itr = m_TalentContainer.begin(); itr != m_TalentContainer.end(); ++itr)
        delete *itr;

    m_TalentContainer.clear();

    uint32 oldMSTime = getMSTime();
    uint32 count = 0;

    QueryResult result = CharacterDatabase.Query("SELECT playerClass, playerSpec, talentId FROM template_npc_talents;");

    if (!result)
    {
        sLog->outString(">>TEMPLATE NPC: Loaded 0 talent templates. DB table `template_npc_talents` is empty!");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        TalentTemplate* pTalent = new TalentTemplate;

        pTalent->playerClass = fields[0].GetString();
        pTalent->playerSpec = fields[1].GetString();
        pTalent->talentId = fields[2].GetUInt32();

        m_TalentContainer.push_back(pTalent);
        ++count;
    } while (result->NextRow());
    sLog->outString(">>TEMPLATE NPC: Loaded %u talent templates in %u ms.", count, GetMSTimeDiffToNow(oldMSTime));
}

void sTemplateNPC::LoadGlyphsContainer()
{
    for (GlyphContainer::const_iterator itr = m_GlyphContainer.begin(); itr != m_GlyphContainer.end(); ++itr)
        delete *itr;

    m_GlyphContainer.clear();

    QueryResult result = CharacterDatabase.Query("SELECT playerClass, playerSpec, slot, glyph FROM template_npc_glyphs;");

    uint32 oldMSTime = getMSTime();
    uint32 count = 0;

    if (!result)
    {
        sLog->outString(">>TEMPLATE NPC: Loaded 0 glyph templates. DB table `template_npc_glyphs` is empty!");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        GlyphTemplate* pGlyph = new GlyphTemplate;

        pGlyph->playerClass = fields[0].GetString();
        pGlyph->playerSpec = fields[1].GetString();
        pGlyph->slot = fields[2].GetUInt8();
        pGlyph->glyph = fields[3].GetUInt32();

        m_GlyphContainer.push_back(pGlyph);
        ++count;
    } while (result->NextRow());


    sLog->outString(">>TEMPLATE NPC: Loaded %u glyph templates in %u ms.", count, GetMSTimeDiffToNow(oldMSTime));
}

void sTemplateNPC::LoadHumanGearContainer()
{
    for (HumanGearContainer::const_iterator itr = m_HumanGearContainer.begin(); itr != m_HumanGearContainer.end(); ++itr)
        delete *itr;

    m_HumanGearContainer.clear();

    QueryResult result = CharacterDatabase.Query("SELECT playerClass, playerSpec, pos, itemEntry, enchant, socket1, socket2, socket3, bonusEnchant, prismaticEnchant FROM template_npc_human;");

    uint32 oldMSTime = getMSTime();
    uint32 count = 0;

    if (!result)
    {
        sLog->outString(">>TEMPLATE NPC: Loaded 0 'gear templates. DB table `template_npc_human` is empty!");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        HumanGearTemplate* pItem = new HumanGearTemplate;

        pItem->playerClass = fields[0].GetString();
        pItem->playerSpec = fields[1].GetString();
        pItem->pos = fields[2].GetUInt8();
        pItem->itemEntry = fields[3].GetUInt32();
        pItem->enchant = fields[4].GetUInt32();
        pItem->socket1 = fields[5].GetUInt32();
        pItem->socket2 = fields[6].GetUInt32();
        pItem->socket3 = fields[7].GetUInt32();
        pItem->bonusEnchant = fields[8].GetUInt32();
        pItem->prismaticEnchant = fields[9].GetUInt32();

        m_HumanGearContainer.push_back(pItem);
        ++count;
    } while (result->NextRow());
    sLog->outString(">>TEMPLATE NPC: Loaded %u gear templates for Humans in %u ms.", count, GetMSTimeDiffToNow(oldMSTime));
}

void sTemplateNPC::LoadAllianceGearContainer()
{
    for (AllianceGearContainer::const_iterator itr = m_AllianceGearContainer.begin(); itr != m_AllianceGearContainer.end(); ++itr)
        delete *itr;

    m_AllianceGearContainer.clear();

    QueryResult result = CharacterDatabase.Query("SELECT playerClass, playerSpec, pos, itemEntry, enchant, socket1, socket2, socket3, bonusEnchant, prismaticEnchant FROM template_npc_alliance;");

    uint32 oldMSTime = getMSTime();
    uint32 count = 0;

    if (!result)
    {
        sLog->outString(">>TEMPLATE NPC: Loaded 0 'gear templates. DB table `template_npc_alliance` is empty!");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        AllianceGearTemplate* pItem = new AllianceGearTemplate;

        pItem->playerClass = fields[0].GetString();
        pItem->playerSpec = fields[1].GetString();
        pItem->pos = fields[2].GetUInt8();
        pItem->itemEntry = fields[3].GetUInt32();
        pItem->enchant = fields[4].GetUInt32();
        pItem->socket1 = fields[5].GetUInt32();
        pItem->socket2 = fields[6].GetUInt32();
        pItem->socket3 = fields[7].GetUInt32();
        pItem->bonusEnchant = fields[8].GetUInt32();
        pItem->prismaticEnchant = fields[9].GetUInt32();

        m_AllianceGearContainer.push_back(pItem);
        ++count;
    } while (result->NextRow());
    sLog->outString(">>TEMPLATE NPC: Loaded %u gear templates for Alliances in %u ms.", count, GetMSTimeDiffToNow(oldMSTime));
}

void sTemplateNPC::LoadHordeGearContainer()
{
    for (HordeGearContainer::const_iterator itr = m_HordeGearContainer.begin(); itr != m_HordeGearContainer.end(); ++itr)
        delete *itr;

    m_HordeGearContainer.clear();

    QueryResult result = CharacterDatabase.Query("SELECT playerClass, playerSpec, pos, itemEntry, enchant, socket1, socket2, socket3, bonusEnchant, prismaticEnchant FROM template_npc_horde;");

    uint32 oldMSTime = getMSTime();
    uint32 count = 0;

    if (!result)
    {
        sLog->outString(">>TEMPLATE NPC: Loaded 0 'gear templates. DB table `template_npc_horde` is empty!");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        HordeGearTemplate* pItem = new HordeGearTemplate;

        pItem->playerClass = fields[0].GetString();
        pItem->playerSpec = fields[1].GetString();
        pItem->pos = fields[2].GetUInt8();
        pItem->itemEntry = fields[3].GetUInt32();
        pItem->enchant = fields[4].GetUInt32();
        pItem->socket1 = fields[5].GetUInt32();
        pItem->socket2 = fields[6].GetUInt32();
        pItem->socket3 = fields[7].GetUInt32();
        pItem->bonusEnchant = fields[8].GetUInt32();
        pItem->prismaticEnchant = fields[9].GetUInt32();

        m_HordeGearContainer.push_back(pItem);
        ++count;
    } while (result->NextRow());
    sLog->outString(">>TEMPLATE NPC: Loaded %u gear templates for Hordes in %u ms.", count, GetMSTimeDiffToNow(oldMSTime));
}

std::string sTemplateNPC::GetClassString(Player* player)
{
    switch (player->getClass())
    {
    case CLASS_PRIEST:       return "Priest";      break;
    case CLASS_PALADIN:      return "Paladin";     break;
    case CLASS_WARRIOR:      return "Warrior";     break;
    case CLASS_MAGE:         return "Mage";        break;
    case CLASS_WARLOCK:      return "Warlock";     break;
    case CLASS_SHAMAN:       return "Shaman";      break;
    case CLASS_DRUID:        return "Druid";       break;
    case CLASS_HUNTER:       return "Hunter";      break;
    case CLASS_ROGUE:        return "Rogue";       break;
    case CLASS_DEATH_KNIGHT: return "DeathKnight"; break;
    default:
        break;
    }
    return "Unknown"; // Fix warning, this should never happen
}

bool sTemplateNPC::OverwriteTemplate(Player* player, std::string& playerSpecStr)
{
    // Delete old talent and glyph templates before extracting new ones
    CharacterDatabase.PExecute("DELETE FROM template_npc_talents WHERE playerClass = '%s' AND playerSpec = '%s';", GetClassString(player).c_str(), playerSpecStr.c_str());
    CharacterDatabase.PExecute("DELETE FROM template_npc_glyphs WHERE playerClass = '%s' AND playerSpec = '%s';", GetClassString(player).c_str(), playerSpecStr.c_str());

    // Delete old gear templates before extracting new ones
    if (player->getRace() == RACE_HUMAN)
    {
        CharacterDatabase.PExecute("DELETE FROM template_npc_human WHERE playerClass = '%s' AND playerSpec = '%s';", GetClassString(player).c_str(), playerSpecStr.c_str());
        player->GetSession()->SendAreaTriggerMessage("Template successfuly created!");
        return false;
    }
    else if (player->GetTeamId() == TEAM_ALLIANCE && player->getRace() != RACE_HUMAN)
    {
        CharacterDatabase.PExecute("DELETE FROM template_npc_alliance WHERE playerClass = '%s' AND playerSpec = '%s';", GetClassString(player).c_str(), playerSpecStr.c_str());
        player->GetSession()->SendAreaTriggerMessage("Template successfuly created!");
        return false;
    }
    else if (player->GetTeamId() == TEAM_HORDE)
    {                                                                                                        // ????????????? sTemplateNpcMgr here??
        CharacterDatabase.PExecute("DELETE FROM template_npc_horde WHERE playerClass = '%s' AND playerSpec = '%s';", GetClassString(player).c_str(), playerSpecStr.c_str());
        player->GetSession()->SendAreaTriggerMessage("Template successfuly created!");
        return false;
    }
    return true;
}

void sTemplateNPC::ExtractGearTemplateToDB(Player* player, std::string& playerSpecStr)
{
    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        Item* equippedItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);

        if (equippedItem)
        {
            if (player->getRace() == RACE_HUMAN)
            {
                CharacterDatabase.PExecute("INSERT INTO template_npc_human (`playerClass`, `playerSpec`, `pos`, `itemEntry`, `enchant`, `socket1`, `socket2`, `socket3`, `bonusEnchant`, `prismaticEnchant`) VALUES ('%s', '%s', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u');"
                    , GetClassString(player).c_str(), playerSpecStr.c_str(), equippedItem->GetSlot(), equippedItem->GetEntry(), equippedItem->GetEnchantmentId(PERM_ENCHANTMENT_SLOT),
                    equippedItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT), equippedItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2), equippedItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3),
                    equippedItem->GetEnchantmentId(BONUS_ENCHANTMENT_SLOT), equippedItem->GetEnchantmentId(PRISMATIC_ENCHANTMENT_SLOT));
            }
            else if (player->GetTeamId() == TEAM_ALLIANCE && player->getRace() != RACE_HUMAN)
            {
                CharacterDatabase.PExecute("INSERT INTO template_npc_alliance (`playerClass`, `playerSpec`, `pos`, `itemEntry`, `enchant`, `socket1`, `socket2`, `socket3`, `bonusEnchant`, `prismaticEnchant`) VALUES ('%s', '%s', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u');"
                    , GetClassString(player).c_str(), playerSpecStr.c_str(), equippedItem->GetSlot(), equippedItem->GetEntry(), equippedItem->GetEnchantmentId(PERM_ENCHANTMENT_SLOT),
                    equippedItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT), equippedItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2), equippedItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3),
                    equippedItem->GetEnchantmentId(BONUS_ENCHANTMENT_SLOT), equippedItem->GetEnchantmentId(PRISMATIC_ENCHANTMENT_SLOT));
            }
            else if (player->GetTeamId() == TEAM_HORDE)
            {
                CharacterDatabase.PExecute("INSERT INTO template_npc_horde (`playerClass`, `playerSpec`, `pos`, `itemEntry`, `enchant`, `socket1`, `socket2`, `socket3`, `bonusEnchant`, `prismaticEnchant`) VALUES ('%s', '%s', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u');"
                    , GetClassString(player).c_str(), playerSpecStr.c_str(), equippedItem->GetSlot(), equippedItem->GetEntry(), equippedItem->GetEnchantmentId(PERM_ENCHANTMENT_SLOT),
                    equippedItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT), equippedItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2), equippedItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3),
                    equippedItem->GetEnchantmentId(BONUS_ENCHANTMENT_SLOT), equippedItem->GetEnchantmentId(PRISMATIC_ENCHANTMENT_SLOT));
            }
        }
    }
}

void sTemplateNPC::ExtractTalentTemplateToDB(Player* player, std::string& playerSpecStr)
{
    QueryResult result = CharacterDatabase.PQuery("SELECT spell FROM character_talent WHERE guid = '%u' "
        "AND specMask = '%u';", (player->GetGUID()).GetCounter(), player->GetActiveSpecMask());

    if (!result)
    {
        return;
    }
    else if (player->GetFreeTalentPoints() > 0)
    {
        player->GetSession()->SendAreaTriggerMessage("You have unspend talent points. Please spend all your talent points and re-extract the template.");
        return;
    }
    else
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 spell = fields[0].GetUInt32();

            CharacterDatabase.PExecute("INSERT INTO template_npc_talents (playerClass, playerSpec, talentId) "
                "VALUES ('%s', '%s', '%u');", GetClassString(player).c_str(), playerSpecStr.c_str(), spell);
        } while (result->NextRow());
    }
}

void sTemplateNPC::ExtractGlyphsTemplateToDB(Player* player, std::string& playerSpecStr)
{
    QueryResult result = CharacterDatabase.PQuery("SELECT glyph1, glyph2, glyph3, glyph4, glyph5, glyph6 "
        "FROM character_glyphs WHERE guid = '%u' AND talentGroup = '%u';", player->GetGUID().GetCounter(), player->GetActiveSpec());

    for (uint8 slot = 0; slot < MAX_GLYPH_SLOT_INDEX; ++slot)
    {
        if (!result)
        {
            player->GetSession()->SendAreaTriggerMessage("Get glyphs and re-extract the template!");
            continue;
        }

        Field* fields = result->Fetch();
        uint32 glyph1 = fields[0].GetUInt32();
        uint32 glyph2 = fields[1].GetUInt32();
        uint32 glyph3 = fields[2].GetUInt32();
        uint32 glyph4 = fields[3].GetUInt32();
        uint32 glyph5 = fields[4].GetUInt32();
        uint32 glyph6 = fields[5].GetUInt32();

        uint32 storedGlyph;

        switch (slot)
        {
        case 0:
            storedGlyph = glyph1;
            break;
        case 1:
            storedGlyph = glyph2;
            break;
        case 2:
            storedGlyph = glyph3;
            break;
        case 3:
            storedGlyph = glyph4;
            break;
        case 4:
            storedGlyph = glyph5;
            break;
        case 5:
            storedGlyph = glyph6;
            break;
        default:
            break;
        }

        CharacterDatabase.PExecute("INSERT INTO template_npc_glyphs (playerClass, playerSpec, slot, glyph) "
            "VALUES ('%s', '%s', '%u', '%u');", GetClassString(player).c_str(), playerSpecStr.c_str(), slot, storedGlyph);
    }
}

bool sTemplateNPC::CanEquipTemplate(Player* player, std::string& playerSpecStr)
{
    if (player->getRace() == RACE_HUMAN)
    {
        QueryResult result = CharacterDatabase.PQuery("SELECT playerClass, playerSpec FROM template_npc_human "
            "WHERE playerClass = '%s' AND playerSpec = '%s';", GetClassString(player).c_str(), playerSpecStr.c_str());

        if (!result)
            return false;
    }
    else if (player->GetTeamId() == TEAM_ALLIANCE && player->getRace() != RACE_HUMAN)
    {
        QueryResult result = CharacterDatabase.PQuery("SELECT playerClass, playerSpec FROM template_npc_alliance "
            "WHERE playerClass = '%s' AND playerSpec = '%s';", GetClassString(player).c_str(), playerSpecStr.c_str());

        if (!result)
            return false;
    }
    else if (player->GetTeamId() == TEAM_HORDE)
    {
        QueryResult result = CharacterDatabase.PQuery("SELECT playerClass, playerSpec FROM template_npc_horde "
            "WHERE playerClass = '%s' AND playerSpec = '%s';", GetClassString(player).c_str(), playerSpecStr.c_str());

        if (!result)
            return false;
    }
    return true;
}

class TemplateNPC : public CreatureScript
{
public:
    TemplateNPC() : CreatureScript("TemplateNPC") { }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Preset Gear, Talents, and Glyphs", GOSSIP_SENDER_MAIN, 200);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Preset Talents Only", GOSSIP_SENDER_MAIN, 201);

            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Remove All Gear", GOSSIP_SENDER_MAIN, 32);
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Remove All Glyphs", GOSSIP_SENDER_MAIN, 30);
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Reset Talents", GOSSIP_SENDER_MAIN, 31);
			SendGossipMenuFor(player, 55002, creature->GetGUID());
            return true;
        }

        static void EquipFullTemplateGear(Player* player, std::string& playerSpecStr) // Merge
        {
            if (sTemplateNpcMgr->CanEquipTemplate(player, playerSpecStr) == false)
            {
                player->GetSession()->SendAreaTriggerMessage("There's no templates for %s ialization yet.", playerSpecStr.c_str());
                return;
            }

            if (player->resetTalents(true))
            {
                player->SendTalentsInfoData(false);
                player->GetSession()->SendAreaTriggerMessage("Your talents have been reset.");
            }

            // Don't let players to Template feature after spending some talent points
            if (player->GetFreeTalentPoints() < 71)
            {
                player->GetSession()->SendAreaTriggerMessage("You have already spent some talent points. You need to reset your talents first!");
                CloseGossipMenuFor(player);
                return;
            }

            sTemplateNpcMgr->LearnTemplateTalents(player, playerSpecStr);
            sTemplateNpcMgr->LearnTemplateGlyphs(player, playerSpecStr);
            sTemplateNpcMgr->EquipTemplateGear(player, playerSpecStr);
            sTemplateNpcMgr->LearnPlateMailSpells(player);

            // update warr talent
            player->UpdateTitansGrip();

            LearnWeaponSkills(player);

            player->GetSession()->SendAreaTriggerMessage("Successfuly equipped %s %s template!", playerSpecStr.c_str(), sTemplateNpcMgr->GetClassString(player).c_str());

            if (player->getPowerType() == POWER_MANA)
                player->SetPower(POWER_MANA, player->GetMaxPower(POWER_MANA));

            player->SetHealth(player->GetMaxHealth());

            // Learn Riding/Flying
            if (player->HasSpell(SPELL_Artisan_Riding) ||
                player->HasSpell(SPELL_Cold_Weather_Flying) ||
                player->HasSpell(SPELL_Amani_War_Bear) ||
                player->HasSpell(SPELL_Teach_Learn_Talent_Specialization_Switches)
                || player->HasSpell(SPELL_Learn_a_Second_Talent_Specialization)
                )
                return;

            // Cast spells that teach dual 
            // Both are also ImplicitTarget self and must be cast by player
            player->CastSpell(player, SPELL_Teach_Learn_Talent_Specialization_Switches, player->GetGUID());
            player->CastSpell(player, SPELL_Learn_a_Second_Talent_Specialization, player->GetGUID());

            player->learnSpell(SPELL_Artisan_Riding);
            player->learnSpell(SPELL_Cold_Weather_Flying);
            player->learnSpell(SPELL_Amani_War_Bear);

        }

        static void LearnOnlyTalentsAndGlyphs(Player* player, std::string& playerSpecStr) // Merge
        {
            if (sTemplateNpcMgr->CanEquipTemplate(player, playerSpecStr) == false)
            {
                player->GetSession()->SendAreaTriggerMessage("There's no templates for %s ialization yet.", playerSpecStr.c_str());
                return;
            }

            if (player->resetTalents(true))
            {
                player->SendTalentsInfoData(false);
                player->GetSession()->SendAreaTriggerMessage("Your talents have been reset.");
            }

            // Don't let players to Template feature after spending some talent points
            if (player->GetFreeTalentPoints() < 71)
            {
                player->GetSession()->SendAreaTriggerMessage("You have already spent some talent points. You need to reset your talents first!");
                CloseGossipMenuFor(player);
                return;
            }

            sTemplateNpcMgr->LearnTemplateTalents(player, playerSpecStr);
            sTemplateNpcMgr->LearnTemplateGlyphs(player, playerSpecStr);
            //sTemplateNpcMgr->EquipTemplateGear(player);
            sTemplateNpcMgr->LearnPlateMailSpells(player);

            LearnWeaponSkills(player);

            player->GetSession()->SendAreaTriggerMessage("Successfuly learned talent  %s!", playerSpecStr.c_str());

            // Learn Riding/Flying
            if (player->HasSpell(SPELL_Artisan_Riding) ||
                player->HasSpell(SPELL_Cold_Weather_Flying) ||
                player->HasSpell(SPELL_Amani_War_Bear) ||
                player->HasSpell(SPELL_Teach_Learn_Talent_Specialization_Switches)
               || player->HasSpell(SPELL_Learn_a_Second_Talent_Specialization)
                )
                return;

            // Cast spells that teach dual 
            // Both are also ImplicitTarget self and must be cast by player
            player->CastSpell(player, SPELL_Teach_Learn_Talent_Specialization_Switches, player->GetGUID());
            player->CastSpell(player, SPELL_Learn_a_Second_Talent_Specialization, player->GetGUID());

            player->learnSpell(SPELL_Artisan_Riding);
            player->learnSpell(SPELL_Cold_Weather_Flying);
            player->learnSpell(SPELL_Amani_War_Bear);
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction)
        {
            player->PlayerTalkClass->ClearMenus();

            if (!player || !creature)
                return false;
            std::string playerSpec;
            switch (uiAction)
            {
            case 0: // Discipline Priest 
                playerSpec = "Discipline";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 1: // Holy Priest 
                playerSpec = "Holy";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 2: // Shadow Priest 
                playerSpec = "Shadow";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 3: // Holy Paladin 
                playerSpec = "Holy";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 4: // Protection Paladin 
                playerSpec = "Protection";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 5: // Retribution Paladin 
                playerSpec = "Retribution";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 6: // Fury Warrior 
                playerSpec = "Fury";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 7: // Arms Warrior 
                playerSpec = "Arms";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 8: // Protection Warrior 
                playerSpec = "Protection";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 9: // Arcane Mage 
                playerSpec = "Arcane";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 10: // Fire Mage 
                playerSpec = "Fire";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 11: // Frost Mage 
                playerSpec = "Frost";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 12: // Affliction Warlock 
                playerSpec = "Affliction";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 13: // Demonology Warlock 
                playerSpec = "Demonology";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 14: // Destruction Warlock 
                playerSpec = "Destruction";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 15: // Elemental Shaman 
                playerSpec = "Elemental";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 16: // Enhancement Shaman 
                playerSpec = "Enhancement";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 17: // Restoration Shaman 
                playerSpec = "Restoration";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 18: // Ballance Druid 
                playerSpec = "Ballance";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 19: // Feral Druid 
                playerSpec = "Feral";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 20: // Restoration Druid 
                playerSpec = "Restoration";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 21: // Marksmanship Hunter 
                playerSpec = "Marksmanship";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 22: // Beastmastery Hunter 
                playerSpec = "Beastmastery";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 23: // Survival Hunter 
                playerSpec = "Survival";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 24: // Assassination Rogue 
                playerSpec = "Assassination";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 25: // Combat Rogue 
                playerSpec = "Combat";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 26: // Subtlety Rogue 
                playerSpec = "Subtlety";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 27: // Blood DK 
                playerSpec = "Blood";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 28: // Frost DK 
                playerSpec = "Frost";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 29: // Unholy DK 
                playerSpec = "Unholy";
                EquipFullTemplateGear(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 30:
                sTemplateNpcMgr->RemoveAllGlyphs(player);
                player->GetSession()->SendAreaTriggerMessage("Your glyphs have been removed.");
                //GossipHello(player);
                CloseGossipMenuFor(player);
                break;

            case 31:
                if (player->resetTalents(true))
                {
                    player->SendTalentsInfoData(false);
                    player->GetSession()->SendAreaTriggerMessage("Your talents have been reset.");
                }
                CloseGossipMenuFor(player);
                break;

            case 32:
                sTemplateNpcMgr->RemoveAllGear(player);
                CloseGossipMenuFor(player);
                break;

                //Priest
            case 100:
                playerSpec = "Discipline";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 101:
                playerSpec = "Holy";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 102:
                playerSpec = "Shadow";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

                //Paladin
            case 103:
                playerSpec = "Holy";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 104:
                playerSpec = "Protection";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 105:
                playerSpec = "Retribution";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

                //Warrior
            case 106:
                playerSpec = "Fury";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 107:
                playerSpec = "Arms";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 108:
                playerSpec = "Protection";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

                //Mage
            case 109:
                playerSpec = "Arcane";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 110:
                playerSpec = "Fire";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 111:
                playerSpec = "Frost";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

                //Warlock
            case 112:
                playerSpec = "Affliction";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 113:
                playerSpec = "Demonology";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 114:
                playerSpec = "Destruction";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

                //Shaman
            case 115:
                playerSpec = "Elemental";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 116:
                playerSpec = "Enhancement";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 117:
                playerSpec = "Restoration";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

                //Druid
            case 118:
                playerSpec = "Ballance";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 119:
                playerSpec = "Feral";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 120:
                playerSpec = "Restoration";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

                //Hunter
            case 121:
                playerSpec = "Marksmanship";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 122:
                playerSpec = "Beastmastery";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 123:
                playerSpec = "Survival";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

                //Rogue
            case 124:
                playerSpec = "Assasination";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 125:
                playerSpec = "Combat";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 126:
                playerSpec = "Subtlety";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

                //DK
            case 127:
                playerSpec = "Blood";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 128:
                playerSpec = "Frost";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 129:
                playerSpec = "Unholy";
                LearnOnlyTalentsAndGlyphs(player, playerSpec);
                CloseGossipMenuFor(player);
                break;

            case 200:
                switch (player->getClass())
                {
                case CLASS_PRIEST:
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_holy_wordfortitude:30|t|r Discipline ", GOSSIP_SENDER_MAIN, 0);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_holy_holybolt:30|t|r Holy ", GOSSIP_SENDER_MAIN, 1);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_shadow_shadowwordpain:30|t|r Shadow ", GOSSIP_SENDER_MAIN, 2);
                    break;
                case CLASS_PALADIN:
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_holy_holybolt:30|t|r Holy ", GOSSIP_SENDER_MAIN, 3);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_holy_devotionaura:30|t|r Protection ", GOSSIP_SENDER_MAIN, 4);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_holy_auraoflight:30|t|r Retribution ", GOSSIP_SENDER_MAIN, 5);
                    break;
                case CLASS_WARRIOR:
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\ability_warrior_innerrage:30|t|r Fury ", GOSSIP_SENDER_MAIN, 6);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\ability_rogue_eviscerate:30|t|r Arms ", GOSSIP_SENDER_MAIN, 7);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\ability_warrior_defensivestance:30|t|r Protection ", GOSSIP_SENDER_MAIN, 8);
                    break;
                case CLASS_MAGE:
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_holy_magicalsentry:30|t|r Arcane ", GOSSIP_SENDER_MAIN, 9);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_fire_flamebolt:30|t|r Fire ", GOSSIP_SENDER_MAIN, 10);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_frost_frostbolt02:30|t|r Frost ", GOSSIP_SENDER_MAIN, 11);
                    break;
                case CLASS_WARLOCK:
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_shadow_deathcoil:30|t|r Affliction ", GOSSIP_SENDER_MAIN, 12);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_shadow_metamorphosis:30|t|r Demonology ", GOSSIP_SENDER_MAIN, 13);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_shadow_rainoffire:30|t|r Destruction ", GOSSIP_SENDER_MAIN, 14);
                    break;
                case CLASS_SHAMAN:
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_nature_lightning:30|t|r Elemental ", GOSSIP_SENDER_MAIN, 15);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_nature_lightningshield:30|t|r Enhancement ", GOSSIP_SENDER_MAIN, 16);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_nature_magicimmunity:30|t|r Restoration ", GOSSIP_SENDER_MAIN, 17);
                    break;
                case CLASS_DRUID:
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_nature_starfall:30|t|r Balance ", GOSSIP_SENDER_MAIN, 18);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\ability_racial_bearform:30|t|r Feral ", GOSSIP_SENDER_MAIN, 19);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_nature_healingtouch:30|t|r Restoration ", GOSSIP_SENDER_MAIN, 20);
                    break;
                case CLASS_HUNTER:
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\ability_marksmanship:30|t|r Marksmanship ", GOSSIP_SENDER_MAIN, 21);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\ability_hunter_beasttaming:30|t|r Beastmastery ", GOSSIP_SENDER_MAIN, 22);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\ability_Hunter_swiftstrike:30|t|r Survival ", GOSSIP_SENDER_MAIN, 23);
                    break;
                case CLASS_ROGUE:
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\ability_rogue_eviscerate:30|t|r Assasination ", GOSSIP_SENDER_MAIN, 24);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\ability_backstab:30|t|r Combat ", GOSSIP_SENDER_MAIN, 25);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\ability_stealth:30|t|r Subtlety ", GOSSIP_SENDER_MAIN, 26);
                    break;
                case CLASS_DEATH_KNIGHT:
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_deathknight_bloodpresence:30|t|r Blood ", GOSSIP_SENDER_MAIN, 27);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_deathknight_frostpresence:30|t|r Frost ", GOSSIP_SENDER_MAIN, 28);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_deathknight_unholypresence:30|t|r Unholy ", GOSSIP_SENDER_MAIN, 29);
                    break;
                }
                AddGossipItemFor(player, GOSSIP_ICON_DOT, "Back to Main Menu", GOSSIP_SENDER_MAIN, 5000);
                SendGossipMenuFor(player, 55002, creature->GetGUID());
                break;
            case 201:
                switch (player->getClass())
                {
                case CLASS_PRIEST:
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_holy_wordfortitude:30|t|r Discipline (Talents Only)", GOSSIP_SENDER_MAIN, 100);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_holy_holybolt:30|t|r Holy (Talents only)", GOSSIP_SENDER_MAIN, 101);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_shadow_shadowwordpain:30|t|r Shadow (Talents only)", GOSSIP_SENDER_MAIN, 102);
                    break;
                case CLASS_PALADIN:
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_holy_holybolt:30|t|r Holy (Talents Only)", GOSSIP_SENDER_MAIN, 103);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_holy_devotionaura:30|t|r Protection  (Talents Only)", GOSSIP_SENDER_MAIN, 104);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_holy_auraoflight:30|t|r Retribution  (Talents Only)", GOSSIP_SENDER_MAIN, 105);
                    break;
                case CLASS_WARRIOR:
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\ability_warrior_innerrage:30|t|r Fury  (Talents Only)", GOSSIP_SENDER_MAIN, 106);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\ability_rogue_eviscerate:30|t|r Arms  (Talents Only)", GOSSIP_SENDER_MAIN, 107);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\ability_warrior_defensivestance:30|t|r Protection  (Talents Only)", GOSSIP_SENDER_MAIN, 108);
                    break;
                case CLASS_MAGE:
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_holy_magicalsentry:30|t|r Arcane  (Talents Only)", GOSSIP_SENDER_MAIN, 109);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_fire_flamebolt:30|t|r Fire  (Talents Only)", GOSSIP_SENDER_MAIN, 110);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_frost_frostbolt02:30|t|r Frost  (Talents Only)", GOSSIP_SENDER_MAIN, 111);
                    break;
                case CLASS_WARLOCK:
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_shadow_deathcoil:30|t|r Affliction  (Talents Only)", GOSSIP_SENDER_MAIN, 112);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_shadow_metamorphosis:30|t|r Demonology  (Talents Only)", GOSSIP_SENDER_MAIN, 113);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_shadow_rainoffire:30|t|r Destruction  (Talents Only)", GOSSIP_SENDER_MAIN, 114);
                    break;
                case CLASS_SHAMAN:
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_nature_lightning:30|t|r Elemental  (Talents Only)", GOSSIP_SENDER_MAIN, 115);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_nature_lightningshield:30|t|r Enhancement  (Talents Only)", GOSSIP_SENDER_MAIN, 116);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_nature_magicimmunity:30|t|r Restoration  (Talents Only)", GOSSIP_SENDER_MAIN, 117);
                    break;
                case CLASS_DRUID:
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_nature_starfall:30|t|r Balance  (Talents Only)", GOSSIP_SENDER_MAIN, 118);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\ability_racial_bearform:30|t|r Feral  (Talents Only)", GOSSIP_SENDER_MAIN, 119);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_nature_healingtouch:30|t|r Restoration  (Talents Only)", GOSSIP_SENDER_MAIN, 120);
                    break;
                case CLASS_HUNTER:
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\ability_marksmanship:30|t|r Marksmanship  (Talents Only)", GOSSIP_SENDER_MAIN, 121);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\ability_hunter_beasttaming:30|t|r Beastmastery  (Talents Only)", GOSSIP_SENDER_MAIN, 122);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\ability_Hunter_swiftstrike:30|t|r Survival  (Talents Only)", GOSSIP_SENDER_MAIN, 123);
                    break;
                case CLASS_ROGUE:
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\ability_rogue_eviscerate:30|t|r Assasination  (Talents Only)", GOSSIP_SENDER_MAIN, 124);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\ability_backstab:30|t|r Combat  (Talents Only)", GOSSIP_SENDER_MAIN, 125);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\ability_stealth:30|t|r Subtlety  (Talents Only)", GOSSIP_SENDER_MAIN, 126);
                    break;
                case CLASS_DEATH_KNIGHT:
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_deathknight_bloodpresence:30|t|r Blood  (Talents Only)", GOSSIP_SENDER_MAIN, 127);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_deathknight_frostpresence:30|t|r Frost  (Talents Only)", GOSSIP_SENDER_MAIN, 128);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_deathknight_unholypresence:30|t|r Unholy  (Talents Only)", GOSSIP_SENDER_MAIN, 129);
                    break;
                }
                AddGossipItemFor(player, GOSSIP_ICON_DOT, "Back to Main Menu", GOSSIP_SENDER_MAIN, 5000);
                SendGossipMenuFor(player, 55002, creature->GetGUID());
                break;

            case 5000:
                // return to OnGossipHello menu, otherwise it will freeze every menu
                OnGossipHello(player, creature);
                break;

            default: // Just in case
                player->GetSession()->SendAreaTriggerMessage("Something went wrong in the code. Please contact the administrator.");
                break;
            }
            player->UpdateSkillsForLevel();

            return true;
        }
};

class TemplateNPC_command : public CommandScript
{
public:
    TemplateNPC_command() : CommandScript("TemplateNPC_command") {}

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> saveTable = {
            {"horde", SEC_ADMINISTRATOR, false, &HandleSaveGearHorde, "<spec>, example: `.template save horde Arms`"},
            {"alliance", SEC_ADMINISTRATOR, false, &HandleSaveGearAlly, "<spec>, example: `.template save alliance Arms`"},
            {"human", SEC_ADMINISTRATOR, false, &HandleSaveGearHuman, "<spec>, example: `.template save human Arms`"},
        };

        static std::vector<ChatCommand> TemplateNPCTable = {
            {"copy", SEC_ADMINISTRATOR, false, &HandleCopyCommand, "Copies your target's gear onto your character. example: `.template copy`"},
            {"save", SEC_ADMINISTRATOR, false, nullptr, "", saveTable},
            {"reload", SEC_ADMINISTRATOR, true, &HandleReloadTemplateNPCCommand, ""},
        };

        static std::vector<ChatCommand> commandTable = {
            {"template", SEC_ADMINISTRATOR, true, nullptr, "", TemplateNPCTable},
        };

        return commandTable;
    }

    static bool HandleCopyCommand(ChatHandler* handler, const char* _args)
    {
        Player* caller = handler->GetSession()->GetPlayer();
        if (!caller)
        {
            assert(false);
        }
        ObjectGuid selected = handler->GetSession()->GetPlayer()->GetTarget();
        if (!selected)
        {
            handler->SendGlobalGMSysMessage("You have to select a player");
            return false;
        }
        Player* target = ObjectAccessor::FindConnectedPlayer(selected);

        if (!target)
        {
            handler->SendGlobalGMSysMessage("You have to select a player.");
            return false;
        }

        if (caller == target)
        {
            handler->SendGlobalGMSysMessage("You are trying to copy your own gear, which is pointless.");
            return false;
        }

        if (caller->getClass() != target->getClass())
        {
            handler->SendGlobalGMSysMessage("In order to copy someone you need to be of the same class.");
            return false;
        }

        sTemplateNpcMgr->Copy(caller, target);

        handler->SendGlobalGMSysMessage("Character copied.");

        return true;
    }

    static bool HandleSaveGearHorde(ChatHandler* handler, const char* _args)
    {
        return HandleSaveCommon(handler, _args);
    }

    static bool HandleSaveGearAlly(ChatHandler* handler, const char* _args)
    {
        return HandleSaveCommon(handler, _args);
    }

    static bool HandleSaveGearHuman(ChatHandler* handler, const char* _args)
    {
        return HandleSaveCommon(handler, _args);
    }

    static bool HandleSaveCommon(ChatHandler* handler, const char* _args)
    {
        if (!*_args)
            return false;

        auto spec  = std::string(_args);

        auto selectedSpec = knownSpecs.find(spec);
        if (selectedSpec == knownSpecs.end())
        {
            handler->SendGlobalGMSysMessage("Unknown  passed");
            return false;
        }

        Player* p = handler->getSelectedPlayerOrSelf();
        if (p == nullptr)
            return false;

        sTemplateNpcMgr->OverwriteTemplate(p, spec);
        sTemplateNpcMgr->ExtractGearTemplateToDB(p, spec);
        sTemplateNpcMgr->ExtractTalentTemplateToDB(p, spec);
        sTemplateNpcMgr->ExtractGlyphsTemplateToDB(p, spec);

        handler->SendGlobalGMSysMessage("Template extracted to DB. You might want to type \".template reload\".");

        return true;
    }

    static bool HandleReloadTemplateNPCCommand(ChatHandler* handler, const char* /*args*/)
    {
        sLog->outString("Reloading templates for Template NPC table...");
        sTemplateNpcMgr->LoadHumanGearContainer();
        sTemplateNpcMgr->LoadAllianceGearContainer();
        sTemplateNpcMgr->LoadHordeGearContainer();
        sTemplateNpcMgr->LoadTalentsContainer();
        sTemplateNpcMgr->LoadGlyphsContainer();
        handler->SendGlobalGMSysMessage("Template NPC templates reloaded.");
        return true;
    }
};


class TemplateNPC_World : public WorldScript
{
public:
    TemplateNPC_World() : WorldScript("TemplateNPC_World") { }

    void OnStartup() override
    {
        // Load templates for Template NPC #1
        sLog->outString("== TEMPLATE NPC ===========================================================================");
        sLog->outString("Loading Template Talents...");
        sTemplateNpcMgr->LoadTalentsContainer();

        // Load templates for Template NPC #2
        sLog->outString("Loading Template Glyphs...");
        sTemplateNpcMgr->LoadGlyphsContainer();

        // Load templates for Template NPC #3
        sLog->outString("Loading Template Gear for Humans...");
        sTemplateNpcMgr->LoadHumanGearContainer();

        // Load templates for Template NPC #4
        sLog->outString("Loading Template Gear for Alliances...");
        sTemplateNpcMgr->LoadAllianceGearContainer();

        // Load templates for Template NPC #5
        sLog->outString("Loading Template Gear for Hordes...");
        sTemplateNpcMgr->LoadHordeGearContainer();
        sLog->outString("== END TEMPLATE NPC ===========================================================================");
    }
};

void AddSC_TemplateNPC()
{
    new TemplateNPC();
    new TemplateNPC_command();
    new TemplateNPC_World();
}
