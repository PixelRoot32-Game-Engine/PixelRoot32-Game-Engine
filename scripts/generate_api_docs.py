#!/usr/bin/env python3
"""
PixelRoot32 API Documentation Generator

Parses C++ header files and generates Markdown API documentation for VitePress.
Output goes to docs/api/generated/ (isolated from conceptual docs).

Public inheritance (`class Derived : public Base`) is read from the declaration and
rendered as **Inherits from** / ## Inheritance; Doxygen "Inherits from …" lines in
the body are omitted when the base was detected from code to avoid duplication.

Usage:
    python scripts/generate_api_docs.py
"""

import os
import re
import json
import shutil
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Tuple
from collections import defaultdict


@dataclass
class DocComment:
    """Represents a parsed documentation comment."""
    brief: str = ""
    params: Dict[str, str] = field(default_factory=dict)
    return_doc: str = ""
    notes: List[str] = field(default_factory=list)
    warnings: List[str] = field(default_factory=list)
    description: str = ""  # Non-brief text after @brief


@dataclass
class Method:
    """Represents a parsed method."""
    name: str = ""
    signature: str = ""
    doc: DocComment = field(default_factory=DocComment)
    is_virtual: bool = False
    is_static: bool = False
    is_constructor: bool = False
    is_destructor: bool = False


@dataclass
class Property:
    """Represents a parsed property/field."""
    name: str = ""
    type: str = ""
    doc: str = ""


@dataclass
class ClassDoc:
    """Represents a parsed class/struct/enum."""
    name: str = ""
    type: str = "class"  # class, struct, enum
    doc: DocComment = field(default_factory=DocComment)
    methods: List[Method] = field(default_factory=list)
    properties: List[Property] = field(default_factory=list)
    source_file: str = ""
    namespace: str = ""
    # Short base name parsed from `class Derived : public Base` (source of truth)
    base_class: Optional[str] = None


def parse_doc_comment(comment_text: str) -> DocComment:
    """Parse a /** */ style documentation comment."""
    doc = DocComment()
    
    # Remove comment markers and leading asterisks
    lines = comment_text.split('\n')
    cleaned_lines = []
    for line in lines:
        # Remove /** and */
        line = re.sub(r'^\s*/\*\*?\s*', '', line)
        line = re.sub(r'\s*\*/\s*$', '', line)
        # Remove leading * and whitespace
        line = re.sub(r'^\s*\*\s?', '', line)
        cleaned_lines.append(line)
    
    text = '\n'.join(cleaned_lines).strip()
    
    # Remove @class, @struct, @enum tags first
    text = re.sub(r'@class\s+\w+', '', text)
    text = re.sub(r'@struct\s+\w+', '', text)
    text = re.sub(r'@enum\s+\w+', '', text)
    text = text.strip()
    
    # Extract @brief (stop at blank line or next @-tag; avoids swallowing body text with DOTALL)
    brief_match = re.search(
        r'@brief\s+(.+?)(?=\n\s*\n|\n\s*@\w|\Z)',
        text,
        re.DOTALL,
    )
    if brief_match:
        doc.brief = brief_match.group(1).strip()
    
    # Extract @param (clean up any trailing @return in the description)
    param_matches = re.findall(r'@param\s+(\w+)\s+(.+?)(?=\n\s*@|\Z)', text, re.DOTALL)
    for name, desc in param_matches:
        # Remove any embedded @return or @note from param description
        clean_desc = re.sub(r'\s*@\w+.*$', '', desc, flags=re.DOTALL).strip()
        doc.params[name] = clean_desc
    
    # Extract @return
    return_match = re.search(r'@return\s+(.+?)(?=\n\s*@|\Z)', text, re.DOTALL)
    if return_match:
        doc.return_doc = return_match.group(1).strip()
    
    # Extract @note (multiple allowed)
    note_matches = re.findall(r'@note\s+(.+?)(?=\n\s*@|\Z)', text, re.DOTALL)
    doc.notes = [n.strip() for n in note_matches]
    
    # Extract @warning (multiple allowed)
    warning_matches = re.findall(r'@warning\s+(.+?)(?=\n\s*@|\Z)', text, re.DOTALL)
    doc.warnings = [w.strip() for w in warning_matches]
    
    # Any text before first @ tag (excluding @brief content and tags) is extra description
    # Remove all @ tags and their content to get the description text
    desc_text = re.sub(r'@\w+\s+', '', text)
    # Remove brief content from description
    if brief_match:
        desc_text = desc_text.replace(brief_match.group(1).strip(), '')
    desc_text = desc_text.strip()
    # Filter out common leftover artifacts
    desc_text = re.sub(r'^(In\s+)?Phase\s+\d+.*$', '', desc_text, flags=re.MULTILINE).strip()
    if desc_text:
        doc.description = desc_text
    
    return doc


