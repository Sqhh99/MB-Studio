// MB-Studio QML UI - translations of the user interface.
//
// The catalogues are JSON files in the resources (i18n/<language>.json):
//
//   { "language": "ja",
//     "messages": { "<English source text>": "<translation>", ... },
//     "contexts": { "<context>": { "<source>": "<translation>" } } }
//
// "messages" apply to every context (QML file or C++ class); "contexts"
// hold the few texts that need a context-specific translation, e.g. the
// program categories ("ToolCategory"). i18n/check_translations.py keeps the
// catalogues in sync with the qsTr()/tr() texts of the sources.
#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QTranslator>

namespace mbstudio::ui {

struct LanguageInfo {
  QString code;        // "en", "ja", "zh_CN"
  QString nativeName;  // "English", "日本語", "简体中文"
};

// Languages of the user interface, English first.
const QList<LanguageInfo> &supportedLanguages();

// Supported language for a requested code; "" or "system" selects the
// operating system's user interface language (English if not supported).
QString resolveLanguage(const QString &requested);

class JsonTranslator : public QTranslator {
  Q_OBJECT
 public:
  explicit JsonTranslator(QObject *parent = nullptr);
  // Loads ":/mbstudio/i18n/<language>.json" (or the given file).
  bool loadLanguage(const QString &language);
  bool loadFile(const QString &path);
  QString translate(const char *context, const char *sourceText, const char *disambiguation = nullptr,
                    int n = -1) const override;
  bool isEmpty() const override;
  QString languageCode() const { return language_; }
  int size() const { return static_cast<int>(messages_.size()); }

 private:
  QString language_;
  QHash<QString, QString> messages_;
  QHash<QString, QHash<QString, QString>> contexts_;
};

}  // namespace mbstudio::ui
