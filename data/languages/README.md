# LibreSprite Localization

This directory contains translation files for the LibreSprite user interface.

## How Localization Works

LibreSprite uses a key-value JSON format for translations. The localization system is implemented in `src/app/modules/i18n.cpp`:

1. The user's language preference is stored in `data/pref.xml` (default: `en`)
2. Users select their language in **Edit > Preferences > General > Language**
3. At runtime, the `i18n()` function looks up UI strings by their English key
4. If a translation exists, it returns the localized string; otherwise, it falls back to the English key

## File Format

Each language file is a JSON object mapping English strings to their translations:

```json
{
  "File": "&Archivo",
  "Edit": "&Editar",
  "Save": "&Guardar"
}
```

**Format notes:**
- Keys are the original English strings
- Values are the translated strings
- The `&` character denotes keyboard accelerators (Alt+key shortcuts)
- Newlines in strings use `\n` escape sequences
- Special characters must be JSON-escaped

## Available Languages

| Code | Language   | File      |
|------|------------|-----------|
| ar   | Arabic     | ar.json   |
| de   | German     | de.json   |
| en   | English    | en.json   |
| es   | Spanish    | es.json   |
| fr   | French     | fr.json   |
| hi   | Hindi      | hi.json   |
| id   | Indonesian | id.json   |
| it   | Italian    | it.json   |
| jp   | Japanese   | jp.json   |
| kr   | Korean     | kr.json   |
| pl   | Polish     | pl.json   |
| pt   | Portuguese | pt.json   |
| ru   | Russian    | ru.json   |
| zh   | Chinese    | zh.json   |

## Adding a New Translation

1. Copy `en.json` to `{language_code}.json` (use ISO 639-1 codes)
2. Translate all values while keeping keys unchanged
3. Preserve `&` accelerator positions appropriately for the target language
4. Test by setting the language in Preferences

## Contributing Translations

To contribute a new language or improve existing translations:

1. Fork the [LibreSprite repository](https://github.com/LibreSprite/LibreSprite)
2. Create or edit the appropriate language file in `data/languages/`
3. Use `en.json` as the reference for all translatable strings
4. Submit a pull request with your changes

**Tips for translators:**
- Build LibreSprite in debug mode to see missing translation keys logged to console
- Keep translations concise to fit UI elements
- Test your translations in the actual application
- Maintain consistent terminology throughout the translation
