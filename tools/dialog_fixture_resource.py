"""Stage one isolated dialogue fixture without modifying game resources."""
import argparse
import hashlib
from pathlib import Path
import sys

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent / "scene_export"))
from export import ReadOnlyGameFiles, parse_ini  # noqa: E402
from f2lib.msg import parse  # noqa: E402


def stage(game_root, run_root, language):
    config = parse_ini(game_root / "fallout2.cfg").get("system", {})
    # The test config mounts data twice; a different install config would make
    # the exporter's selected source differ from what this test game reads.
    for key, expected in (("master_patches", "data"), ("critter_patches", "data"),
                          ("master_dat", "master.dat"), ("critter_dat", "critter.dat"), ("f2_res_dat", r"mods\f2_res.dat")):
        if config.get(key, "").lower() != expected:
            raise ValueError(f"unsupported {key} in scratch game config")
    key = f"text/{language}/dialog/kcsulik.msg"
    game = ReadOnlyGameFiles(game_root)
    try:
        original = game.read(key)
    finally:
        game.close()
    messages = parse(original)
    required = (126, 796, 797, 798, 799, 800, 801, 803)
    if any(number not in messages for number in required):
        raise ValueError(f"{key} lacks the recorded first-page messages")
    # Appending replaces only ID 797 under CE's last-entry-wins parser.
    text = ("I have some questions for you, and I would like to hear everything you can tell me about this town."
            if language == "english" else
            "У меня есть несколько вопросов. Расскажи, пожалуйста, всё, что ты знаешь об этом городе.")
    suffix = f"\r\n{{797}}{{}}{{{text}}}\r\n".encode("cp1251")
    target = run_root / "data" / "text" / language / "dialog" / "kcsulik.msg"
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(original + suffix)
    check = parse(target.read_bytes())
    if len(check) != len(messages) or any(check[n] != messages[n] for n in messages if n != 797):
        raise ValueError("fixture changed other messages")
    print(f"DIALOG_FIXTURE_RESOURCE language={language} source={key} messages={len(messages)} "
          f"source_sha256={hashlib.sha256(original).hexdigest()} fixture_sha256={hashlib.sha256(target.read_bytes()).hexdigest()} target={target}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("game_root", type=Path)
    parser.add_argument("run_root", type=Path)
    parser.add_argument("language", choices=("english", "russian"))
    args = parser.parse_args()
    stage(args.game_root, args.run_root, args.language)
