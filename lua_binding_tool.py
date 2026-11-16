#!/usr/bin/env python3
import os
import re
from pathlib import Path
from dataclasses import dataclass, field

# 掃描根目錄與輸出路徑（可依實際專案目錄調整）
SRC_DIR = Path("src") if Path("src").exists() else Path(".")
OUT_STUB_FILE = Path("engine_api.lua")
OUT_AUDIT_FILE = Path("binding_audit.md")


@dataclass
class LuaParam:
    name: str
    lua_type: str = "any"
    optional: bool = False


@dataclass
class BindingRecord:
    lua_name: str
    cpp_func: str
    source_file: str
    reg_line: int
    impl_line: int | None = None
    has_valid_tag: bool = False
    raw_tag: str = ""
    tag_name_mismatch: bool = False
    params: list[LuaParam] = field(default_factory=list)
    returns: list[str] = field(default_factory=list)
    doc: str = ""


def line_number_at(content: str, pos: int) -> int:
    return content.count("\n", 0, pos) + 1


def parse_luafunc_tag(comment_block: str, expected_lua_name: str) -> tuple[bool, str, bool, list[LuaParam], list[str], str]:
    """
    僅解析統一格式的註解：
    // @luaFunc FuncName(param1: type, param2?: type) -> ret1, ret2
    以及緊接在前後的普通 // 說明文字
    """
    params: list[LuaParam] = []
    returns: list[str] = []
    doc_lines: list[str] = []
    raw_tag = ""
    has_valid_tag = False
    name_mismatch = False

    for line in comment_block.splitlines():
        clean = re.sub(r"^\s*//+\s*", "", line).strip()
        if not clean:
            continue
        if clean.startswith("@luaFunc"):
            raw_tag = clean
            sig_body = clean[len("@luaFunc"):].strip()
            m = re.match(r"^([A-Za-z0-9_.:]+)\s*\(([^)]*)\)\s*(?:->\s*(.+))?$", sig_body)
            if m:
                has_valid_tag = True
                annotated_name = m.group(1).strip()
                # 比對註解內的函數名稱（取最後一段以相容 Module.Func 或純 Func）與實際註冊的 Lua 名稱
                short_annotated = annotated_name.split(".")[-1].split(":")[-1]
                if annotated_name != expected_lua_name and short_annotated != expected_lua_name:
                    name_mismatch = True

                raw_args = m.group(2).strip()
                raw_ret = (m.group(3) or "").strip()

                if raw_args:
                    for arg in raw_args.split(","):
                        arg = arg.strip()
                        if not arg:
                            continue
                        if arg == "...":
                            params.append(LuaParam("...", "any", False))
                            continue
                        if ":" in arg:
                            p_name, p_type = [x.strip() for x in arg.split(":", 1)]
                            opt = p_name.endswith("?")
                            p_name = p_name.rstrip("?")
                            params.append(LuaParam(p_name, p_type, opt))
                        else:
                            opt = arg.endswith("?")
                            p_name = arg.rstrip("?")
                            params.append(LuaParam(p_name, "any", opt))

                if raw_ret:
                    for r in raw_ret.split(","):
                        r = r.strip()
                        if r:
                            returns.append(r)
        else:
            doc_lines.append(clean)

    return has_valid_tag, raw_tag, name_mismatch, params, returns, " ".join(doc_lines)


def scan_cpp_file(filepath: Path) -> list[BindingRecord]:
    content = filepath.read_text(encoding="utf-8", errors="ignore")
    if "RegisterScriptBindingEvent" not in content:
        return []

    records: list[BindingRecord] = []

    # 匹配 RegisterScriptBindingEvent{ ... } 內的 { "LuaName", cpp_func } 條目
    event_blocks = re.finditer(r"RegisterScriptBindingEvent\s*\{(.*?)\}\s*\)\s*;", content, re.DOTALL)
    entry_pattern = re.compile(r'\{\s*"([A-Za-z0-9_]+)"\s*,\s*&?([A-Za-z0-9_:]+)\s*\}')

    for block in event_blocks:
        block_inner = block.group(1)
        block_start = block.start(1)
        for m in entry_pattern.finditer(block_inner):
            lua_name = m.group(1)
            cpp_func_full = m.group(2)
            cpp_func = cpp_func_full.split("::")[-1]
            reg_line = line_number_at(content, block_start + m.start())

            # 搜尋同檔案內對應的 int cpp_func(lua_State* L) 定義與上方緊鄰註解
            fn_pattern = re.compile(
                rf"((?:^\s*//[^\n]*\n)*)^\s*(?:static\s+)?int\s+(?:[A-Za-z0-9_]+::)?{re.escape(cpp_func)}\s*\(\s*lua_State\s*\*",
                re.MULTILINE,
            )
            fn_match = fn_pattern.search(content)

            if fn_match:
                comment_block = fn_match.group(1) or ""
                # 計算函數簽名本體所在行號（跳過上方註解行）
                impl_offset = fn_match.start() + len(comment_block)
                impl_line = line_number_at(content, impl_offset)
                has_valid_tag, raw_tag, mismatch, params, returns, doc = parse_luafunc_tag(
                    comment_block, lua_name
                )
                records.append(
                    BindingRecord(
                        lua_name=lua_name,
                        cpp_func=cpp_func_full,
                        source_file=str(filepath),
                        reg_line=reg_line,
                        impl_line=impl_line,
                        has_valid_tag=has_valid_tag,
                        raw_tag=raw_tag,
                        tag_name_mismatch=mismatch,
                        params=params,
                        returns=returns,
                        doc=doc,
                    )
                )
            else:
                # 找不到實作（可能定義在別的檔案或名稱不符）
                records.append(
                    BindingRecord(
                        lua_name=lua_name,
                        cpp_func=cpp_func_full,
                        source_file=str(filepath),
                        reg_line=reg_line,
                        impl_line=None,
                    )
                )

    return records


