#!/usr/bin/env python3
"""Build the offline HTML help of the MB-System programs.

    build_html.py [--version X.Y.Z] <help source dir> <output dir>

The sources are Markdown pages, one folder per language
(<source>/<language>/<program>.md), written in the small Markdown subset
described in README.md: ATX headings, paragraphs, "- " lists, indented or
fenced code blocks and inline `code`, **bold** and *italic*.

Output:
    <output>/index.html               language selection (follows the browser)
    <output>/<language>/index.html    list of the programs
    <output>/<language>/<program>.html

Only the Python standard library is used.
"""

import argparse
import html
import os
import re
import sys

LANGUAGES = {
    "en": {"name": "English", "html": "en"},
    "ja": {"name": "日本語", "html": "ja"},
    "zh_CN": {"name": "简体中文", "html": "zh-Hans"},
}

TEXT = {
    "en": {
        "title": "MB-System program help",
        "intro": "Help pages of the MB-System command line programs included in MB-Studio. "
                 "Run the programs from the MB-System Command Prompt or from the Programs page of MB-Studio.",
        "index": "All programs",
        "missing": "This page is not available in this language yet; the English page is shown.",
    },
    "ja": {
        "title": "MB-System プログラム ヘルプ",
        "intro": "MB-Studio に含まれる MB-System コマンドラインプログラムのヘルプです。"
                 "プログラムは「MB-System コマンドプロンプト」または MB-Studio の「プログラム」ページから実行できます。",
        "index": "プログラム一覧",
        "missing": "このページはまだ日本語に翻訳されていないため、英語版を表示しています。",
    },
    "zh_CN": {
        "title": "MB-System 程序帮助",
        "intro": "MB-Studio 所含 MB-System 命令行程序的帮助文档。"
                 "可在“MB-System 命令提示符”或 MB-Studio 的“程序”页面中运行这些程序。",
        "index": "全部程序",
        "missing": "本页尚未翻译为中文，以下显示英文版。",
    },
}

CSS = """
:root { color-scheme: light dark; --bg:#f6f8fa; --fg:#17212b; --muted:#5e6e7e; --card:#fff;
        --border:#dce2e9; --accent:#1e88e5; --code:#eef2f6; }
@media (prefers-color-scheme: dark) {
  :root { --bg:#0e151d; --fg:#e3eaf1; --muted:#8497aa; --card:#151f2a; --border:#243344;
          --accent:#1fb6c9; --code:#1b2836; } }
* { box-sizing: border-box; }
body { margin:0; background:var(--bg); color:var(--fg);
       font: 15px/1.6 "Segoe UI", "Yu Gothic UI", "Microsoft YaHei UI", system-ui, sans-serif; }
header { background:var(--card); border-bottom:1px solid var(--border); padding:12px 24px;
         display:flex; gap:16px; align-items:center; flex-wrap:wrap; }
header .brand { font-weight:600; margin-right:auto; }
header a { color:var(--accent); text-decoration:none; }
header a.current { color:var(--fg); font-weight:600; }
main { max-width:920px; margin:0 auto; padding:24px; }
h1 { font-size:1.6em; margin:0.2em 0 0.6em; }
h2 { font-size:1.2em; margin-top:1.8em; padding-bottom:4px; border-bottom:1px solid var(--border); }
code, pre { font-family: Consolas, "Cascadia Mono", monospace; font-size:0.92em; }
code { background:var(--code); padding:1px 5px; border-radius:4px; }
pre { background:var(--code); padding:12px 14px; border-radius:8px; overflow-x:auto; }
pre code { background:none; padding:0; }
ul { padding-left:1.4em; }
li { margin:0.25em 0; }
.note { background:var(--card); border:1px solid var(--border); border-radius:8px; padding:8px 12px;
        color:var(--muted); }
.programs { list-style:none; padding:0; display:grid; grid-template-columns:repeat(auto-fill,minmax(260px,1fr));
            gap:10px; }
.programs li { background:var(--card); border:1px solid var(--border); border-radius:8px; padding:10px 12px; margin:0; }
.programs a { color:var(--accent); font-weight:600; text-decoration:none; font-family:Consolas, monospace; }
.programs span { display:block; color:var(--muted); font-size:0.92em; }
footer { color:var(--muted); font-size:0.85em; text-align:center; padding:24px; }
"""

EM_DASH = "—"


def inline(text):
    """Escape HTML and convert `code`, **bold** and *italic*."""
    parts = re.split(r"(`[^`]*`)", text)
    out = []
    for part in parts:
        if len(part) >= 2 and part.startswith("`") and part.endswith("`"):
            out.append("<code>" + html.escape(part[1:-1]) + "</code>")
            continue
        part = html.escape(part)
        part = re.sub(r"\*\*(.+?)\*\*", r"<strong>\1</strong>", part)
        part = re.sub(r"(?<![\w*])\*(?!\s)(.+?)(?<!\s)\*(?![\w*])", r"<em>\1</em>", part)
        out.append(part)
    return "".join(out)


