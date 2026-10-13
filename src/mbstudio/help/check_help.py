#!/usr/bin/env python3
"""Check the help pages of the MB-System programs.

    check_help.py <help source dir> [program ...]

Every English page must exist in every language and all pages must follow the
page template (see README.md): the title line "# <program> — <summary>", the
section headings of the language in the template order, and the same code
(synopsis, examples) in every language. Programs given on the command line
must have a page. Exits with status 1 and a list of problems otherwise.
"""

import os
import re
import sys

EM_DASH = "—"
SECTIONS = {
    "en": ["Synopsis", "Description", "Common options", "Examples", "Notes", "See also"],
    "ja": ["書式", "説明", "主なオプション", "使用例", "注意", "関連項目"],
    "zh_CN": ["语法", "说明", "常用选项", "示例", "注意", "另请参阅"],
}
OPTIONAL = {"Notes", "注意"}


def code_blocks(text):
    """Indented and fenced code blocks, whitespace-normalised."""
    blocks = []
    lines = text.splitlines()
    i = 0
    in_list = False
    while i < len(lines):
        line = lines[i]
        if line.strip().startswith("```"):
            i += 1
            block = []
            while i < len(lines) and not lines[i].strip().startswith("```"):
                block.append(lines[i].strip())
                i += 1
            blocks.append("\n".join(b for b in block if b))
        elif line.startswith("    ") and not in_list and (i == 0 or lines[i - 1].strip() == ""):
            block = []
            while i < len(lines) and (lines[i].startswith("    ") or lines[i].strip() == ""):
                block.append(lines[i].strip())
                i += 1
            blocks.append("\n".join(b for b in block if b))
            continue
        in_list = bool(re.match(r"^\s*[-*]\s", line)) or (in_list and line.strip() != "")
        i += 1
    return blocks


def check_page(path, program, lang, problems):
    with open(path, encoding="utf-8-sig") as f:
        text = f.read()
    lines = text.splitlines()
    where = "%s/%s.md" % (lang, program)
    if not lines or not re.match(r"^# %s %s \S" % (re.escape(program), EM_DASH), lines[0]):
        problems.append("%s: first line must be '# %s %s <summary>'" % (where, program, EM_DASH))
    headings = [l[3:].strip() for l in lines if l.startswith("## ")]
    expected = SECTIONS[lang]
    required = [h for h in expected if h not in OPTIONAL]
    for h in required:
        if h not in headings:
            problems.append("%s: missing section '## %s'" % (where, h))
    unknown = [h for h in headings if h not in expected]
    if unknown:
        problems.append("%s: unexpected sections %s" % (where, unknown))
    known = [h for h in headings if h in expected]
    if known != sorted(known, key=expected.index):
        problems.append("%s: sections out of order" % where)
    # outside code, "<file>" would be taken for an HTML tag and disappear
    prose = re.sub(r"(?ms)^```.*?^```", "", text)
    prose = re.sub(r"(?m)^    .*$", "", prose)
    prose = re.sub(r"`[^`]*`", "", prose)
    for match in re.finditer(r"<[A-Za-z/!][^>]*>?", prose):
        problems.append("%s: '%s' outside code (put it in backticks)" % (where, match.group(0)[:30]))
    return text


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    source = sys.argv[1]
    required = sys.argv[2:]
    problems = []
    english = sorted(f[:-3] for f in os.listdir(os.path.join(source, "en")) if f.endswith(".md"))
    for program in required:
        if program not in english:
            problems.append("en/%s.md: missing (the program is installed)" % program)
    for program in english:
        texts = {}
        for lang in SECTIONS:
            path = os.path.join(source, lang, program + ".md")
            if not os.path.exists(path):
                problems.append("%s/%s.md: missing" % (lang, program))
                continue
            texts[lang] = check_page(path, program, lang, problems)
        if "en" in texts:
            reference = code_blocks(texts["en"])
            for lang, text in texts.items():
                if lang != "en" and code_blocks(text) != reference:
                    problems.append("%s/%s.md: code blocks differ from the English page" % (lang, program))
    for lang in SECTIONS:
        folder = os.path.join(source, lang)
        for f in sorted(os.listdir(folder)) if os.path.isdir(folder) else []:
            if f.endswith(".md") and f[:-3] not in english:
                problems.append("%s/%s: no English page" % (lang, f))
    if problems:
        print("\n".join(problems))
        print("%d problem(s) in the help pages" % len(problems))
        return 1
    print("help pages OK: %d programs in %s" % (len(english), ", ".join(SECTIONS)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