def extract_public_base_class(decl_before_open_brace: str, class_name: str, kind: str) -> Optional[str]:
    """
    Parse the first `public` base from a class/struct declaration (before '{').
    Returns the short name (last :: segment), e.g. pixelroot32::core::PhysicsActor -> PhysicsActor.
    """
    if kind == "enum":
        return None
    normalized = " ".join(decl_before_open_brace.split())
    match = re.search(
        rf"\b(?:class|struct)\s+{re.escape(class_name)}\b(?:\s+final)?\s*:\s*public\s+([\w:]+)",
        normalized,
    )
    if not match:
        return None
    full = match.group(1).strip()
    return full.split("::")[-1]


def strip_redundant_inherit_comment(description: str, has_code_derived_base: bool) -> str:
    """Remove 'Inherits from X.' lines from Doxygen body when base comes from the declaration."""
    if not description or not has_code_derived_base:
        return description
    cleaned = re.sub(r"(?m)^\s*Inherits from\s+[\w:]+\.?\s*\n?", "", description)
    return cleaned.strip()


def build_class_index(modules: Dict[str, List[ClassDoc]]) -> Dict[str, Tuple[str, str]]:
    """Map class short name -> (module_dir, class_name) for markdown linking (first wins)."""
    index: Dict[str, Tuple[str, str]] = {}
    for module, classes in modules.items():
        for cls in classes:
            if cls.name not in index:
                index[cls.name] = (module, cls.name)
    return index


def format_base_class_markdown(base_short: str, from_module: str, class_index: Dict[str, Tuple[str, str]]) -> str:
    """**Inherits from:** line with link to generated doc when available."""
    if base_short not in class_index:
        return f"**Inherits from:** {base_short}"
    mod, cn = class_index[base_short]
    if mod == from_module:
        link = f"./{cn}.md"
    else:
        link = f"../{mod}/{cn}.md"
    return f"**Inherits from:** [{base_short}]({link})"


