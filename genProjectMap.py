import os
import re
from collections import defaultdict
from pathlib import Path

# ================= 配置區（對齊 ProjectGirigara 目錄結構）=================
PROJECT_ROOT = Path(__file__).resolve().parent
OUTPUT_FILE = PROJECT_ROOT / "PROJECT_SIGNATURE_MAP.md"

INCLUDE_DIR = PROJECT_ROOT / "include"
SRC_DIR = PROJECT_ROOT / "src"
LUA_DIRS = [
    PROJECT_ROOT / "assets" / "sysScripts",
    PROJECT_ROOT / "assets" / "scripts",
]

# 排除第三方目錄與實作展開檔
IGNORE_SUBDIRS = {"glad", "KHR", "ThirdParty"}
IGNORE_FILES = {"glad.c", "glad.h", "khrplatform.h", "miniaudio_impl.cpp", "stb_image_impl.cpp"}
# =========================================================================

def strip_comments_cpp(text: str) -> str:
    text = re.sub(r'//.*', '', text)
    text = re.sub(r'/\*[\s\S]*?\*/', '', text)
    return text

def extract_local_includes(content: str) -> list[str]:
    """同時提取 "" 與 <> 的 #include，並精準驗證是否屬於專案 include/ 內的標頭檔"""
    includes = []
    for match in re.finditer(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', content, re.MULTILINE):
        inc_path = match.group(1)
        if Path(inc_path).name in IGNORE_FILES:
            continue
        # 驗證該路徑是否真的存在於專案的 include/ 目錄下，自動過濾掉 <vector>, <GLFW/glfw3.h> 等外部庫
        if (INCLUDE_DIR / inc_path).exists():
            includes.append(inc_path)
    return includes

def extract_cpp_header_signatures(content: str) -> list[str]:
    """提取 namespace, class, struct, enum, 前置宣告, 與成員/自由函式宣告"""
    clean = strip_comments_cpp(content)
    lines = clean.splitlines()
    results = []

    type_pattern = re.compile(
        r'^\s*(template\s*<[^>]+>\s*)?(class|struct|enum(\s+class)?|namespace)\s+([a-zA-Z0-9_]+)(.*\{|.*?;)?'
    )
    func_pattern = re.compile(
        r'^\s*(?!if\b|for\b|while\b|switch\b|return\b|typedef\b|using\b|#)'
        r'[a-zA-Z0-9_:<>&*\[\]\s~]+\s+[a-zA-Z0-9_~]+\s*\([^;{}]*\)\s*'
        r'(const)?\s*(noexcept)?\s*(override)?\s*(final)?\s*(=\s*0)?\s*;'
    )

    for line in lines:
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        if type_pattern.match(stripped) or func_pattern.match(stripped):
            results.append(re.sub(r'\s+', ' ', stripped))
    return results

def extract_cpp_source_signatures(content: str) -> list[str]:
    """提取 .cpp 中的自由函式（不含 :: 的全域函式，如 main 或 C 橋接）"""
    clean = strip_comments_cpp(content)
    results = []
    free_func_pattern = re.compile(
        r'^(?!if\b|for\b|while\b|switch\b|return\b)'
        r'[a-zA-Z0-9_<>&*\[\]\s]+\s+([a-zA-Z0-9_]+)\s*\([^;{}]*\)\s*(const)?\s*(noexcept)?\s*\{',
        re.MULTILINE
    )
    for match in free_func_pattern.finditer(clean):
        sig = match.group(0).rstrip('{').strip()
        if "::" not in sig and not sig.startswith(("if", "while", "for", "switch")):
            results.append(re.sub(r'\s+', ' ', sig))
    return results

def extract_lua_signatures(content: str) -> list[str]:
    """提取 Lua 全域與模組函式定義"""
    results = []
    func_pattern = re.compile(r'^\s*(local\s+)?function\s+([a-zA-Z0-9_.:]+)\s*\(([^)]*)\)', re.MULTILINE)
    for match in func_pattern.finditer(content):
        if not match.group(1):
            results.append(f"function {match.group(2)}({match.group(3).strip()})")
    return results

def scan_cpp_dir(base_dir: Path, is_header: bool, reverse_deps: dict) -> list[str]:
    sections = []
    if not base_dir.exists():
        print(f"[WARN] Directory not found: {base_dir}")
        return sections

    for root, dirs, files in os.walk(base_dir):
        dirs[:] = [d for d in sorted(dirs) if d not in IGNORE_SUBDIRS]
        for file in sorted(files):
            if file in IGNORE_FILES:
                continue
            filepath = Path(root) / file
            rel_path_str = str(filepath.relative_to(PROJECT_ROOT))
            content = filepath.read_text(encoding="utf-8", errors="ignore")
            includes = extract_local_includes(content)

            for inc in includes:
                reverse_deps[Path(inc).name].append(rel_path_str)

            sigs = extract_cpp_header_signatures(content) if is_header else extract_cpp_source_signatures(content)

            block = [f"### `{rel_path_str}`"]
            if includes:
                block.append(f"- **Includes:** `{', '.join(includes)}`")
            if sigs:
                block.append("```cpp")
                block.extend(sigs)
                block.append("```")
            sections.append("\n".join(block) + "\n")
    return sections

def generate_map():
    reverse_deps = defaultdict(list)

    header_sections = scan_cpp_dir(INCLUDE_DIR, is_header=True, reverse_deps=reverse_deps)
    source_sections = scan_cpp_dir(SRC_DIR, is_header=False, reverse_deps=reverse_deps)

    output_lines = [
        "# Project Girigara - Architecture & Signature Map",
        "> Auto-generated API, Include Dependency & Blast-Radius Map.\n",
        "## 1. Blast Radius (Reverse Include Index)",
        "> 修改某個標頭檔時，直接受影響需要檢視實作的檔案列表：\n"
    ]

    for header_name in sorted(reverse_deps.keys()):
        dependents = sorted(set(reverse_deps[header_name]))
        output_lines.append(f"- **`{header_name}`** ➔ {', '.join(f'`{d}`' for d in dependents)}")
    output_lines.append("")

    output_lines.append("## 2. C++ Headers & Core Interfaces (`include/`)\n")
    output_lines.extend(header_sections)

    output_lines.append("## 3. C++ Sources (`src/` Includes & Free Functions)\n")
    output_lines.extend(source_sections)

    output_lines.append("## 4. Lua Scripts Signatures (`assets/`)\n")
    for lua_dir in LUA_DIRS:
        if not lua_dir.exists():
            continue
        for file in sorted(os.listdir(lua_dir)):
            if file.endswith(".lua"):
                filepath = lua_dir / file
                rel_path_str = str(filepath.relative_to(PROJECT_ROOT))
                content = filepath.read_text(encoding="utf-8", errors="ignore")
                sigs = extract_lua_signatures(content)
                if sigs:
                    output_lines.append(f"### `{rel_path_str}`")
                    output_lines.append("```lua")
                    output_lines.extend(sigs)
                    output_lines.append("```\n")

    OUTPUT_FILE.write_text("\n".join(output_lines), encoding="utf-8")
    print(f"Done! Headers: {len(header_sections)}, Sources: {len(source_sections)} -> {OUTPUT_FILE} ({OUTPUT_FILE.stat().st_size / 1024:.1f} KB)")

if __name__ == "__main__":
    generate_map()
