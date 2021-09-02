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

    player->ApplyEnchantment(item, slot, false);
    item->SetEnchantment(slot, bonusEntry, 0, 0);
    player->ApplyEnchantment(item, slot, true);
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
        if (Item* haveItemEquipped = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            if (haveItemEquipped)
            {
                player->DestroyItemCount(haveItemEquipped->GetEntry(), 1, true, true);

                if (haveItemEquipped->IsInWorld())
                {
                    haveItemEquipped->RemoveFromWorld();
                    haveItemEquipped->DestroyForPlayer(player);
                }

                haveItemEquipped->SetUInt64Value(ITEM_FIELD_CONTAINED, 0);
                haveItemEquipped->SetSlot(NULL_SLOT);
                haveItemEquipped->SetState(ITEM_REMOVED, player);
            }
        }
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

void sTemplateNPC::Copy(Player* target, Player* src)
{

    target->resetTalents(true);

    // copy talents
    for (const auto& [talentId, _] : src->GetTalentMap())
    {
        target->learnSpell(talentId);
        target->addTalent(talentId, src->GetActiveSpecMask(), true);
        target->SetFreeTalentPoints(0);
    }

    RemoveAllGlyphs(target);

    // copy glyphs
    for (uint8 slot = 0; slot < MAX_GLYPH_SLOT_INDEX; ++slot) ApplyGlyph(target, slot, src->GetGlyph(slot));

    target->SendTalentsInfoData(false);

    // copy gear
    // empty slots will be ignored
    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        Item* equippedItem = src->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (!equippedItem)
            continue;

        target->EquipNewItem(i, equippedItem->GetEntry(), true);

        ApplyBonus(target, target->GetItemByPos(INVENTORY_SLOT_BAG_0, i), PERM_ENCHANTMENT_SLOT, equippedItem->GetEnchantmentId(PERM_ENCHANTMENT_SLOT));
        ApplyBonus(target, target->GetItemByPos(INVENTORY_SLOT_BAG_0, i), SOCK_ENCHANTMENT_SLOT, equippedItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT));
        ApplyBonus(target, target->GetItemByPos(INVENTORY_SLOT_BAG_0, i), SOCK_ENCHANTMENT_SLOT_2, equippedItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_2));
        ApplyBonus(target, target->GetItemByPos(INVENTORY_SLOT_BAG_0, i), SOCK_ENCHANTMENT_SLOT_3, equippedItem->GetEnchantmentId(SOCK_ENCHANTMENT_SLOT_3));
        ApplyBonus(target, target->GetItemByPos(INVENTORY_SLOT_BAG_0, i), BONUS_ENCHANTMENT_SLOT, equippedItem->GetEnchantmentId(BONUS_ENCHANTMENT_SLOT));
        ApplyBonus(target, target->GetItemByPos(INVENTORY_SLOT_BAG_0, i), PRISMATIC_ENCHANTMENT_SLOT, equippedItem->GetEnchantmentId(PRISMATIC_ENCHANTMENT_SLOT));
    }
}

