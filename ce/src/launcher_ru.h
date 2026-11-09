#pragma once

// The worker's user-facing log in Russian (task 238).  The log lines are
// English format strings in the code; with --ui-language ru each one is looked
// up here by its exact text and the Russian format with the same conversions
// is printed instead.  A line that is not here stays English.  Machine lines
// the GUI parses (RELEASE_*, the scene JSON, FAILED, "  ... N / M MB") are
// never in this table.  No windows, no network.

#include <cstring>

namespace launcher_ru {

// The two lines launcher_progress.h reads by their text.
inline constexpr const char* kUnpacking = "unpacking";
inline constexpr const char* kUnpackingRu = "распаковка";
inline constexpr const char* kMakingScene = "making the 3D scene data";
inline constexpr const char* kMakingSceneRu = "строим 3D-данные сцены";

struct Entry {
    const char* en;
    const char* ru;
};

inline constexpr Entry kTable[] = {
    { "ERROR: %s", "ОШИБКА: %s" },
    { "  route %s failed: %s", "  путь %s не сработал: %s" },
    { "  route that worked: %s", "  сработал путь: %s" },
    { "  downloaded %zu bytes", "  скачано байт: %zu" },
    { "unpacking", "распаковка" },
    { "making the 3D scene data for: %s", "строим 3D-данные сцены для: %s" },
    { "game root (from --game-root): %s", "папка игры (из --game-root): %s" },
    { "game root (the folder of this launcher): %s", "папка игры (папка этого запуска): %s" },
    { "this is not a Fallout 2 folder: master.dat and critter.dat are not there",
        "это не папка Fallout 2: в ней нет master.dat и critter.dat" },
    { "no Fallout 2 folder was chosen", "папка Fallout 2 не выбрана" },
    { "installed build: %s", "установленная сборка: %s" },
    { "installed build: an older manual install (no version.txt)", "установленная сборка: старая ручная установка (нет version.txt)" },
    { "installed build: none, fresh install", "установленная сборка: нет, чистая установка" },
    { "GitHub does not answer (%s)", "GitHub не отвечает (%s)" },
    { "release build: %s", "сборка в релизе: %s" },
    { "GitHub does not answer, and the first install needs the internet. Try again later or use another network",
        "GitHub не отвечает, а первой установке нужен интернет. Попробуйте позже или другую сеть" },
    { "GitHub does not answer: the installed game is used as it is", "GitHub не отвечает: используем установленную игру как есть" },
    { "installing: none -> %s", "установка: нет -> %s" },
    { "updating: %s -> %s", "обновление: %s -> %s" },
    { "downloading %s", "скачиваем %s" },
    { "GitHub does not answer, download failed (%s)", "GitHub не отвечает, скачать не удалось (%s)" },
    { "the downloaded file could not be unpacked; try the install again", "скачанный файл не распаковался; повторите установку" },
    { "the downloaded package is damaged (the game is not in it); try the install again",
        "скачанный пакет повреждён (игры в нём нет); повторите установку" },
    { "could not save the current build before updating; nothing was changed",
        "не удалось сохранить текущую сборку перед обновлением; ничего не изменено" },
    { "installing into the game folder:", "ставим в папку игры:" },
    { "  %s: not in the package, skipped", "  %s: нет в пакете, пропущен" },
    { "  %s: installed", "  %s: установлен" },
    { "  %s: copy failed", "  %s: копирование не удалось" },
    { "  %s: copy failed (%s)", "  %s: копирование не удалось (%s)" },
    { "installed", "установлен" },
    { "installed (default)", "установлен (по умолчанию)" },
    { "  fallout2.cfg: kept (the player's file is never overwritten)", "  fallout2.cfg: оставлен (файл игрока никогда не перезаписывается)" },
    { "the installed build matches the release: nothing to pull", "установленная сборка совпадает с релизом: качать нечего" },
    { "backed up build %s to f2reimagined-backups", "сборка %s сохранена в f2reimagined-backups" },
    { "scene-exporter.exe is missing; the game will start without 3D", "scene-exporter.exe не найден; игра запустится без 3D" },
    { "  the first install builds every map: about 20 minutes; later updates take seconds",
        "  первая установка строит все карты: около 20 минут; дальше обновления идут секунды" },
    { "the 3D data could not be built; the game will start without 3D", "3D-данные не построились; игра запустится без 3D" },
    { "the 3D scene data is current", "3D-данные сцены актуальны" },
    { "the game folder holds build %s", "в папке игры сборка %s" },
    { "done (--no-start)", "готово (--no-start)" },
    { "the Russian translation could not be installed (see the lines above)",
        "русификатор не установился (см. строки выше); игра установлена, русификатор попробуем снова при следующей установке" },
    { "the game is not installed yet; press Install first", "игра ещё не установлена; сначала нажмите «Установить»" },
    { "the game could not be started", "игру не удалось запустить" },
    { "starting %s", "запускаем %s" },
    { "bye", "до свидания" },
    // the Russian translation
    { "  RPU release page failed: %s", "  страница релиза RPU не открылась: %s" },
    { "cannot read the official RPU release version on GitHub", "не удалось узнать версию официального релиза RPU на GitHub" },
    { "downloading the official Russian translation from RPU %s", "скачиваем официальный русификатор из RPU %s" },
    { "cannot create translation staging folder: %s", "не удалось создать временную папку для русификатора: %s" },
    { "the Russian translation archive could not be unpacked", "архив русификатора не распаковался" },
    { "the official translation archive does not contain rpu_russian.dat", "в официальном архиве перевода нет rpu_russian.dat" },
    { "failed to install the Russian translation file", "не удалось установить файл русификатора" },
    { "  installed mods/rpu_russian.dat from the official RPU release", "  mods/rpu_russian.dat установлен из официального релиза RPU" },
    { "  installed mods/rpu_russian.dat from the project mirror (RPU %s)", "  mods/rpu_russian.dat установлен с зеркала проекта (RPU %s)" },
    { "  kept the existing mods/rpu_russian.dat", "  mods/rpu_russian.dat уже есть, оставлен" },
    { "  game language set to Russian; mods/mods_order.txt updated", "  язык игры: русский; mods/mods_order.txt обновлён" },
    { "GitHub does not answer for the Russian translation (%s); trying the project mirror",
        "GitHub не отвечает для русификатора (%s); пробуем зеркало проекта" },
    { "  the mirror's RPU version: %s", "  версия RPU на зеркале: %s" },
    { "the Russian translation could not be downloaded: GitHub and the project mirror both failed (%s)",
        "русификатор не скачался: не ответили ни GitHub, ни зеркало проекта (%s)" },
    { "cannot update mods/mods_order.txt", "не удалось обновить mods/mods_order.txt" },
    { "removing the Russian translation:", "удаляем русификатор:" },
    { "cannot remove mods/rpu_russian.dat; is the game running? Close it and try again",
        "не удалось удалить mods/rpu_russian.dat; игра запущена? Закройте её и повторите" },
    { "  removed mods/rpu_russian.dat", "  mods/rpu_russian.dat удалён" },
    { "  mods/mods_order.txt: the translation line removed", "  mods/mods_order.txt: строка русификатора убрана" },
    { "  game language set back to English", "  язык игры снова английский" },
    // removing the mod
    { "removing the mod from the game folder:", "убираем мод из папки игры:" },
    { "  removed %s", "  удалено: %s" },
    { "  cannot remove %s (%s); is the game running? Close it and try again",
        "  не удалось удалить %s (%s); игра запущена? Закройте её и повторите" },
    { "  removed the folder %s", "  папка удалена: %s" },
    { "  cannot remove the folder %s (%s)", "  не удалось удалить папку %s (%s)" },
    { "the mod is only partly removed; see the lines above", "мод удалён не полностью; см. строки выше" },
    { "the mod is removed; your saves and the original Fallout 2 files were not touched",
        "мод удалён; ваши сохранения и оригинальные файлы Fallout 2 не тронуты" },
};

// The Russian format for an English one; the English one itself when there is none.
inline const char* translate(const char* en)
{
    for (const Entry& entry : kTable) {
        if (std::strcmp(entry.en, en) == 0) return entry.ru;
    }
    return en;
}

} // namespace launcher_ru
