#include "Translator.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>

namespace mbstudio::ui {

const QList<LanguageInfo> &supportedLanguages() {
  static const QList<LanguageInfo> languages = {
      {QStringLiteral("en"), QStringLiteral("English")},
      {QStringLiteral("ja"), QStringLiteral("日本語")},
      {QStringLiteral("zh_CN"), QStringLiteral("简体中文")},
  };
  return languages;
}

QString resolveLanguage(const QString &requested) {
  QStringList wanted;
  if (requested.isEmpty() || requested == QLatin1String("system"))
    wanted = QLocale::system().uiLanguages();
  else
    wanted << requested;
  for (QString code : wanted) {
    code.replace(QLatin1Char('-'), QLatin1Char('_'));
    for (const auto &l : supportedLanguages())
      if (code.compare(l.code, Qt::CaseInsensitive) == 0)
        return l.code;
    // ja_JP -> ja; any Chinese variant -> Simplified Chinese
    const QString base = code.section(QLatin1Char('_'), 0, 0).toLower();
    if (base == QLatin1String("zh"))
      return QStringLiteral("zh_CN");
    for (const auto &l : supportedLanguages())
      if (base == l.code)
        return l.code;
  }
  return QStringLiteral("en");
}

JsonTranslator::JsonTranslator(QObject *parent) : QTranslator(parent) {}

bool JsonTranslator::loadLanguage(const QString &language) {
  return loadFile(QStringLiteral(":/mbstudio/i18n/%1.json").arg(language));
}

bool JsonTranslator::loadFile(const QString &path) {
  messages_.clear();
  contexts_.clear();
  language_.clear();
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    return false;
  QJsonParseError error;
  const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
  if (error.error != QJsonParseError::NoError || !doc.isObject())
    return false;
  const QJsonObject root = doc.object();
  language_ = root.value(QStringLiteral("language")).toString();
  const QJsonObject messages = root.value(QStringLiteral("messages")).toObject();
  for (auto it = messages.begin(); it != messages.end(); ++it)
    if (!it.value().toString().isEmpty())
      messages_.insert(it.key(), it.value().toString());
  const QJsonObject contexts = root.value(QStringLiteral("contexts")).toObject();
  for (auto c = contexts.begin(); c != contexts.end(); ++c) {
    const QJsonObject entries = c.value().toObject();
    QHash<QString, QString> &target = contexts_[c.key()];
    for (auto it = entries.begin(); it != entries.end(); ++it)
      if (!it.value().toString().isEmpty())
        target.insert(it.key(), it.value().toString());
  }
  return true;
}

QString JsonTranslator::translate(const char *context, const char *sourceText, const char *disambiguation,
                                  int n) const {
  (void)disambiguation;
  (void)n;  // %n is replaced by QCoreApplication::translate
  if (sourceText == nullptr)
    return QString();
  const QString source = QString::fromUtf8(sourceText);
  if (context != nullptr) {
    const auto c = contexts_.constFind(QString::fromUtf8(context));
    if (c != contexts_.constEnd()) {
      const auto it = c->constFind(source);
      if (it != c->constEnd())
        return *it;
    }
  }
  return messages_.value(source);  // empty: not translated, Qt keeps the source
}

bool JsonTranslator::isEmpty() const { return messages_.isEmpty() && contexts_.isEmpty(); }

}  // namespace mbstudio::ui
