"""Prepare local Task 227 candidates from separately extracted source archives.

The output is unverified game data, never a launcher catalog or release.
"""
import argparse
import hashlib
import json
import re
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / "scene_export" / "vendor"))
from f2lib.dat2 import Dat2, write_dat2

RPU_SHA256 = "f32d9d0b2696a5066a05a13e431c8dfc9d0772c058932121209049c374d8a8c7"


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def select(root, kind):
    result = {}
    for path in sorted(root.rglob("*")):
        if not path.is_file():
            continue
        name = path.relative_to(root).as_posix().lower()
        parts = name.split("/")
        if kind == "text" and len(parts) >= 3 and parts[:2] == ["text", "english"] and path.suffix.lower() in (".msg", ".txt", ".sve"):
            name = "text/russian/" + "/".join(parts[2:])
        elif kind == "voice" and len(parts) >= 3 and parts[:2] == ["sound", "speech"] and path.suffix.lower() in (".acm", ".lip"):
            pass
        else:
            continue
        if name in result:
            raise ValueError(f"duplicate case-insensitive source: {name}")
        result[name] = path.read_bytes()
    return result


def convert_text(raw, levy):
    if levy:
        raw = bytes(b + 48 if 0xB0 <= b <= 0xBF else b for b in raw)
    if b"\0" in raw:
        raise ValueError("NUL in source")
    return raw.decode("cp866").encode("cp1251")


def normalize_fargus(label, name, raw):
    if label == "fio-fargus-text" and name == "text/russian/game/scrname.msg":
        row = "{1086}{}{Объявление}".encode("cp866")
        raw = raw.replace(row.replace(b"\xea", b"\xba"), row)
    return raw


def validate_msg(raw):
    if b"\0" in raw:
        raise ValueError("NUL in message")
    fields = []
    pos = 0
    while pos < len(raw):
        start = raw.find(b"{", pos)
        stray = raw.find(b"}", pos)
        if stray != -1 and (start == -1 or stray < start):
            raise ValueError("unmatched closing brace")
        if start == -1:
            break
        end = raw.find(b"}", start + 1)
        if end == -1:
            raise ValueError("unterminated field")
        field = raw[start + 1:end].replace(b"\n", b"").replace(b"\r", b"")
        if b"{" in field or len(field) >= 1024:
            raise ValueError("nested brace or field exceeds engine limit")
        fields.append(field)
        pos = end + 1
    if not fields:
        raise ValueError("no message entries; would mask fallback")
    if len(fields) % 3:
        raise ValueError("incomplete message entry")
    if any(not re.fullmatch(rb"[+-]?[0-9]+", number) for number in fields[::3]):
        raise ValueError("invalid message number")


