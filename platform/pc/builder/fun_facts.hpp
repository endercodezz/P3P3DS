#pragma once
// Shown on the build page while the compiler runs. Game facts are well-known
// public facts about Persona 3 / Persona 3 Portable; project facts are
// P3P3DS measurements (docs/3DS_PLATFORM.md).
namespace p3p3ds::builder {

inline constexpr const wchar_t *kFunFacts[] = {
    L"Persona 3 first came out on the PlayStation 2 in Japan in 2006. Persona 3 Portable followed on the PSP in 2009 in Japan and in 2010 in North America.",
    L"Persona 3 Portable was the first Persona game with a choice of protagonist: the female protagonist has her own Social Links, scenes and dialogue.",
    L"With the female protagonist, the Velvet Room attendant is Theodore, Elizabeth's younger brother.",
    L"The female protagonist has her own battle theme, \"Wiping All Out\".",
    L"Persona 3 Portable replaced walking around town in 3D with a point-and-click cursor. Tartarus is still explored in full 3D.",
    L"Unlike the PlayStation 2 original, Persona 3 Portable lets you give direct orders to every party member in battle.",
    L"The Dark Hour is a hidden hour between one day and the next. Almost nobody notices it.",
    L"Every night during the Dark Hour, Gekkoukan High School turns into Tartarus, a labyrinthine tower.",
    L"The protagonist's first Persona is Orpheus, the musician of Greek myth who went into the underworld.",
    L"Social Links follow the Major Arcana of the tarot. The protagonist's own Arcana is the Fool, card number 0.",
    L"The story runs on an in-game calendar from April 2009 to March 2010.",
    L"\"Memento mori\" - remember that you will die - is the theme that runs through the whole game.",
    L"The soundtrack was composed by Shoji Meguro.",
    L"Persona 3 Portable was re-released for modern consoles and PC in January 2023.",
    L"Persona 3 Reload (2024) remade the game from scratch, but without the female protagonist.",
    L"P3P3DS does not emulate the PSP processor on your 3DS: the game's 3.6 MB of MIPS code is translated to C++ and compiled to about 44 MB of ARM code - right now, on your PC.",
    L"About 750,000 PSP instructions are being translated in this build, grouped into 237 C++ files of 16 KiB of game code each.",
    L"On the 3DS, the PICA200 graphics chip draws what the PSP's Graphics Engine drew. The 480x272 picture is scaled to the 400x240 top screen.",
    L"Your ISO stays on the SD card: P3P3DS reads the game's CPK archives from it while you play.",
    L"Mods made for the community \"Mod Support\" patch (mod.cpk, bind/) go to sdmc:/p3p3ds/mods/.",
    L"The bottom screen of P3P3DS shows a live report: frame rate, speed, memory. It is also saved to sdmc:/p3p3ds/report.txt.",
};

} // namespace p3p3ds::builder