def extract_method_signature(line: str) -> Optional[Tuple[str, str, bool, bool, bool, bool]]:
    """
    Extract method information from a line.
    Returns (name, signature, is_virtual, is_static, is_constructor, is_destructor) or None.
    """
    original_line = line.strip()
    
    # Skip lines that look like code statements (not declarations)
    if re.match(r'^return\s+', original_line):
        return None
    if re.match(r'^\{', original_line):  # Opening brace
        return None
    if '=' in original_line and '(' not in original_line.split('=')[0]:
        # Assignment statement like "int x = 5;" but not "operator="
        return None
    
    # Check for virtual
    is_virtual = 'virtual ' in line
    line = line.replace('virtual ', '')
    
    # Check for static
    is_static = line.strip().startswith('static ')
    line = line.replace('static ', '')
    
    # Check for explicit override/final and pure virtual
    line = re.sub(r'\s+override\s*$', '', line)
    line = re.sub(r'\s+final\s*$', '', line)
    line = re.sub(r'\s*=\s*0\s*;', ';', line)  # Pure virtual
    
    # Skip if line contains function body indicators
    if '{' in line and ';' not in line.split('{')[0]:
        # Likely a function definition with body, not just declaration
        pass  # Still check if it ends with semicolon after body
    
    # Match method signature: return_type name(params) [const];
    # Handle templates, pointers, references, etc.
    # Can end with semicolon OR have inline body { ... }
    stripped = line.strip()
    
    # Check if this looks like an inline implementation (ends with })
    # but has proper signature pattern before the body
    has_inline_body = stripped.endswith('}') and '{' in stripped
    
    # For inline methods, we need to extract just the signature part before { or :
    # First, try to match the signature part only (before { or :)
    sig_line = stripped
    if ':' in stripped and ('{' not in stripped or stripped.find(':') < stripped.find('{')):
        # Constructor with initializer list - cut before :
        sig_line = stripped.split(':')[0].strip()
    elif '{' in stripped:
        # Inline function body - cut before {
        sig_line = stripped.split('{')[0].strip()
    
    method_pattern = r'^(?:template\s*<[^>]+>\s*)?'  # Optional template
    method_pattern += r'([\w:<>,\s&*~]+?)'  # Return type (non-greedy), allow ~ for destructor
    method_pattern += r'\s+([\w~]+)\s*'  # Method name (allow ~ for destructor)
    method_pattern += r'\((.*?)\)'  # Parameters (non-greedy)
    method_pattern += r'\s*(const)?'  # Optional const
    method_pattern += r'(?:\s*=\s*0)?'  # Optional pure virtual = 0
    method_pattern += r'(?:\s*;)?$'  # Optional semicolon at end
    
    match = re.match(method_pattern, sig_line)
    if not match:
        return None
    
    return_type = match.group(1).strip()
    name = match.group(2)
    params = match.group(3)
    is_const = bool(match.group(4))
    
    # Handle destructor
    is_destructor = name.startswith('~')
    is_constructor = not is_destructor and return_type == ""
    
    # Clean up return type for constructors
    if is_constructor or is_destructor:
        sig = f"{name}({params})"
    else:
        const_suffix = " const" if is_const else ""
        sig = f"{return_type} {name}({params}){const_suffix}"
    
    return (name, sig, is_virtual, is_static, is_constructor, is_destructor)


def parse_header_file(file_path: str) -> List[ClassDoc]:
    """Parse a C++ header file and extract documentation."""
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    classes = []
    
    # Find all doc comments
    doc_pattern = r'/\*\*(.*?)\*/'
    doc_comments = list(re.finditer(doc_pattern, content, re.DOTALL))
    
    # Track position for finding what follows each doc comment
    for i, doc_match in enumerate(doc_comments):
        doc_end = doc_match.end()
        doc_text = doc_match.group(1)
        
        # Look at what follows this doc comment
        following_text = content[doc_end:doc_end + 1000]
        following_lines = following_text.split('\n')
        
        # Check for @class, @struct, @enum in the doc comment
        class_match = re.search(r'@class\s+(\w+)', doc_text)
        struct_match = re.search(r'@struct\s+(\w+)', doc_text)
        enum_match = re.search(r'@enum\s+(\w+)', doc_text)
        
        if class_match or struct_match or enum_match:
            # This doc comment documents a class/struct/enum
            name = (class_match or struct_match or enum_match).group(1)
            type_str = "class" if class_match else ("struct" if struct_match else "enum")
            
            class_doc = ClassDoc(
                name=name,
                type=type_str,
                doc=parse_doc_comment(doc_match.group(0)),
                source_file=os.path.basename(file_path)
            )
            
            # Find the class/struct/enum definition and parse its contents
            # Look for: class Name { ... };
            class_def_pattern = rf'(class|struct|enum)\s+{re.escape(name)}\b[^{{]*\{{'
            class_def_match = re.search(class_def_pattern, content[doc_end:])
            
            if class_def_match:
                decl_start = doc_end + class_def_match.start()
                decl_end = doc_end + class_def_match.end() - 1  # exclude '{'
                decl_text = content[decl_start:decl_end]
                base_short = extract_public_base_class(decl_text, name, type_str)
                if base_short:
                    class_doc.base_class = base_short

                # Find matching closing brace
                start_idx = doc_end + class_def_match.end() - 1  # Position of {
                brace_count = 1
                end_idx = start_idx + 1
                while brace_count > 0 and end_idx < len(content):
                    if content[end_idx] == '{':
                        brace_count += 1
                    elif content[end_idx] == '}':
                        brace_count -= 1
                    end_idx += 1
                
                class_body = content[start_idx + 1:end_idx - 1]
                
                # Parse class body for methods and properties
                parse_class_body(class_body, class_doc)
            
            classes.append(class_doc)
        else:
            # Check if this doc comment precedes a method
            # Look for method signature in following lines
            method_found = False
            for line in following_lines[:5]:
                line = line.strip()
                if not line or line.startswith('//'):
                    continue
                if line.startswith('/*'):
                    break
                
                # Check for method signature
                method_info = extract_method_signature(line)
                if method_info and classes:
                    # Associate with the most recent class
                    name, sig, is_virt, is_static, is_ctor, is_dtor = method_info
                    method = Method(
                        name=name,
                        signature=sig,
                        doc=parse_doc_comment(doc_match.group(0)),
                        is_virtual=is_virt,
                        is_static=is_static,
                        is_constructor=is_ctor,
                        is_destructor=is_dtor
                    )
                    classes[-1].methods.append(method)
                    method_found = True
                    break
            
            # Note: Properties are now only parsed in parse_class_body, not here
            # to avoid duplication and ensure correct access level detection
    
    return classes


