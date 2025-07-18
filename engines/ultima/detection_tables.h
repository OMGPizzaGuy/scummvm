/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

namespace Ultima {

#define GUI_OPTIONS_ULTIMA1	GUIO0()
#define GUI_OPTIONS_ULTIMA4	GUIO1(GUIO_NOSPEECH)
#define GUI_OPTIONS_ULTIMA6	GUIO0()
#define GUI_OPTIONS_ULTIMA8	GUIO9(GAMEOPTION_ORIGINAL_SAVELOAD, GAMEOPTION_FRAME_SKIPPING, GAMEOPTION_FRAME_LIMITING, GAMEOPTION_CHEATS, GAMEOPTION_HIGH_RESOLUTION, GAMEOPTION_FOOTSTEP_SOUNDS, GAMEOPTION_JUMP_TO_MOUSE, GAMEOPTION_FONT_REPLACEMENT, GAMEOPTION_FONT_ANTIALIASING)
#define GUI_OPTIONS_REMORSE	GUIO6(GUIO_NOMIDI, GAMEOPTION_FRAME_SKIPPING, GAMEOPTION_FRAME_LIMITING, GAMEOPTION_CHEATS, GAMEOPTION_HIGH_RESOLUTION, GAMEOPTION_CAMERA_WITH_SILENCER)
#define GUI_OPTIONS_REGRET	GUIO7(GUIO_NOMIDI, GAMEOPTION_FRAME_SKIPPING, GAMEOPTION_FRAME_LIMITING, GAMEOPTION_CHEATS, GAMEOPTION_HIGH_RESOLUTION, GAMEOPTION_CAMERA_WITH_SILENCER, GAMEOPTION_ALWAYS_CHRISTMAS)
#define GUI_OPTIONS_REGRET_DEMO	GUIO6(GUIO_NOMIDI, GAMEOPTION_FRAME_SKIPPING, GAMEOPTION_FRAME_LIMITING, GAMEOPTION_CHEATS, GAMEOPTION_HIGH_RESOLUTION, GAMEOPTION_CAMERA_WITH_SILENCER)
#define GUI_OPTIONS_MARTIAN_DREAMS GUIO0()
#define GUI_OPTIONS_SAVAGE_EMPIRE  GUIO0()

// Ultima 6 normal mode only
#define ENTRY_ULTIMA6_NORMAL(FILENAME, MD5, FILESIZE, LANG, PLATFORM) {{"ultima6", 0, AD_ENTRY1s(FILENAME, MD5, FILESIZE), LANG, PLATFORM, ADGF_NO_FLAGS, GUI_OPTIONS_ULTIMA6}, GAME_ULTIMA6, 0}

// Ultima 6 enhanced mode only
#define ENTRY_ULTIMA6_ENHANCED(FILENAME, MD5, FILESIZE, LANG, PLATFORM) {{"ultima6_enh", 0, AD_ENTRY1s(FILENAME, MD5, FILESIZE), LANG, PLATFORM, ADGF_NO_FLAGS, GUI_OPTIONS_ULTIMA6}, GAME_ULTIMA6, GF_VGA_ENHANCED}

// Ultima 6 both normal and enhanced mode (this should normally be used)
#define ENTRY_ULTIMA6(FILENAME, MD5, FILESIZE, LANG, PLATFORM) \
	ENTRY_ULTIMA6_NORMAL(FILENAME, MD5, FILESIZE, LANG, PLATFORM), \
	ENTRY_ULTIMA6_ENHANCED(FILENAME, MD5, FILESIZE, LANG, PLATFORM)

// Ultima 6 normal mode only - unstable (currently only used for the PC98 version)
#define ENTRY_ULTIMA6_NORMAL_UNSTABLE(FILENAME, MD5, FILESIZE, LANG, PLATFORM) {{"ultima6", 0, AD_ENTRY1s(FILENAME, MD5, FILESIZE), LANG, PLATFORM, ADGF_UNSTABLE, GUI_OPTIONS_ULTIMA6}, GAME_ULTIMA6, 0}

static const UltimaGameDescription GAME_DESCRIPTIONS[] = {
	{
		// Ultima I - The First Age of Darkness
		{
			"ultima1",
			"VGA Enhanced",
			{
				{ "maptiles.vga", 0, "d4b67e17affe64c0ddb48511bfe4cf37", 47199 },
				{ "objtiles.vga", 0, "1a1446970d095aeb03bcf6dcec40d6e2", 289344 },
				{ "map.bin", 0, "f99633a0110ccf90837ab161be56cf1c", 13104 },
				AD_LISTEND
			},
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUI_OPTIONS_ULTIMA1
		},
		GAME_ULTIMA1,
		GF_VGA_ENHANCED
	},

	{
		// Ultima I - The First Age of Darkness, PC98 version
		{
			"ultima1",
			0,
			AD_ENTRY2s("egctown.bin",	"4f7de68f6689cf9617aa1ea03240137e", 4896,
					   "map.bin",		"f99633a0110ccf90837ab161be56cf1c", 13104),
			Common::JA_JPN,
			Common::kPlatformPC98,
			ADGF_UNSTABLE,
			GUI_OPTIONS_ULTIMA1
		},
		GAME_ULTIMA1,
		0
	},

	{
		// Ultima I - The First Age of Darkness
		{
			"ultima1",
			0,
			AD_ENTRY1s("map.bin", "f99633a0110ccf90837ab161be56cf1c", 13104),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUI_OPTIONS_ULTIMA1
		},
		GAME_ULTIMA1,
		0
	},

	{
		// Ultima IV - Quest of the Avatar
		{
			"ultima4",
			0,
			AD_ENTRY1s("britain.ult", "304fe52ce5f34b9181052363d74d7505", 1280),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_NO_FLAGS,
			GUI_OPTIONS_ULTIMA4
		},
		GAME_ULTIMA4,
		0
	},

	{
		// Ultima IV - Quest of the Avatar
		{
			"ultima4_enh",
			0,
			AD_ENTRY1s("britain.ult", "304fe52ce5f34b9181052363d74d7505", 1280),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_NO_FLAGS,
			GUI_OPTIONS_ULTIMA4
		},
		GAME_ULTIMA4,
		GF_VGA_ENHANCED
	},

	// GOG Ultima VI
	ENTRY_ULTIMA6("converse.a", "5065716423ef1389e3f7b4946d815c26", 162615,
				Common::EN_ANY,
				Common::kPlatformDOS),

	// Ultima VI - French patch by Docwise Dragon
	// https://sirjohn.de/en/ultima6/ultima-vi-french-translation-patch/
	// Note: Not all user interface elements are translated in ScummVM
	ENTRY_ULTIMA6("converse.a", "35c95d56737d957db7e72193e810053b", 182937,
				Common::FR_FRA,
				Common::kPlatformDOS),

	// Ultima VI - German Patch https://sirjohn.de/ultima-6/
	// Note: Not all user interface elements are translated in ScummVM
	ENTRY_ULTIMA6("converse.a", "ae979230b97f8813bdf8f82698847435", 198627,
				Common::DE_DEU,
				Common::kPlatformDOS),

	// Ultima VI - German Patch 1.6 https://sirjohn.de/ultima-6/
	// Note: Not all user interface elements are translated in ScummVM
	ENTRY_ULTIMA6("converse.a", "5242f0228bbc9c3a60c7aa6071499688", 198797,
				Common::DE_DEU,
				Common::kPlatformDOS),

	// Ultima VI - German Patch 1.7 https://sirjohn.de/ultima-6/
	// Note: Not all user interface elements are translated in ScummVM
	ENTRY_ULTIMA6("converse.a", "f4e9280402baff12e5132e62f7bbb54f", 198810,
				Common::DE_DEU,
				Common::kPlatformDOS),

	// PC98 Ultima 6
	ENTRY_ULTIMA6_NORMAL_UNSTABLE("converse.a", "99975e79e7cae3ee24a8e33982f60fe4", 190920,
				Common::JA_JPN,
				Common::kPlatformPC98),

	// Ultima VI - Nitpickers Delight older version
	ENTRY_ULTIMA6("converse.a", "5c15ba2a75fb921b715a1a0bf0152bac", 165874,
				Common::EN_ANY,
				Common::kPlatformDOS),

	// Ultima VI - Nitpickers Delight newer version
	ENTRY_ULTIMA6("converse.a", "9f77c84a03efc77df2d53544d1275da8", 167604,
				Common::EN_ANY,
				Common::kPlatformDOS),

	// Ultima VI - alternative release
	// TRAC #14659
	ENTRY_ULTIMA6("converse.a", "ee22a6ac3964f9ff11a48fcb3f4a9389", 162458,
				Common::EN_ANY,
				Common::kPlatformDOS),

	// Ultima VIII - CD (provided by ddeluca1com, bug #11944)
	{
		{
			"ultima8",
			"",
			AD_ENTRY1s("usecode/eusecode.flx", "5494165cbf4b07be04a465e28350e086", 1209018),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_NO_FLAGS,
			GUI_OPTIONS_ULTIMA8
		},
		GAME_ULTIMA8,
		0
	},

	// Ultima VIII - Ultima Collection 1998
	{
		{
			"ultima8",
			"",
			AD_ENTRY1s("usecode/eusecode.flx", "87c8b584e2947e5e4d99bd8bff6cea2e", 1251108),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_NO_FLAGS,
			GUI_OPTIONS_ULTIMA8
		},
		GAME_ULTIMA8,
		0
	},

	// GOG Ultima VIII
	{
		{
			"ultima8",
			"Gold Edition",
			AD_ENTRY2s("usecode/eusecode.flx", "c61f1dacde591cb39d452264e281f234", 1251108,
					   "static/eintro.skf", "b34169ece4286735262ac3430c441909", 1297731),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_NO_FLAGS,
			GUI_OPTIONS_ULTIMA8
		},
		GAME_ULTIMA8,
		0
	},

	{
		{
			"ultima8",
			"Gold Edition",
			AD_ENTRY2s("usecode/fusecode.flx", "4017eb8678ee24af0ce8c7647a05509b", 1300957,
					   "static/fintro.skf", "58990327f3e155551a69f41e7dcc0d08", 1275321),
			Common::FR_FRA,
			Common::kPlatformDOS,
			ADGF_NO_FLAGS,
			GUI_OPTIONS_ULTIMA8
		},
		GAME_ULTIMA8,
		0
	},

	{
		{
			"ultima8",
			"Gold Edition",
			AD_ENTRY2s("usecode/gusecode.flx", "d69599a46870b66c1b7c02710ed185bd", 1378604,
					   "static/gintro.skf", "4a2f3a996d13dba0528ef73264303bf5", 1264343),
			Common::DE_DEU,
			Common::kPlatformDOS,
			ADGF_NO_FLAGS,
			GUI_OPTIONS_ULTIMA8
		},
		GAME_ULTIMA8,
		0
	},

	// Fan translation patch for GOG version (provided by Condezer0, bug #14484)
	{
		{
			"ultima8",
			"Gold Edition",
			AD_ENTRY2s("usecode/eusecode.flx", "cd4b330e09efd232360fd476bcc6a1d1", 1285847,
					   "static/eintro.skf", "9f8a9d95248ae3ae4b74a24ab88bf95f", 1233678),
			Common::ES_ESP,
			Common::kPlatformDOS,
			ADGF_NO_FLAGS,
			GUI_OPTIONS_ULTIMA8
		},
		GAME_ULTIMA8,
		0
	},

	// German CD version
	{
		{
			"ultima8",
			"",
			AD_ENTRY1s("usecode/gusecode.flx", "dc981f82c6303548ad1c967cdef1a0ea", 1335445),
			Common::DE_DEU,
			Common::kPlatformDOS,
			ADGF_NO_FLAGS,
			GUI_OPTIONS_ULTIMA8
		},
		GAME_ULTIMA8,
		0
	},

	// French version (provided by habib256 bug #13003)
	{
		{
			"ultima8",
			"",
			AD_ENTRY1s("usecode/fusecode.flx", "6f7643af10bffa11debea4533ba47061", 1300957),
			Common::FR_FRA,
			Common::kPlatformDOS,
			ADGF_NO_FLAGS,
			GUI_OPTIONS_ULTIMA8
		},
		GAME_ULTIMA8,
		0
	},

	{
		{
			"ultima8",
			"",
			AD_ENTRY1s("usecode/eusecode.flx", "1abad7a58e052ff3d9664df1ab2ddb86", 1136206),
			Common::ES_ESP,
			Common::kPlatformDOS,
			ADGF_NO_FLAGS,
			GUI_OPTIONS_ULTIMA8
		},
		GAME_ULTIMA8,
		0
	},

	{
		{
			"ultima8",
			"",
			AD_ENTRY1s("usecode/jusecode.flx", "1793bb252b805bf8d59300690987c605", 1208003),
			Common::JA_JPN,
			Common::kPlatformDOS,
			ADGF_NO_FLAGS,
			GUI_OPTIONS_ULTIMA8
		},
		GAME_ULTIMA8,
		0
	},

	// Crusader games use a very similar engine to ultima8.
	// complete.  Because each version requires a separate Usecode callback
	// table, only fully patched versions are marked supported.

	// GOG Crusader - No Remorse (V1.21)
	{
		{
			"remorse",
			"",
			AD_ENTRY1s("usecode/eusecode.flx", "0a0f64507adc4f280129c735ee9cad42", 556613),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_USECODE_DEFAULT,
			GUI_OPTIONS_REMORSE
		},
		GAME_CRUSADER_REM,
		0
	},

	// Crusader - No Remorse CD version (V1.10) provided by heff978 (#15065)
	{
		{
			"remorse",
			"",
			AD_ENTRY1s("usecode/eusecode.flx", "73b413b1ef291c4512f16c719ad746f3", 419591),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUI_OPTIONS_REMORSE
		},
		GAME_CRUSADER_REM,
		0
	},

	// Crusader - No Remorse provided by andy155
	{
		{
			"remorse",
			"",
			AD_ENTRY1s("usecode/eusecode.flx", "3fb211f4adfd80595078afc85bdfe7b4", 433143),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUI_OPTIONS_REMORSE
		},
		GAME_CRUSADER_REM,
		0
	},

	// Crusader - No Remorse (V1.01), unpatched data on the GOG CD image
	{
		{
			"remorse",
			"",
			AD_ENTRY1s("usecode/eusecode.flx", "8c74327e30088ce93f08a15a7f85b3ce", 418556),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE | ADGF_USECODE_ORIG,
			GUI_OPTIONS_REMORSE
		},
		GAME_CRUSADER_REM,
		0
	},

	// Crusader - No Remorse (French) provided by BeWorld2018
	{
		{
			"remorse",
			"",
			AD_ENTRY1s("usecode/eusecode.flx", "efbd33d6a5e8f14e9c57f963c3fbe939", 423051),
			Common::FR_FRA,
			Common::kPlatformDOS,
			ADGF_UNSTABLE | ADGF_USECODE_FR,
			GUI_OPTIONS_REMORSE
		},
		GAME_CRUSADER_REM,
		0
	},

	// Crusader - No Remorse (Spanish) provided by Wesker
	{
		{
			"remorse",
			"",
			AD_ENTRY1s("usecode/eusecode.flx", "36a16d70c97d0379f1133cc743c31313", 558493),
			Common::ES_ESP,
			Common::kPlatformDOS,
			ADGF_USECODE_ES,
			GUI_OPTIONS_REMORSE
		},
		GAME_CRUSADER_REM,
		0
	},

	// Crusader - No Remorse (Spanish fan patch) provided by Wesker
	{
		{
			"remorse",
			"Fan Translation",
			AD_ENTRY1s("usecode/eusecode.flx", "a8b5c421c5d74be8c69fcd4fecadd1dd", 559015),
			Common::ES_ESP,
			Common::kPlatformDOS,
			ADGF_USECODE_DEFAULT,
			GUI_OPTIONS_REMORSE
		},
		GAME_CRUSADER_REM,
		0
	},

	// Crusader - No Remorse (Japanese) provided by Dominus
	{
		{
			"remorse",
			"",
			AD_ENTRY1s("usecode/jusecode.flx", "088105959be4f2de1cb9e796e71c5f2d", 554522),
			Common::JA_JPN,
			Common::kPlatformWindows,
			ADGF_UNSTABLE | ADGF_USECODE_JA,
			GUI_OPTIONS_REMORSE
		},
		GAME_CRUSADER_REM,
		0
	},

	// Crusader - No Remorse - Demo V1.12D
	{
		{
			"remorse",
			"Demo",
			AD_ENTRY1s("usecode/eusecode.flx", "41cdca35b62f4b2a7bb4c3b1ec782423", 556613),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_DEMO,
			GUI_OPTIONS_REMORSE
		},
		GAME_CRUSADER_REM,
		0
	},

	// GOG Crusader - No Regret (V1.06)
	{
		{
			"regret",
			"",
			AD_ENTRY1s("usecode/eusecode.flx", "1bb360156b7240a1f05eb9bda01c54db", 481652),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE | ADGF_USECODE_DEFAULT,
			GUI_OPTIONS_REGRET
		},
		GAME_CRUSADER_REG,
		0
	},

	// Crusader - No Regret - German V1.07NV
	{
		{
			"regret",
			"",
			AD_ENTRY1s("usecode/eusecode.flx", "1824d9725de45a8b49f058c12c6cf5c3", 484445),
			Common::DE_DEU,
			Common::kPlatformDOS,
			ADGF_UNSTABLE | ADGF_USECODE_DE,
			GUI_OPTIONS_REGRET
		},
		GAME_CRUSADER_REG,
		0
	},

	// Crusader - No Regret - Demo V1.08
	{
		{
			"regret",
			"Demo",
			AD_ENTRY1s("usecode/eusecode.flx", "c6416e4716f3c008dba113a2a460367e", 483174),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE | ADGF_DEMO,
			GUI_OPTIONS_REGRET_DEMO
		},
		GAME_CRUSADER_REG,
		0
	},

	// Crusader - No Regret (Spanish) provided by Wesker
	{
		{
			"regret",
			"",
			AD_ENTRY1s("usecode/eusecode.flx", "f5906654047ed1dab75760da6426ecfa", 478125),
			Common::ES_ESP,
			Common::kPlatformDOS,
			ADGF_UNSTABLE | ADGF_USECODE_ES,
			GUI_OPTIONS_REGRET
		},
		GAME_CRUSADER_REG,
		0
	},

	// GOG Martian Dreams
	{
		{
			"martiandreams",
			0,
			AD_ENTRY1s("talk.lzc", "6efafc030cb552028c564897e40d87b5", 409705),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUI_OPTIONS_MARTIAN_DREAMS
		},
		GAME_MARTIAN_DREAMS,
		0
	},

	// GOG Martian Dreams - Enhanced
	{
		{
			"martiandreams_enh",
			0,
			AD_ENTRY1s("talk.lzc", "6efafc030cb552028c564897e40d87b5", 409705),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUI_OPTIONS_MARTIAN_DREAMS
		},
		GAME_MARTIAN_DREAMS,
		GF_VGA_ENHANCED
	},


	// The Savage Empire v1.6
	{
		{
			"thesavageempire",
			0,
			AD_ENTRY1s("talk.lzc", "bef60fbc3cc478b2a2e8f0883652b2f3", 160784),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUI_OPTIONS_SAVAGE_EMPIRE
		},
		GAME_SAVAGE_EMPIRE,
		0
	},

	// The Savage Empire v1.6
	{
		{
			"thesavageempire_enh",
			0,
			AD_ENTRY1s("talk.lzc", "bef60fbc3cc478b2a2e8f0883652b2f3", 160784),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUI_OPTIONS_SAVAGE_EMPIRE
		},
		GAME_SAVAGE_EMPIRE,
		GF_VGA_ENHANCED
	},

	// The Savage Empire v2.1
	{
		{
			"thesavageempire",
			0,
			AD_ENTRY1s("talk.lzc", "1bbb5a425e1d7e2e3aa9b887e511ffc6", 160931),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUI_OPTIONS_SAVAGE_EMPIRE
		},
		GAME_SAVAGE_EMPIRE,
		0
	},

	// The Savage Empire v2.1
	{
		{
			"thesavageempire_enh",
			0,
			AD_ENTRY1s("talk.lzc", "1bbb5a425e1d7e2e3aa9b887e511ffc6", 160931),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUI_OPTIONS_SAVAGE_EMPIRE
		},
		GAME_SAVAGE_EMPIRE,
		GF_VGA_ENHANCED
	},

	// The Savage Empire v2.1
	{
		{
			"thesavageempire",
			0,
			AD_ENTRY1s("talk.lzc", "1bbb5a425e1d7e2e3aa9b887e511ffc6", 160931),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_SAVAGE_EMPIRE,
		0
	},

	// The Savage Empire v2.1
	{
		{
			"thesavageempire_enh",
			0,
			AD_ENTRY1s("talk.lzc", "1bbb5a425e1d7e2e3aa9b887e511ffc6", 160931),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO0()
		},
		GAME_SAVAGE_EMPIRE,
		GF_VGA_ENHANCED
	},

	{ AD_TABLE_END_MARKER, (GameId)0, 0 }
};

} // End of namespace Ultima
