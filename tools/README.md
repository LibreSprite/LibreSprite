# I18N String Extraction Tool

This directory contains the internationalization (i18n) string extraction tool.

The `update_i18n.py` script automatically scans the codebase to find strings that need translation and updates the language JSON files with any missing entries.

## Usage

Run the script from the repository root directory:

```bash
python3 tools/update_i18n.py
```

## What it Does

The tool performs three main tasks:

1. **Scans C++ source files** (`./src/**/*.cpp` and `./src/**/*.h`)
   - Looks for calls to `app::i18n()` function
   - Extracts string literals passed directly to the function
   - Example: `app::i18n("Shortcut: ")` → extracts "Shortcut: "

2. **Scans XML files**
   - Scans all XML files in `./data/widgets/`
   - Scans `./data/gui.xml` for tool definitions
   - Extracts values of `text` attributes
   - **Strips `&` characters**: Since `Widget::setI18N()` removes ampersands before using text as an i18n key, the tool automatically strips them during extraction
   - Example: `<button text="&amp;OK">` → extracts "OK" (not "&OK")

3. **Updates language JSON files** (`./data/languages/*.json`)
   - Adds missing translation keys to all language files
   - For new entries, sets the value equal to the key (e.g., `"OK": "OK"`)
   - Maintains alphabetical order by key
   - **Preserves all existing entries**: The tool only adds new keys and never deletes or modifies existing translations

## When to Run

Run this tool whenever you:
- Add new UI text that needs translation
- Add new widgets with text attributes
- Add new `app::i18n()` calls in C++ code
- Want to ensure all language files are in sync

## Output

The script will display:
- Number of C++ and XML files scanned
- Number of unique strings found
- Number of new entries added to each language file

Example output:
```
I18N String Extraction Tool
==================================================
Repository root: /home/user/repository

Scanning 1252 C++ files...
Found 2 strings in C++ files
Scanning 41 XML files...
Found 486 strings in XML files

Total unique strings to translate: 488

Updating 14 language files...
  en.json: Added 5 new entries
  fr.json: Added 5 new entries
  ...

Done!
```

## Limitations

The current implementation:
- **Only extracts direct string literals** from C++ code (e.g., `app::i18n("text")`)
- **Cannot extract strings passed by variable** (e.g., `app::i18n(someVariable)`)
- **Cannot extract strings from function returns** (e.g., `app::i18n(getName())`)

### Handling Dynamic Strings

For cases where strings are passed indirectly (variables or function returns), use the runtime detection mechanism:

1. **Build the application in DEBUG mode**: When compiled with `_DEBUG` defined, the application automatically logs missing translation keys to stdout.

2. **Run and test the application**: Exercise all features and UI elements. When a string is not found in the language file, it will be printed to the console in JSON format:
   ```
   "Missing String" : "Missing String",
   ```

3. **Copy the output**: The logged strings are already in the correct JSON format and can be copied directly into the language files.

4. **Re-run the static tool**: After adding any manual entries, run `update_i18n.py` again to ensure all files are synchronized and alphabetically sorted.

### Best Practices

1. Use direct string literals when possible for better static extraction
2. Run the static tool regularly during development
3. Periodically test in DEBUG mode to catch dynamically-generated strings
4. Keep language files synchronized across all languages

## Requirements

- Python 3.6 or higher
- No external dependencies (uses only Python standard library)

## Technical Details

### String Extraction Regex

For C++ files, the tool uses this regex pattern:
```python
app::i18n\s*\(\s*"([^"\\]*(?:\\.[^"\\]*)*)"\s*(?:,|\))
```

This pattern:
- Matches `app::i18n(` with optional whitespace
- Captures string content between double quotes
- Handles escaped characters (e.g., `\"`, `\n`, `\t`)
- Supports both single-argument and two-argument versions of `app::i18n()`

### JSON Formatting

Language files are formatted with:
- 1-space indentation (matching existing files)
- UTF-8 encoding
- Alphabetical sorting by key
- Final newline at end of file

## Troubleshooting

**Script reports "No language files found"**
- Ensure you're running from the repository root or tools directory
- Check that `./data/languages/` exists and contains `.json` files

**Some strings aren't being extracted**
- Check if they're passed as variables or function returns (not supported)
- Verify the syntax matches `app::i18n("string")`
- Check XML files have `text="..."` attributes (not `label` or other attributes)

**Language files not updating**
- Ensure JSON files are valid and parseable
- Check file permissions allow writing
- Look for error messages in the script output