def parse_class_body(body: str, class_doc: ClassDoc):
    """Parse the body of a class/struct to extract methods and properties."""
    lines = body.split('\n')
    # C++ default access: class → private until public:/protected:; struct/enum → public
    current_access = "private" if class_doc.type == "class" else "public"
    
    i = 0
    while i < len(lines):
        line = lines[i].strip()
        
        # Track access specifier
        if line.startswith('public:'):
            current_access = "public"
            i += 1
            continue
        elif line.startswith('private:'):
            current_access = "private"
            i += 1
            continue
        elif line.startswith('protected:'):
            current_access = "protected"
            i += 1
            continue
        
        # Skip private/protected members
        if current_access != "public":
            i += 1
            continue
        
        # Look for inline property documentation with ///<
        # Match patterns like: "Type name; ///< doc" or "Type name = value; ///< doc"
        # Handle multi-variable declarations like "int x, y; ///< pos" - capture first only
        # The key is to stop at the FIRST variable name (before comma if present)
        inline_prop_match = re.match(
            r'^\s*([\w:<>,\s&*]+?)\s+(\w+)\b',  # Type and first variable name (word boundary)
            line
        )
        if inline_prop_match and '///<' in line:
            # Extract the doc part after ///<
            doc_match = re.search(r'///<\s*(.+)$', line)
            if doc_match:
                full_before_semicolon = line.split(';')[0] if ';' in line else line.split('///<')[0]
                
                # Parse type and first variable name
                type_var_match = re.match(r'^\s*([\w:<>,\s&*]+?)\s+(\w+)\b', full_before_semicolon)
                if type_var_match:
                    prop_type = type_var_match.group(1).strip()
                    prop_name = type_var_match.group(2)
                    prop_doc = doc_match.group(1).strip()
                    
                    # Filter out obvious non-properties and keywords
                    invalid_names = {'if', 'for', 'while', 'return', 'class', 'struct', 'switch', 'case'}
                    if prop_name not in invalid_names and not any(p.name == prop_name for p in class_doc.properties):
                        prop = Property(name=prop_name, type=prop_type, doc=prop_doc)
                        class_doc.properties.append(prop)
        
        # Look for methods without preceding doc comments
        method_info = extract_method_signature(line)
        if method_info:
            name, sig, is_virt, is_static, is_ctor, is_dtor = method_info
            
            # Check if method with same name already exists
            # If it does and the existing one has no documentation, we might want to keep both
            # or replace if signatures match closely
            existing_idx = None
            for idx, m in enumerate(class_doc.methods):
                if m.name == name:
                    # Normalize signatures for comparison (remove const, spaces)
                    sig_clean = re.sub(r'\s+const\s*$', '', sig).replace(' ', '')
                    existing_clean = re.sub(r'\s+const\s*$', '', m.signature).replace(' ', '')
                    if sig_clean == existing_clean:
                        existing_idx = idx
                        break
            
            if existing_idx is None:
                # New method - add it
                method = Method(
                    name=name,
                    signature=sig,
                    doc=DocComment(),
                    is_virtual=is_virt,
                    is_static=is_static,
                    is_constructor=is_ctor,
                    is_destructor=is_dtor
                )
                class_doc.methods.append(method)
        
        i += 1


