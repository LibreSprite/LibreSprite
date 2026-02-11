#!/usr/bin/env python3

import json
import re
import sys
from pathlib import Path
from typing import Set
import xml.etree.ElementTree as ETree


class I18nExtractor:
    """Extracts i18n strings from C++ and XML files."""
    
    def __init__(self, repo_root: Path):
        self.repo_root = repo_root
        self.src_dir = repo_root / "src"
        self.data_dir = repo_root / "data"
        self.widgets_dir = repo_root / "data" / "widgets"
        self.languages_dir = repo_root / "data" / "languages"
        self.strings: Set[str] = set()
    
    def extract_from_cpp(self) -> Set[str]:
        """Extract strings from i18n() calls in C++ files."""
        strings = set()
        
        # Pattern to match i18n("string") - direct string literals
        # This handles both single and double quotes, and escaped quotes
        direct_pattern = re.compile(
            r'(app::)?i18n\s*\(\s*"([^"\\]*(?:\\.[^"\\]*)*)"\s*[,)]',
            re.MULTILINE
        )
        
        # Find all .cpp and .h files
        cpp_files = list(self.src_dir.rglob("*.cpp")) + list(self.src_dir.rglob("*.h"))
        
        print(f"Scanning {len(cpp_files)} C++ files...")
        
        for cpp_file in cpp_files:
            try:
                content = cpp_file.read_text(encoding='utf-8', errors='ignore')
                
                # Find direct string literals
                for match in direct_pattern.finditer(content):
                    string_literal = match.group(1)
                    # Unescape the string
                    unescaped = string_literal.replace('\\"', '"').replace('\\n', '\n').replace('\\t', '\t')
                    if unescaped.strip():  # Only add non-empty strings
                        strings.add(unescaped)
                        
            except Exception as e:
                print(f"Warning: Could not read {cpp_file}: {e}", file=sys.stderr)
        
        print(f"Found {len(strings)} strings in C++ files")
        return strings
    
    def extract_from_xml(self) -> Set[str]:
        """Extract text attributes from XML widget files and gui.xml."""
        strings = set()
        
        # Get all XML files in widgets directory
        xml_files = list(self.widgets_dir.rglob("*.xml"))
        
        # Also scan gui.xml in data directory which contains tool definitions
        gui_xml = self.data_dir / "gui.xml"
        if gui_xml.exists():
            xml_files.append(gui_xml)
        
        print(f"Scanning {len(xml_files)} XML files...")
        
        for xml_file in xml_files:
            try:
                tree = ETree.parse(xml_file)
                root = tree.getroot()
                
                # Find all elements with text attribute
                for elem in root.iter():
                    text_attr = elem.get('text')
                    if text_attr and text_attr.strip():
                        # Strip '&' characters as they are removed by Widget::setI18N()
                        # before using the text as an i18n key
                        text_without_ampersand = text_attr.replace('&', '')
                        if text_without_ampersand.strip():  # Only add non-empty strings
                            strings.add(text_without_ampersand)
                        
            except Exception as e:
                print(f"Warning: Could not parse {xml_file}: {e}", file=sys.stderr)
        
        print(f"Found {len(strings)} strings in XML files")
        return strings
    
    def extract_all(self) -> Set[str]:
        """Extract all strings from both C++ and XML files."""
        cpp_strings = self.extract_from_cpp()
        xml_strings = self.extract_from_xml()
        
        all_strings = cpp_strings | xml_strings
        print(f"\nTotal unique strings to translate: {len(all_strings)}")
        
        return all_strings
    
    def update_language_files(self, strings: Set[str]) -> None:
        """Update all language JSON files with missing entries."""
        
        # Get all language JSON files
        lang_files = list(self.languages_dir.glob("*.json"))
        
        if not lang_files:
            print("Error: No language files found!", file=sys.stderr)
            return
        
        print(f"\nUpdating {len(lang_files)} language files...")
        
        for lang_file in lang_files:
            try:
                # Read existing translations
                with open(lang_file, 'r', encoding='utf-8') as f:
                    content = f.read()
                    # Parse JSON, preserving any formatting
                    translations = json.loads(content)
                
                # Track how many new entries we add
                added_count = 0
                
                # Add missing strings
                for string in strings:
                    if string not in translations:
                        # New entry: value equals key
                        translations[string] = string
                        added_count += 1
                
                if added_count > 0:
                    # Sort dictionary by keys alphabetically
                    sorted_translations = dict(sorted(translations.items()))
                    
                    # Write back to file with pretty formatting
                    with open(lang_file, 'w', encoding='utf-8') as f:
                        json.dump(sorted_translations, f, ensure_ascii=False, indent=1)
                        f.write('\n')  # Add final newline
                    
                    print(f"  {lang_file.name}: Added {added_count} new entries")
                else:
                    print(f"  {lang_file.name}: No updates needed")
                    
            except json.JSONDecodeError as e:
                print(f"Error: Could not parse {lang_file}: {e}", file=sys.stderr)
            except Exception as e:
                print(f"Error: Could not update {lang_file}: {e}", file=sys.stderr)


def main():
    """Main entry point."""
    # Determine repository root (script is in tools/ subdirectory)
    script_dir = Path(__file__).parent
    repo_root = script_dir.parent
    
    print("I18N String Extraction Tool")
    print("=" * 50)
    print(f"Repository root: {repo_root}")
    print()
    
    # Create extractor
    extractor = I18nExtractor(repo_root)
    
    # Extract all strings
    all_strings = extractor.extract_all()
    
    # Update language files
    extractor.update_language_files(all_strings)
    
    print("\nDone!")

if __name__ == "__main__":
    main()
