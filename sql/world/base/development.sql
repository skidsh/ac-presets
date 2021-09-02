-- DATA
SET
@NpcEntry 		:= 333595,
@NpcName		:= "Master Pierce Raske",
@NpcSubname		:= "Arena Tournament Presets",
@NpcDisplayID		:= 15953;

INSERT INTO creature_template (`entry`, `modelid1`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `faction`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `unit_class`, `unit_flags`, `type`, `type_flags`, `InhabitType`, `RegenHealth`, `flags_extra`, `ScriptName`) VALUES
(@NpcEntry, @NpcDisplayID, @NpcName , @NpcSubname, "Speak", 0, 80, 80, 35, 1, 1, 1.14286, 1, 0, 1, 2, 7, 138936390, 3, 1, 2, "TemplateNPC");

INSERT INTO `npc_text` (`ID`, `text0_0`, `text0_1`) VALUES
(@NpcEntry, 'Here you can select a character template which will gear up, gem up, set talent specialization, and set glyphs for your character instantly.\r\n\r\nSelect your spec:', 'Here you can select a character template which will gear up, gem up, set talent specialization, and set glyphs for your character instantly.\r\n\r\nSelect your spec:');