def validate_sve(raw):
    if b"\0" in raw:
        raise ValueError("binary or NUL subtitle")
    lines = raw.splitlines()
    if not lines:
        raise ValueError("empty subtitle")
    previous = -1
    for line in lines:
        if not line:
            continue
        if len(line) > 257 or not re.fullmatch(rb"[0-9]+:.*", line):
            raise ValueError("invalid subtitle line or exceeds engine buffer")
        frame = int(line.split(b":", 1)[0])
        if frame < previous:
            raise ValueError("subtitle frames out of order")
        previous = frame
    if previous == -1:
        raise ValueError("empty subtitle")


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def write_checked(files, dest):
    write_dat2(files, dest)
    with Dat2(dest) as archive:
        if len(archive.order) != len(files) or set(archive.names()) != set(files):
            raise ValueError(f"incorrect directory: {dest}")
        for name, content in files.items():
            if archive.read(name) != content:
                raise ValueError(f"incorrect member: {name}")
    return {"size": dest.stat().st_size, "sha256": digest(dest),
            "files": len(files), "members_sha256": {n: sha(v) for n, v in sorted(files.items())}}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("work", type=Path, help="extracted work directory")
    parser.add_argument("output", type=Path, help="new output directory outside git")
    parser.add_argument("--rpu-reference", type=Path, required=True)
    args = parser.parse_args()
    if args.output.resolve().is_relative_to(Path(__file__).resolve().parent.parent):
        raise SystemExit("Output must be outside the repository")
    if args.output.exists():
        raise SystemExit("Output directory already exists; choose a new directory")
    if digest(args.rpu_reference) != RPU_SHA256:
        raise SystemExit("RPU reference SHA256 does not match official v2.4.34")
    work = args.work
    app = work / "fio" / "{app}" / "DATA"
    components = work / "fio" / "{src}" / "Fallout 2 Language Packs (FiO)_Resource" / "Components"
    # The embedded installer copies {app}, then the selected component.
    recipes = {
        "fio-1c-text": ("text", [app, components / "F2_1C" / "DATA"]),
        "fio-fargus-text": ("text", [app, components / "F2_FAR" / "DATA"]),
        "fio-levy-text": ("text", [app, components / "F2_LEV" / "DATA"]),
        "steam-1c-text": ("text", [work / "steam1c" / "app" / "data"]),
        "steam-1c-voice": ("voice", [work / "steam1c" / "app" / "data"]),
        "rp233-levy-text-overlay": ("text", [work / "rp233" / "DATA"]),
        "rp233-levy-voice": ("voice", [work / "rp233" / "DATA"]),
        "rp23-1c-for-fargus-voice": ("voice", [work / "rp-speech" / "Data"]),
    }
    with Dat2(args.rpu_reference) as rpu:
        rpu_text = {n for n in rpu.names() if n.startswith("text/russian/")}
        font_names = [f"font{i}.aaf" for i in range(5)] + [f"font{i}.fon" for i in (0, 1, 2, 3, 5)]
        if not set(font_names).issubset(set(rpu.names())):
            raise ValueError("reference missing required legacy fonts")
        fonts = {n: rpu.read(n) for n in font_names}
    args.output.mkdir(parents=True)
    metadata = {}
    all_names = {}
    for label, (kind, roots) in recipes.items():
        files = {}
        layers = []
        for root in roots:
            if not root.is_dir():
                raise FileNotFoundError(root)
            layer = select(root, kind)
            if not layer:
                raise ValueError(f"no {kind} files: {root}")
            layers.append({"root": str(root), "files": len(layer), "overwrites": len(files.keys() & layer.keys())})
            files.update(layer)
        excluded = []
        source_hashes = {n: sha(v) for n, v in files.items()}
        component_names = set(select(roots[-1], "text")) if label in ("fio-fargus-text", "fio-levy-text") else set()
        for name, raw in list(files.items()):
            if kind != "text":
                continue
            try:
                raw = normalize_fargus(label, name, raw)
                if name.endswith(".msg"):
                    validate_msg(raw)
                if name in component_names:
                    if name.endswith(".msg"):
                        # Comments are ignored by the engine and may use DOS decoration bytes.
                        raw = re.sub(rb"(?<=\{)[^{}]*(?=\})",
                                     lambda m: convert_text(m.group(), label == "fio-levy-text"), raw)
                    else:
                        raw = convert_text(raw, label == "fio-levy-text")
                if name.endswith(".msg"):
                    validate_msg(raw)
                elif name.endswith(".sve"):
                    validate_sve(raw)
                elif b"\0" in raw:
                    raise ValueError("NUL in text")
                files[name] = raw
            except (UnicodeError, ValueError) as e:
                files.pop(name)
                excluded.append({"name": name, "source_sha256": source_hashes[name], "reason": str(e)})
        dest = args.output / (label + ".dat")
        result = write_checked(files, dest)
        all_names[label] = set(files)
        metadata[label] = {"status": "candidate; modes and releases unverified", "kind": kind,
                           "scope": "old RP candidate" if label.startswith("rp") else "vanilla candidate",
                           **result, "source_members_sha256": source_hashes,
                           "extensions": dict(Counter(Path(n).suffix for n in files)),
                           "layers": layers, "excluded": excluded,
                           "rpu_text_overlap": len(set(files) & rpu_text) if kind == "text" else None,
                           "rpu_text_missing": len(rpu_text - set(files)) if kind == "text" else None}
    metadata["shared-legacy-fonts"] = {"kind": "shared legacy fonts", "status": "candidate; activation unverified",
                                       "source": "BGforgeNet/Fallout2_Restoration_Project tag v2.4.34, rpu_russian.dat",
                                       "source_sha256": digest(args.rpu_reference),
                                       **write_checked(fonts, args.output / "shared-legacy-fonts.dat")}
    a = all_names["steam-1c-voice"]
    b = all_names["rp23-1c-for-fargus-voice"]
    metadata["voice_comparison"] = {"rp23_only": len(b - a), "both": len(a & b), "steam_only": len(a - b)}
    metadata["rpu_reference"] = {"path": str(args.rpu_reference), "sha256": digest(args.rpu_reference), "text_entries": len(rpu_text)}
    (args.output / "manifest.json").write_text(json.dumps(metadata, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