def deduplicate_methods(methods: List[Method]) -> List[Method]:
    """Deduplicate methods, keeping documented versions when available."""
    seen = {}  # signature -> (method, has_docs)
    
    for method in methods:
        # Normalize signature for comparison
        sig_clean = re.sub(r'\s+const\s*$', '', method.signature).replace(' ', '')
        
        has_docs = bool(method.doc.brief or method.doc.params or method.doc.return_doc or 
                        method.doc.notes or method.doc.warnings)
        
        if sig_clean not in seen:
            seen[sig_clean] = (method, has_docs)
        else:
            existing_method, existing_has_docs = seen[sig_clean]
            # Keep the documented version
            if has_docs and not existing_has_docs:
                seen[sig_clean] = (method, has_docs)
    
    return [m for m, _ in seen.values()]


def generate_class_markdown(
    class_doc: ClassDoc,
    class_index: Dict[str, Tuple[str, str]],
) -> str:
    """Generate Markdown documentation for a class."""
    # Deduplicate methods before generating
    class_doc.methods = deduplicate_methods(class_doc.methods)

    from_module = class_doc.namespace

    lines = []

    # Title
    lines.append(f"# {class_doc.name}")
    lines.append("")

    # Type badge
    type_label = class_doc.type.capitalize()
    lines.append(f'<Badge type="info" text="{type_label}" />')
    lines.append("")

    # Source file
    lines.append(f"**Source:** `{class_doc.source_file}`")
    lines.append("")

    # Inheritance from C++ declaration (`class X : public Y`)
    if class_doc.base_class:
        lines.append(format_base_class_markdown(class_doc.base_class, from_module, class_index))
        lines.append("")

    extra_description = strip_redundant_inherit_comment(
        class_doc.doc.description, bool(class_doc.base_class)
    )

    # Description
    if class_doc.doc.brief:
        lines.append("## Description")
        lines.append("")
        lines.append(class_doc.doc.brief)
        lines.append("")

    if extra_description:
        lines.append(extra_description)
        lines.append("")

    if class_doc.base_class:
        lines.append("## Inheritance")
        lines.append("")
        base_md = format_base_class_markdown(class_doc.base_class, from_module, class_index)
        # Turn "**Inherits from:** …" into chain fragment (linked base + arrow + derived)
        base_part = base_md.replace("**Inherits from:** ", "", 1)
        lines.append(f"{base_part} → `{class_doc.name}`")
        lines.append("")
    
    # Warnings
    for warning in class_doc.doc.warnings:
        lines.append(f"::: warning")
        lines.append(warning)
        lines.append(":::")
        lines.append("")
    
    # Notes
    for note in class_doc.doc.notes:
        lines.append(f"::: tip")
        lines.append(note)
        lines.append(":::")
        lines.append("")
    
    # Properties
    if class_doc.properties:
        lines.append("## Properties")
        lines.append("")
        lines.append("| Name | Type | Description |")
        lines.append("|------|------|-------------|")
        for prop in class_doc.properties:
            # Escape pipe characters in type
            safe_type = prop.type.replace('|', '\\|')
            lines.append(f"| `{prop.name}` | `{safe_type}` | {prop.doc} |")
        lines.append("")
    
    # Methods
    if class_doc.methods:
        lines.append("## Methods")
        lines.append("")
        
        for method in class_doc.methods:
            # Skip destructor display
            if method.is_destructor:
                continue
            
            # Method signature as heading
            sig_display = method.signature
            if method.is_virtual:
                sig_display = "virtual " + sig_display
            if method.is_static:
                sig_display = "static " + sig_display
            
            lines.append(f"### `{sig_display}`")
            lines.append("")
            
            # Description
            if method.doc.brief:
                lines.append("**Description:**")
                lines.append("")
                lines.append(method.doc.brief)
                lines.append("")
            
            # Parameters
            if method.doc.params:
                lines.append("**Parameters:**")
                lines.append("")
                for param_name, param_desc in method.doc.params.items():
                    lines.append(f"- `{param_name}`: {param_desc}")
                lines.append("")
            
            # Return value
            if method.doc.return_doc:
                lines.append(f"**Returns:** {method.doc.return_doc}")
                lines.append("")
            
            # Notes
            for note in method.doc.notes:
                lines.append(f"::: tip")
                lines.append(note)
                lines.append(":::")
                lines.append("")
            
            # Warnings
            for warning in method.doc.warnings:
                lines.append(f"::: warning")
                lines.append(warning)
                lines.append(":::")
                lines.append("")
    
    return '\n'.join(lines)


