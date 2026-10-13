#!/usr/bin/env python3
"""Keep the user interface translation catalogues in sync with the sources.

    check_translations.py <qmlui source dir> [--update]

Collects the translatable texts of the QML user interface plugin:
  - qsTr("...") in qml/**/*.qml
  - tr("...") and QCoreApplication::translate("<context>", "...") in the C++
    sources of the plugin
  - the program categories of the MB-System program catalogue
    (plugins/mbsystem/ToolService.cpp), translated in context "ToolCategory"
and checks every catalogue i18n/<language>.json: each text needs a
non-empty translation with the same %1, %2, ... placeholders, and the
catalogue must not keep texts that are no longer used.

--update adds missing texts with an empty translation (to be filled in) and
removes unused ones, keeping the file sorted. Exits 1 if a problem remains.
"""

import glob
import json
import os
import re
import sys

STRING = r'"((?:[^"\\]|\\.)*)"'
ESCAPES = {"n": "\n", "t": "\t", '"': '"', "'": "'", "\\": "\\"}


def unescape(text):
    return re.sub(r"\\(.)", lambda m: ESCAPES.get(m.group(1), m.group(1)), text)


def collect(source):
    messages = set()
    contexts = {}
    for path in glob.glob(os.path.join(source, "qml", "**", "*.qml"), recursive=True):
        with open(path, encoding="utf-8") as f:
            text = f.read()
        for m in re.finditer(r"\bqsTr\(\s*(?:" + STRING + r"|'((?:[^'\\]|\\.)*)')", text):
            messages.add(unescape(m.group(1) if m.group(1) is not None else m.group(2)))
    cpp = glob.glob(os.path.join(source, "*.cpp")) + glob.glob(os.path.join(source, "viewmodels", "*.cpp"))
    for path in cpp:
        with open(path, encoding="utf-8") as f:
            text = f.read()
        for m in re.finditer(r"(?<![\w:])tr\(\s*" + STRING, text):
            messages.add(unescape(m.group(1)))
        for m in re.finditer(r"QCoreApplication::translate\(\s*" + STRING + r"\s*,\s*" + STRING, text):
            contexts.setdefault(unescape(m.group(1)), set()).add(unescape(m.group(2)))
    catalogue = os.path.join(source, "..", "mbsystem", "ToolService.cpp")
    with open(catalogue, encoding="utf-8") as f:
        for m in re.finditer(r'entry\(\s*"[^"]+"\s*,\s*' + STRING, f.read()):
            contexts.setdefault("ToolCategory", set()).add(unescape(m.group(1)))
    return messages, contexts


def placeholders(text):
    return sorted(set(re.findall(r"%\d", text)))


def check_entries(lang, where, wanted, entries, problems, update):
    for text in sorted(wanted):
        value = entries.get(text, "")
        if not value:
            problems.append("%s: %s: missing translation of %r" % (lang, where, text))
            if update:
                entries[text] = ""
        elif placeholders(value) != placeholders(text):
            problems.append("%s: %s: placeholders of %r differ in %r" % (lang, where, text, value))
    for text in sorted(set(entries) - set(wanted)):
        problems.append("%s: %s: unused text %r" % (lang, where, text))
        if update:
            del entries[text]


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    update = "--update" in sys.argv
    if len(args) != 1:
        sys.exit(__doc__)
    source = args[0]
    messages, contexts = collect(source)
    catalogues = sorted(glob.glob(os.path.join(source, "i18n", "*.json")))
    if not catalogues:
        sys.exit("no catalogues in " + os.path.join(source, "i18n"))
    problems = []
    for path in catalogues:
        lang = os.path.splitext(os.path.basename(path))[0]
        with open(path, encoding="utf-8") as f:
            data = json.load(f)
        if data.get("language") != lang:
            problems.append("%s: \"language\" must be %r" % (lang, lang))
        entries = data.setdefault("messages", {})
        check_entries(lang, "messages", messages, entries, problems, update)
        ctx = data.setdefault("contexts", {})
        for name, wanted in sorted(contexts.items()):
            check_entries(lang, name, wanted, ctx.setdefault(name, {}), problems, update)
        for name in sorted(set(ctx) - set(contexts)):
            problems.append("%s: unused context %r" % (lang, name))
            if update:
                del ctx[name]
        if update:
            data["messages"] = dict(sorted(entries.items(), key=lambda kv: kv[0].lower()))
            data["contexts"] = {k: dict(sorted(v.items())) for k, v in sorted(ctx.items())}
            with open(path, "w", encoding="utf-8", newline="\n") as f:
                json.dump(data, f, ensure_ascii=False, indent=2)
                f.write("\n")
    total = len(messages) + sum(len(v) for v in contexts.values())
    if problems:
        print("\n".join(problems))
        print("%d problem(s); %d texts, catalogues: %s" % (len(problems), total,
                                                          ", ".join(os.path.basename(c) for c in catalogues)))
        return 1
    print("translations OK: %d texts in %s" % (total, ", ".join(os.path.basename(c) for c in catalogues)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