void sTemplateNPC::LearnTemplateTalents(Player* player, std::string& playerSpecStr)
{
    for (TalentContainer::const_iterator itr = m_TalentContainer.begin(); itr != m_TalentContainer.end(); ++itr)
    {
        if ((*itr)->playerClass == GetClassString(player).c_str() && (*itr)->playerSpec == playerSpecStr)
        {
			switch (player->getClass())
			{
			case CLASS_WARRIOR:
				if ((*itr)->playerSpec == "Arms")
				{
			        player->learnSpellHighRank(12328); // sweeping strikes
			        player->learnSpellHighRank(12294); // mortal strike
			        player->learnSpellHighRank(46924); // bladestorm
			        player->learnSpellHighRank(12323); // piercing howl
				}
				if ((*itr)->playerSpec == "Protection")
				{
			        player->learnSpellHighRank(20243); // devastate
			        player->learnSpellHighRank(46968); // shockwave
                    player->learnSpellHighRank(12809); // concussion blow
                    player->learnSpellHighRank(12975); // last stand
				}
				break;
			case CLASS_PALADIN:
				if ((*itr)->playerSpec == "Holy")
				{
			        player->learnSpellHighRank(20473); // holy shock
			        player->learnSpellHighRank(53563); // beacon of light
			        player->learnSpellHighRank(64205); // divine sacrifice
			        player->learnSpellHighRank(31842); // divine illumination
			        player->learnSpellHighRank(20216); // divine favor
			        player->learnSpellHighRank(31821); // aura mastery
				}
				if ((*itr)->playerSpec == "Protection")
				{
			        player->learnSpellHighRank(20911); // blessing of sanctuary
			        player->learnSpellHighRank(64205); // divine sacrifice
			        player->learnSpellHighRank(20925); // holy shield
			        player->learnSpellHighRank(31935); // avenger's shield
			        player->learnSpellHighRank(53595); // hammer of the righteous
				}
				if ((*itr)->playerSpec == "Retribution")
				{
			        player->learnSpellHighRank(20066); // repentance
			        player->learnSpellHighRank(35395); // crusader strike
			        player->learnSpellHighRank(64205); // divine sacrifice
			        player->learnSpellHighRank(53385); // divine storm
				}
				break;
			case CLASS_HUNTER:
				if ((*itr)->playerSpec == "Beastmastery")
				{
			        player->learnSpellHighRank(19577); // intimidation
			        player->learnSpellHighRank(19574); // bestial wrath
			        player->learnSpellHighRank(19434); // aimed shot
                    player->learnSpellHighRank(23989); // readiness
				}
				if ((*itr)->playerSpec == "Marksmanship")
				{
			        player->learnSpellHighRank(19434); // aimed shot
			        player->learnSpellHighRank(23989); // readiness
			        player->learnSpellHighRank(19506); // trueshot aura
			        player->learnSpellHighRank(34490); // silencing shot
			        player->learnSpellHighRank(53209); // chimera shot
			        player->learnSpellHighRank(19503); // scatter shot
				}
				break;
			case CLASS_ROGUE:
				if ((*itr)->playerSpec == "Subtlety")
				{
			        player->learnSpellHighRank(16511); // hemorrhage
			        player->learnSpellHighRank(14185); // preparation
			        player->learnSpellHighRank(14183); // premeditation
			        player->learnSpellHighRank(36554); // shadowstep
			        player->learnSpellHighRank(51713); // shadow dance
				}
                if ((*itr)->playerSpec == "Combat")
                {
                    player->learnSpellHighRank(13750); // adrenaline rush
			        player->learnSpellHighRank(51690); // killing spree
                }
                if ((*itr)->playerSpec == "Assassination")
                {
			        player->learnSpellHighRank(14177); // cold blood
			        player->learnSpellHighRank(1329);  // mutilate
			        player->learnSpellHighRank(14185); // preparation
                }
				break;
			case CLASS_PRIEST:
				if ((*itr)->playerSpec == "Discipline")
				{
			        player->learnSpellHighRank(14751); // inner focus
			        player->learnSpellHighRank(10060); // power infusion
			        player->learnSpellHighRank(33206); // pain suppression
			        player->learnSpellHighRank(47540); // penance
			        player->learnSpellHighRank(19236); // desperate prayer
				}
				if ((*itr)->playerSpec == "Holy")
				{
					player->learnSpellHighRank(14751); // inner focus
					player->learnSpellHighRank(19236); // desperate prayer
					player->learnSpellHighRank(724); // lightwell
					player->learnSpellHighRank(34861); // circle of healing
					player->learnSpellHighRank(47788); // Guardian Spirit
				}
				if ((*itr)->playerSpec == "Shadow")
				{
			        player->learnSpellHighRank(15407); // mind fly
			        player->learnSpellHighRank(15487); // silence
			        player->learnSpellHighRank(15286); // vampiric embrace
			        player->learnSpellHighRank(15473); // shadowform
			        player->learnSpellHighRank(64044); // psychic horror
			        player->learnSpellHighRank(34914); // vampiric touch
			        player->learnSpellHighRank(47585); // dispersion
			        player->learnSpellHighRank(14751); // inner focus
				}
				break;
			case CLASS_DEATH_KNIGHT:
				if ((*itr)->playerSpec == "Unholy")
				{
			        player->learnSpellHighRank(49158); // corpse explosion
			        player->learnSpellHighRank(51052); // anti-magic zone
			        player->learnSpellHighRank(49222); // bone shield
			        player->learnSpellHighRank(49206); // summon gargoyle
			        player->learnSpellHighRank(49039); // lichborne
                    player->learnSpellHighRank(55090); // scourge strike
				}
				if ((*itr)->playerSpec == "Frost")
				{
			        player->learnSpellHighRank(49039); // lichborne
			        player->learnSpellHighRank(49796); // deathchill
			        player->learnSpellHighRank(49203); // hungering cold
			        player->learnSpellHighRank(51271); // unbreakable armor
			        player->learnSpellHighRank(49143); // frost strike
				}
                if ((*itr)->playerSpec == "Blood")
				{
			        player->learnSpellHighRank(48982); // rune tap
			        player->learnSpellHighRank(49016); // hysteria
			        player->learnSpellHighRank(55233); // vampiric blood
			        player->learnSpellHighRank(55050); // hearth strike
			        player->learnSpellHighRank(49028); // dancing rune weapon
			        player->learnSpellHighRank(49039); // lichborne
				}
				break;
			case CLASS_SHAMAN:
				if ((*itr)->playerSpec == "Enhancement")
				{
			        player->learnSpellHighRank(17364); // stormstrike
			        player->learnSpellHighRank(60103); // lava lash
			        player->learnSpellHighRank(30823); // shamanistic rage
			        player->learnSpellHighRank(51533); // feral spirit
				}
				if ((*itr)->playerSpec == "Restoration")
				{
			        player->learnSpellHighRank(16188); // nature's swiftness
			        player->learnSpellHighRank(16190); // mana tide totem
			        player->learnSpellHighRank(51886); // cleanse spirit
			        player->learnSpellHighRank(974);   // earth shield
			        player->learnSpellHighRank(61295); // riptide
                    player->learnSpellHighRank(55198); // tidal force
				}
				if ((*itr)->playerSpec == "Elemental")
				{
			        player->learnSpellHighRank(16166); // elemental mastery
			        player->learnSpellHighRank(51490); // thunderstorm
			        player->learnSpellHighRank(30706); // totem of wrath
				}
				break;
			case CLASS_MAGE:
				if ((*itr)->playerSpec == "Fire")
				{
			        player->learnSpellHighRank(11366); // pyroblast
			        player->learnSpellHighRank(11113); // blast wave
			        player->learnSpellHighRank(11129); // combustion
			        player->learnSpellHighRank(31661); // dragon's breath
			        player->learnSpellHighRank(44457); // living bomb
			        player->learnSpellHighRank(54646); // focus magic
				}
				if ((*itr)->playerSpec == "Frost")
				{
			        player->learnSpellHighRank(12472); // icy veins
			        player->learnSpellHighRank(11958); // cold snap
			        player->learnSpellHighRank(11426); // ice barrier
			        player->learnSpellHighRank(31687); // summon water elemental
			        player->learnSpellHighRank(44572); // deep freeze
			        player->learnSpellHighRank(54646); // focus magic
				}
                if ((*itr)->playerSpec == "Arcane")
				{
                    player->learnSpellHighRank(12043); // presence of mind
			        player->learnSpellHighRank(12042); // arcane power
			        player->learnSpellHighRank(31589); // slow
			        player->learnSpellHighRank(44425); // arcane barrage
			        player->learnSpellHighRank(12472); // icy veins
				}
				break;
			case CLASS_WARLOCK:
				if ((*itr)->playerSpec == "Affliction")
				{
			        player->learnSpellHighRank(18223); // curse of exhaustion
			        player->learnSpellHighRank(30108); // unstable affliction
			        player->learnSpellHighRank(48181); // haunt
			        player->learnSpellHighRank(18708); // fel domination
                    player->learnSpellHighRank(19028); // soul link
				}
				if ((*itr)->playerSpec == "Destruction")
				{
			        player->learnSpellHighRank(17877); // shadowburn
			        player->learnSpellHighRank(17962); // conflagrate
			        player->learnSpellHighRank(30283); // shadowfury
			        player->learnSpellHighRank(50796); // chaos bolt
			        player->learnSpellHighRank(18708); // fel domination
                    player->learnSpellHighRank(19028); // soul link
				}
				break;
			case CLASS_DRUID:
				if ((*itr)->playerSpec == "Restoration")
				{
			        player->learnSpellHighRank(17116); // nature's swiftness
			        player->learnSpellHighRank(18562); // swiftmend
			        player->learnSpellHighRank(48438); // wild growth
				}
				if ((*itr)->playerSpec == "Feral")
				{
			        player->learnSpellHighRank(61336); // survival instincts
			        player->learnSpellHighRank(49377); // feral charge
			        player->learnSpellHighRank(33876); // mangle cat
			        player->learnSpellHighRank(33878); // mangle bear
			        player->learnSpellHighRank(50334); // berserk
				}
                if ((*itr)->playerSpec == "Ballance")
				{
                    player->learnSpellHighRank(33831); // force of nature
                    player->learnSpellHighRank(50516); // typhoon
                    player->learnSpellHighRank(48505); // starfall
                    player->learnSpellHighRank(24858); // moonkin form
                    player->learnSpellHighRank(5570); // insect swarm
				}
				break;
			}
            player->addTalent((*itr)->talentId, player->GetActiveSpecMask(), 0);
        }
    }
    player->SyncSpentTalents(player);
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
    if (player->getRace() == RACE_HUMAN)
    {
        // reverse sort so we equip items from trinket to helm so we avoid issue with meta gems
        std::sort(m_HumanGearContainer.begin(), m_HumanGearContainer.end(), std::greater<HumanGearTemplate*>());

        for (HumanGearContainer::const_iterator itr = m_HumanGearContainer.begin(); itr != m_HumanGearContainer.end(); ++itr)
        {
            if ((*itr)->playerClass == GetClassString(player).c_str() && (*itr)->playerSpec == playerSpecStr)
            {
                player->EquipNewItem((*itr)->pos, (*itr)->itemEntry, true); // Equip the item and apply enchants and gems
                ApplyBonus(player, player->GetItemByPos(INVENTORY_SLOT_BAG_0, (*itr)->pos), PERM_ENCHANTMENT_SLOT, (*itr)->enchant);
                ApplyBonus(player, player->GetItemByPos(INVENTORY_SLOT_BAG_0, (*itr)->pos), BONUS_ENCHANTMENT_SLOT, (*itr)->bonusEnchant);
                ApplyBonus(player, player->GetItemByPos(INVENTORY_SLOT_BAG_0, (*itr)->pos), PRISMATIC_ENCHANTMENT_SLOT, (*itr)->prismaticEnchant);
                ApplyBonus(player, player->GetItemByPos(INVENTORY_SLOT_BAG_0, (*itr)->pos), SOCK_ENCHANTMENT_SLOT_2, (*itr)->socket2);
                ApplyBonus(player, player->GetItemByPos(INVENTORY_SLOT_BAG_0, (*itr)->pos), SOCK_ENCHANTMENT_SLOT_3, (*itr)->socket3);
                ApplyBonus(player, player->GetItemByPos(INVENTORY_SLOT_BAG_0, (*itr)->pos), SOCK_ENCHANTMENT_SLOT, (*itr)->socket1);
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
                player->EquipNewItem((*itr)->pos, (*itr)->itemEntry, true); // Equip the item and apply enchants and gems
                ApplyBonus(player, player->GetItemByPos(INVENTORY_SLOT_BAG_0, (*itr)->pos), PERM_ENCHANTMENT_SLOT, (*itr)->enchant);
                ApplyBonus(player, player->GetItemByPos(INVENTORY_SLOT_BAG_0, (*itr)->pos), BONUS_ENCHANTMENT_SLOT, (*itr)->bonusEnchant);
                ApplyBonus(player, player->GetItemByPos(INVENTORY_SLOT_BAG_0, (*itr)->pos), PRISMATIC_ENCHANTMENT_SLOT, (*itr)->prismaticEnchant);
                ApplyBonus(player, player->GetItemByPos(INVENTORY_SLOT_BAG_0, (*itr)->pos), SOCK_ENCHANTMENT_SLOT_2, (*itr)->socket2);
                ApplyBonus(player, player->GetItemByPos(INVENTORY_SLOT_BAG_0, (*itr)->pos), SOCK_ENCHANTMENT_SLOT_3, (*itr)->socket3);
                ApplyBonus(player, player->GetItemByPos(INVENTORY_SLOT_BAG_0, (*itr)->pos), SOCK_ENCHANTMENT_SLOT, (*itr)->socket1);
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
                player->EquipNewItem((*itr)->pos, (*itr)->itemEntry, true); // Equip the item and apply enchants and gems
                ApplyBonus(player, player->GetItemByPos(INVENTORY_SLOT_BAG_0, (*itr)->pos), PERM_ENCHANTMENT_SLOT, (*itr)->enchant);
                ApplyBonus(player, player->GetItemByPos(INVENTORY_SLOT_BAG_0, (*itr)->pos), BONUS_ENCHANTMENT_SLOT, (*itr)->bonusEnchant);
                ApplyBonus(player, player->GetItemByPos(INVENTORY_SLOT_BAG_0, (*itr)->pos), PRISMATIC_ENCHANTMENT_SLOT, (*itr)->prismaticEnchant);
                ApplyBonus(player, player->GetItemByPos(INVENTORY_SLOT_BAG_0, (*itr)->pos), SOCK_ENCHANTMENT_SLOT_2, (*itr)->socket2);
                ApplyBonus(player, player->GetItemByPos(INVENTORY_SLOT_BAG_0, (*itr)->pos), SOCK_ENCHANTMENT_SLOT_3, (*itr)->socket3);
                ApplyBonus(player, player->GetItemByPos(INVENTORY_SLOT_BAG_0, (*itr)->pos), SOCK_ENCHANTMENT_SLOT, (*itr)->socket1);
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

            sTemplateNpcMgr->RemoveAllGear(player);

            // Don't let players to Template feature while wearing some gear
            for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
            {
                if (Item* haveItemEquipped = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                {
                    if (haveItemEquipped)
                    {
                        player->GetSession()->SendAreaTriggerMessage("You need to remove all your equipped items in order to this feature!");
						CloseGossipMenuFor(player);
                        return;
                    }
                }
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

            player->_RemoveAllItemMods();
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