def scan_include_directory(include_dir: str) -> Dict[str, List[ClassDoc]]:
    """Scan include directory and parse all header files."""
    modules = defaultdict(list)
    
    for root, dirs, files in os.walk(include_dir):
        # Determine module name from directory
        rel_path = os.path.relpath(root, include_dir)
        module = rel_path.split(os.sep)[0] if rel_path != '.' else 'core'
        
        for file in files:
            if file.endswith('.h'):
                file_path = os.path.join(root, file)
                try:
                    classes = parse_header_file(file_path)
                    for cls in classes:
                        cls.namespace = module
                        modules[module].append(cls)
                except Exception as e:
                    print(f"Warning: Error parsing {file_path}: {e}")
    
    return dict(modules)


def generate_index_markdown(modules: Dict[str, List[ClassDoc]], output_dir: str) -> str:
    """Generate index.md with table of contents."""
    lines = []
    
    lines.append("# API Reference (Generated)")
    lines.append("")
    lines.append("Auto-generated API documentation from C++ header files.")
    lines.append("")
    
    # Table of contents by module
    for module in sorted(modules.keys()):
        classes = modules[module]
        if not classes:
            continue
        
        lines.append(f"## {module.capitalize()}")
        lines.append("")
        
        # List classes with links
        for cls in sorted(classes, key=lambda c: c.name):
            rel_path = f"./{module}/{cls.name}.md"
            lines.append(f"- [{cls.name}]({rel_path}) — {cls.doc.brief or 'No description'}")
        
        lines.append("")
    
    return '\n'.join(lines)


def main():
    """Main entry point."""
    # Paths
    engine_root = Path(__file__).parent.parent
    include_dir = engine_root / 'include'
    output_dir = engine_root / 'docs' / 'api' / 'generated'
    
    print(f"PixelRoot32 API Documentation Generator")
    print(f"======================================")
    print(f"Input:  {include_dir}")
    print(f"Output: {output_dir}")
    print()
    
    # Clean and recreate output directory
    if output_dir.exists():
        shutil.rmtree(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Scan and parse headers
    print("Scanning header files...")
    modules = scan_include_directory(str(include_dir))

    class_index = build_class_index(modules)

    total_classes = sum(len(classes) for classes in modules.values())
    print(f"Found {total_classes} documented classes/structs/enums across {len(modules)} modules")
    print()

    # Generate documentation for each module
    for module, classes in modules.items():
        if not classes:
            continue

        # Create module directory
        module_dir = output_dir / module
        module_dir.mkdir(exist_ok=True)

        print(f"Generating docs for module: {module}")

        for cls in classes:
            markdown = generate_class_markdown(cls, class_index)
            output_file = module_dir / f"{cls.name}.md"
            
            with open(output_file, 'w', encoding='utf-8') as f:
                f.write(markdown)
    
    # Generate index
    print()
    print("Generating index...")
    index_content = generate_index_markdown(modules, str(output_dir))
    index_file = output_dir / 'index.md'
    
    with open(index_file, 'w', encoding='utf-8') as f:
        f.write(index_content)
    
    print()
    print("Done!")
    print(f"Generated documentation in: {output_dir}")
    print(f"  - {len(list(output_dir.glob('**/index.md')))} index file")
    print(f"  - {len(list(output_dir.glob('**/*.md'))) - 1} class documentation files")
    print(f"  - {len(list(output_dir.iterdir())) - 1} module directories")


if __name__ == '__main__':
    main()