def markdown_to_html(text):
    """Convert the Markdown subset of the help pages; returns (title, html)."""
    lines = text.replace("\r\n", "\n").split("\n")
    out = []
    title = ""
    paragraph = []
    items = []
    i = 0

    def flush_paragraph():
        if paragraph:
            out.append("<p>" + inline(" ".join(s.strip() for s in paragraph)) + "</p>")
            paragraph.clear()

    def flush_list():
        if items:
            out.append("<ul>" + "".join("<li>" + inline(item) + "</li>" for item in items) + "</ul>")
            items.clear()

    while i < len(lines):
        line = lines[i]
        stripped = line.strip()
        if stripped.startswith("```"):
            flush_paragraph()
            flush_list()
            code = []
            i += 1
            while i < len(lines) and not lines[i].strip().startswith("```"):
                code.append(lines[i])
                i += 1
            out.append("<pre><code>" + html.escape("\n".join(code)) + "</code></pre>")
            i += 1
            continue
        if line.startswith("    ") and not paragraph and not items:
            code = []
            while i < len(lines) and (lines[i].startswith("    ") or lines[i].strip() == ""):
                code.append(lines[i][4:])
                i += 1
            while code and code[-1].strip() == "":
                code.pop()
            out.append("<pre><code>" + html.escape("\n".join(code)) + "</code></pre>")
            continue
        heading = re.match(r"^(#{1,4})\s+(.*)$", line)
        if heading:
            flush_paragraph()
            flush_list()
            level = len(heading.group(1))
            content = heading.group(2).strip()
            if level == 1 and not title:
                title = content
            out.append("<h%d>%s</h%d>" % (level, inline(content), level))
        elif re.match(r"^\s*[-*]\s+", line):
            flush_paragraph()
            items.append(re.sub(r"^\s*[-*]\s+", "", line))
        elif stripped == "":
            flush_paragraph()
            flush_list()
        elif items and line.startswith("  "):
            items[-1] += " " + stripped  # continuation of a list item
        else:
            flush_list()
            paragraph.append(line)
        i += 1
    flush_paragraph()
    flush_list()
    return title, "\n".join(out)


def page(lang, title, body, nav, version):
    return """<!DOCTYPE html>
<html lang="{lang}">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{title}</title>
<style>{css}</style>
</head>
<body>
<header>{nav}</header>
<main>
{body}
</main>
<footer>MB-Studio {version} · MB-System</footer>
</body>
</html>
""".format(lang=LANGUAGES[lang]["html"], title=html.escape(title), css=CSS, nav=nav, body=body,
           version=html.escape(version))


def navigation(lang, target, depth_prefix):
    """Header with the index link and the language switch for page 'target'."""
    links = ['<span class="brand">%s</span>' % html.escape(TEXT[lang]["title"]),
             '<a href="index.html">%s</a>' % html.escape(TEXT[lang]["index"])]
    for code, info in LANGUAGES.items():
        cls = ' class="current"' if code == lang else ""
        links.append('<a%s href="%s%s/%s" hreflang="%s">%s</a>'
                     % (cls, depth_prefix, code, target, info["html"], html.escape(info["name"])))
    return "\n".join(links)


def summary_of(title):
    return title.split(EM_DASH, 1)[1].strip() if EM_DASH in title else ""


def build(source, output, version):
    english = os.path.join(source, "en")
    programs = sorted(f[:-3] for f in os.listdir(english) if f.endswith(".md"))
    if not programs:
        sys.exit("no help pages in " + english)
    languages = [l for l in LANGUAGES if os.path.isdir(os.path.join(source, l))]

    for lang in languages:
        out_dir = os.path.join(output, lang)
        os.makedirs(out_dir, exist_ok=True)
        entries = []
        for program in programs:
            path = os.path.join(source, lang, program + ".md")
            fallback = not os.path.exists(path)
            if fallback:
                path = os.path.join(english, program + ".md")
            with open(path, encoding="utf-8-sig") as f:
                title, body = markdown_to_html(f.read())
            if fallback:
                body = '<p class="note">%s</p>\n%s' % (html.escape(TEXT[lang]["missing"]), body)
            entries.append((program, summary_of(title)))
            with open(os.path.join(out_dir, program + ".html"), "w", encoding="utf-8", newline="\n") as f:
                f.write(page(lang, title or program, body, navigation(lang, program + ".html", "../"), version))

        items = "\n".join('<li><a href="%s.html">%s</a><span>%s</span></li>'
                          % (html.escape(p), html.escape(p), html.escape(s)) for p, s in entries)
        body = "<h1>%s</h1>\n<p>%s</p>\n<ul class=\"programs\">\n%s\n</ul>" % (
            html.escape(TEXT[lang]["title"]), html.escape(TEXT[lang]["intro"]), items)
        with open(os.path.join(out_dir, "index.html"), "w", encoding="utf-8", newline="\n") as f:
            f.write(page(lang, TEXT[lang]["title"], body, navigation(lang, "index.html", "../"), version))

    # language selection: follows the browser language unless one is chosen
    choices = "\n".join('<li><a href="%s/index.html" hreflang="%s">%s</a><span>%s</span></li>'
                        % (code, LANGUAGES[code]["html"], html.escape(LANGUAGES[code]["name"]),
                           html.escape(TEXT[code]["title"])) for code in languages)
    script = """<script>
(function () {
  if (location.hash === "#choose") return;
  var langs = navigator.languages || [navigator.language || "en"];
  for (var i = 0; i < langs.length; i++) {
    var l = (langs[i] || "").toLowerCase();
    if (l.indexOf("ja") === 0) { location.replace("ja/index.html"); return; }
    if (l.indexOf("zh") === 0) { location.replace("zh_CN/index.html"); return; }
    if (l.indexOf("en") === 0) { location.replace("en/index.html"); return; }
  }
  location.replace("en/index.html");
})();
</script>"""
    body = "<h1>MB-System</h1>\n<ul class=\"programs\">\n%s\n</ul>\n%s" % (choices, script)
    nav = '<span class="brand">MB-System</span>'
    with open(os.path.join(output, "index.html"), "w", encoding="utf-8", newline="\n") as f:
        f.write(page("en", "MB-System program help", body, nav, version))
    print("HTML help: %d programs, languages %s -> %s" % (len(programs), ", ".join(languages), output))


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("source")
    parser.add_argument("output")
    parser.add_argument("--version", default="")
    args = parser.parse_args()
    build(args.source, args.output, args.version)


if __name__ == "__main__":
    main()