def write_audit_report(records: list[BindingRecord], out_path: Path):
    out_path.parent.mkdir(parents=True, exist_ok=True)
    missing_tag = [r for r in records if r.impl_line is not None and not r.has_valid_tag]
    mismatched = [r for r in records if r.has_valid_tag and r.tag_name_mismatch]
    missing_impl = [r for r in records if r.impl_line is None]
    healthy = [r for r in records if r.has_valid_tag and not r.tag_name_mismatch]

    lines = [
        "# Lua C++ 綁定函數位置與 `@luaFunc` 註解稽核報告",
        "",
        f"- 總偵測綁定數：**{len(records)}**",
        f"- 格式正確且名稱一致：**{len(healthy)}**",
        f"- 缺少或格式不符 `@luaFunc` 註解：**{len(missing_tag)}**",
        f"- 註解函數名稱與實際註冊不一致：**{len(mismatched)}**",
        f"- 找不到同檔 C++ 實作函數：**{len(missing_impl)}**",
        "",
        "---",
        "",
        "## 1. 需要補上或修正 `@luaFunc` 註解的綁定清單",
        "",
        "| 狀態 | 檔案位置 | 實作行號 | 註冊行號 | 實際註冊 Lua 名稱 | C++ 函數名稱 | 現有註解內容 |",
        "| :--- | :--- | :---: | :---: | :--- | :--- | :--- |",
    ]

    flagged = missing_tag + mismatched + missing_impl
    if not flagged:
        lines.append("| OK | - | - | - | - | - | 全部綁定皆已具備正確的 `@luaFunc` 註解 |")
    else:
        for r in flagged:
            if r.impl_line is None:
                status = "找不到實作"
                impl_str = "N/A"
            elif not r.has_valid_tag:
                status = "缺少/舊格式註解"
                impl_str = f"L{r.impl_line}"
            else:
                status = "名稱不一致"
                impl_str = f"L{r.impl_line}"
            tag_preview = f"`{r.raw_tag}`" if r.raw_tag else "(無 `@luaFunc`)"
            lines.append(
                f"| {status} | `{r.source_file}` | {impl_str} | L{r.reg_line} | `{r.lua_name}` | `{r.cpp_func}` | {tag_preview} |"
            )

    lines.extend([
        "",
        "---",
        "",
        "## 2. 所有已偵測之 C++ 綁定函數完整位置總表",
        "",
        "| 檔案路徑 | 實作位置 | 註冊位置 | Lua 函數名稱 | C++ 綁定函數 | `@luaFunc` 狀態 |",
        "| :--- | :---: | :---: | :--- | :--- | :---: |",
    ])

    for r in records:
        impl_str = f"`{r.source_file}:{r.impl_line}`" if r.impl_line else "未在同檔找到"
        reg_str = f"`{r.source_file}:{r.reg_line}`"
        if r.has_valid_tag and not r.tag_name_mismatch:
            st = "已標準化"
        elif r.tag_name_mismatch:
            st = "名稱不符"
        else:
            st = "待更新"
        lines.append(f"| `{r.source_file}` | {impl_str} | {reg_str} | `{r.lua_name}` | `{r.cpp_func}` | {st} |")

    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_lua_stubs(records: list[BindingRecord], out_path: Path):
    out_path.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        "---@meta",
        "-- Auto-generated by gen_lua_stubs.py from // @luaFunc annotations.",
        "-- DO NOT EDIT MANUALLY.",
        "",
    ]

    current_file = None
    for r in records:
        if r.source_file != current_file:
            current_file = r.source_file
            lines.append(f"-- Source: {current_file}")

        if r.doc:
            lines.append(f"---{r.doc}")

        if not r.has_valid_tag:
            lines.append(f"---[TODO: Missing @luaFunc annotation at {r.source_file}:{r.impl_line or r.reg_line}]")
            lines.append(f"---@param ... any")
            lines.append(f"---@return any")
            lines.append(f"function {r.lua_name}(...) end")
            lines.append("")
            continue

        for p in r.params:
            opt = "?" if p.optional else ""
            lines.append(f"---@param {p.name}{opt} {p.lua_type}")

        for ret in r.returns:
            lines.append(f"---@return {ret}")

        arg_list = ", ".join(p.name for p in r.params)
        lines.append(f"function {r.lua_name}({arg_list}) end")
        lines.append("")

    out_path.write_text("\n".join(lines), encoding="utf-8")


def main():
    all_records: list[BindingRecord] = []
    for root, _, files in os.walk(SRC_DIR):
        for fname in sorted(files):
            if fname.endswith((".cpp", ".cc", ".cxx")):
                all_records.extend(scan_cpp_file(Path(root) / fname))

    write_audit_report(all_records, OUT_AUDIT_FILE)
    write_lua_stubs(all_records, OUT_STUB_FILE)
    print(f"[gen_lua_stubs] Scanned {len(all_records)} bindings.")
    print(f"  -> Audit report : {OUT_AUDIT_FILE}")
    print(f"  -> LuaCATS stub : {OUT_STUB_FILE}")


if __name__ == "__main__":
    main()
