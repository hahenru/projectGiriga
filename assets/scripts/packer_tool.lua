-- assets/scripts/packer_tool.lua
-- Project Girigara 資源封包加解密工具 (GUI 雙分頁版)

local State = {
    activeTab = "pack", -- "pack" | "unpack"
    selectedType = "imz" -- "imz" | "luz" | "dtz" | "auz"
}

local UI = {
    tabPackBtn = nil,
    tabUnpackBtn = nil,
    packPage = nil,
    unpackPage = nil,
    -- 打包頁面元件
    packSrcInput = nil,
    packOutInput = nil,
    packKeyInput = nil,
    typeBtns = {},
    -- 解包頁面元件
    unpackFileInput = nil,
    unpackOutInput = nil,
    unpackKeyInput = nil,
    previewText = nil,
    -- 底部狀態列
    statusText = nil
}

local TYPE_DEFS = {
    {id = "imz", label = "圖片 (.imz)"},
    {id = "luz", label = "腳本 (.luz)"},
    {id = "dtz", label = "資料 (.dtz)"},
    {id = "auz", label = "音訊 (.auz)"}
}

local function setStatus(msg)
    if UI.statusText then
        UI_SetText(UI.statusText, "狀態: " .. msg)
    end
end

local function extractBaseName(path)
    if not path or path == "" then
        return "archive"
    end
    local clean = path:gsub("[/\\]+$", "")
    local base = clean:match("([^/\\]+)$")
    return base or "archive"
end

local function replaceExtension(filename, newExt)
    if not filename or filename == "" then
        return "output." .. newExt
    end
    local stem = filename:gsub("%.[%w]+$", "")
    return stem .. "." .. newExt
end

local function switchTab(tabName)
    State.activeTab = tabName
    local isPack = (tabName == "pack")

    UI_SetVisible(UI.packPage, isPack)
    UI_SetVisible(UI.unpackPage, not isPack)

    -- 鎖定目前作用中的分頁按鈕作為視覺高亮指示
    UI_SetButtonLocked(UI.tabPackBtn, isPack)
    UI_SetButtonLocked(UI.tabUnpackBtn, not isPack)

    setStatus(isPack and "請拖入來源資料夾或點擊瀏覽選擇目錄" or "請拖入 .imz / .luz / .dtz / .auz 封包檔")
end

local function selectArchiveType(typeId)
    State.selectedType = typeId
    for id, btn in pairs(UI.typeBtns) do
        UI_SetButtonLocked(btn, id == typeId)
    end

    -- 自動同步輸出檔名的副檔名
    if UI.packOutInput then
        local curOut = UI_GetText(UI.packOutInput)
        if curOut ~= "" then
            UI_SetText(UI.packOutInput, replaceExtension(curOut, typeId))
        end
    end
end

-- 接收視窗系統拖放進來的資料夾或檔案
function OnFileDrop(paths)
    if not paths or #paths == 0 then
        return
    end
    local firstPath = paths[1]

    if State.activeTab == "pack" then
        UI_SetText(UI.packSrcInput, firstPath)
        local curOut = UI_GetText(UI.packOutInput)
        if curOut == "" then
            local base = extractBaseName(firstPath)
            UI_SetText(UI.packOutInput, "./" .. base .. "." .. State.selectedType)
        end
        setStatus("已載入來源資料夾: " .. firstPath)
    else
        UI_SetText(UI.unpackFileInput, firstPath)
        local curOutDir = UI_GetText(UI.unpackOutInput)
        if curOutDir == "" then
            local base = extractBaseName(firstPath):gsub("%.[%w]+$", "")
            UI_SetText(UI.unpackOutInput, "./extracted_" .. base)
        end
        setStatus("已載入封包檔案: " .. firstPath)
    end
end

local function buildPackPage(root)
    local page = CreateUI("Panel", {x = 24, y = 76, w = 912, h = 490}, root)
    UI.packPage = page

    -- 1. 來源資料夾
    CreateUI("Text", {x = 24, y = 22, text = "來源資料夾（支援直接拖放資料夾至此視窗）：", fontSize = 18}, page)
    UI.packSrcInput = CreateUI("InputField", {x = 24, y = 52, w = 710, h = 36, fontSize = 16}, page)
    local browseSrcBtn = CreateUI("Button", {x = 748, y = 52, w = 140, h = 36, text = "瀏覽資料夾..."}, page)
    UI_OnClick(
        browseSrcBtn,
        function()
            local dir = OpenFolderDialog("選擇要加密打包的來源資料夾")
            if dir then
                OnFileDrop({dir})
            end
        end
    )

    -- 2. 封包類型選擇
    CreateUI("Text", {x = 24, y = 108, text = "封包資源類型：", fontSize = 18}, page)
    for i, t in ipairs(TYPE_DEFS) do
        local bx = 24 + (i - 1) * 165
        local btn = CreateUI("Button", {x = bx, y = 138, w = 152, h = 36, text = t.label}, page)
        UI.typeBtns[t.id] = btn
        UI_OnClick(
            btn,
            function()
                selectArchiveType(t.id)
            end
        )
    end

    -- 3. 輸出檔案名稱 / 路徑
    CreateUI("Text", {x = 24, y = 194, text = "輸出封包檔案名稱（含路徑）：", fontSize = 18}, page)
    UI.packOutInput = CreateUI("InputField", {x = 24, y = 224, w = 710, h = 36, fontSize = 16}, page)
    UI_SetText(UI.packOutInput, "./assets_pack.imz")

    local browseOutDirBtn = CreateUI("Button", {x = 748, y = 224, w = 140, h = 36, text = "選擇輸出目錄"}, page)
    UI_OnClick(
        browseOutDirBtn,
        function()
            local outDir = OpenFolderDialog("選擇輸出目錄")
            if outDir then
                local base = extractBaseName(UI_GetText(UI.packSrcInput))
                UI_SetText(UI.packOutInput, outDir .. "/" .. base .. "." .. State.selectedType)
            end
        end
    )

    -- 4. 加密密碼
    CreateUI("Text", {x = 24, y = 280, text = "ChaCha20 加密密碼（支援任意密碼短語或 64 位 Hex 金鑰，留空則不加密）：", fontSize = 18}, page)
    UI.packKeyInput = CreateUI("InputField", {x = 24, y = 310, w = 864, h = 36, fontSize = 16}, page)

    -- 5. 執行打包按鈕
    local execPackBtn = CreateUI("Button", {x = 24, y = 385, w = 240, h = 48, text = "開始壓縮並加密打包"}, page)
    UI_OnClick(
        execPackBtn,
        function()
            local srcDir = UI_GetText(UI.packSrcInput)
            local outFile = UI_GetText(UI.packOutInput)
            local key = UI_GetText(UI.packKeyInput)
            local doEncrypt = (key ~= "")

            if srcDir == "" then
                setStatus("錯誤：請先指定來源資料夾！")
                return
            end
            if outFile == "" then
                setStatus("錯誤：請輸入輸出檔案名稱！")
                return
            end

            outFile = replaceExtension(outFile, State.selectedType)
            UI_SetText(UI.packOutInput, outFile)

            local ok, msg = GpkPackDirectory(srcDir, outFile, State.selectedType, key, doEncrypt)
            setStatus(msg or (ok and "打包完成" or "打包失敗"))
        end
    )
end

local function buildUnpackPage(root)
    local page = CreateUI("Panel", {x = 24, y = 76, w = 912, h = 490}, root)
    UI.unpackPage = page
    UI_SetVisible(page, false)

    -- 1. 來源封包檔案
    CreateUI("Text", {x = 24, y = 22, text = "目標封包檔案（支援直接拖放 .imz / .luz / .dtz / .auz 至此視窗）：", fontSize = 18}, page)
    UI.unpackFileInput = CreateUI("InputField", {x = 24, y = 52, w = 710, h = 36, fontSize = 16}, page)
    local browseFileBtn = CreateUI("Button", {x = 748, y = 52, w = 140, h = 36, text = "選擇封包..."}, page)
    UI_OnClick(
        browseFileBtn,
        function()
            local file = OpenFileDialog("選擇要解密解壓縮的封包檔案")
            if file then
                OnFileDrop({file})
            end
        end
    )

    -- 2. 解壓縮輸出目錄
    CreateUI("Text", {x = 24, y = 104, text = "解壓縮輸出資料夾：", fontSize = 18}, page)
    UI.unpackOutInput = CreateUI("InputField", {x = 24, y = 134, w = 710, h = 36, fontSize = 16}, page)
    UI_SetText(UI.unpackOutInput, "./extracted_output")
    local browseExtractDirBtn = CreateUI("Button", {x = 748, y = 134, w = 140, h = 36, text = "瀏覽目錄..."}, page)
    UI_OnClick(
        browseExtractDirBtn,
        function()
            local dir = OpenFolderDialog("選擇解壓縮輸出資料夾")
            if dir then
                UI_SetText(UI.unpackOutInput, dir)
            end
        end
    )

    -- 3. 解密密碼
    CreateUI("Text", {x = 24, y = 186, text = "解密密碼（若封包未加密則留空）：", fontSize = 18}, page)
    UI.unpackKeyInput = CreateUI("InputField", {x = 24, y = 216, w = 864, h = 36, fontSize = 16}, page)

    -- 4. 預覽與解包操作按鈕
    local listBtn = CreateUI("Button", {x = 24, y = 272, w = 200, h = 42, text = "驗證密碼並檢視清單"}, page)
    UI_OnClick(
        listBtn,
        function()
            local arcPath = UI_GetText(UI.unpackFileInput)
            local key = UI_GetText(UI.unpackKeyInput)
            if arcPath == "" then
                setStatus("錯誤：請先選擇封包檔案！")
                return
            end

            local files, arcType = GpkList(arcPath, key)
            if not files then
                UI_SetText(UI.previewText, "讀取失敗：" .. tostring(arcType))
                setStatus("密碼錯誤或檔案已損毀")
                return
            end

            local lines = {string.format("封包類型: .%s | 共 %d 個檔案：", tostring(arcType), #files)}
            local maxShow = math.min(#files, 8)
            for i = 1, maxShow do
                table.insert(lines, "  - " .. files[i])
            end
            if #files > maxShow then
                table.insert(lines, string.format("  ... 以及其餘 %d 個檔案", #files - maxShow))
            end

            UI_SetText(UI.previewText, table.concat(lines, "\n"))
            setStatus("驗證成功，共讀取 " .. #files .. " 筆資源")
        end
    )

    local extractBtn = CreateUI("Button", {x = 240, y = 272, w = 220, h = 42, text = "解密並全部解壓縮"}, page)
    UI_OnClick(
        extractBtn,
        function()
            local arcPath = UI_GetText(UI.unpackFileInput)
            local outDir = UI_GetText(UI.unpackOutInput)
            local key = UI_GetText(UI.unpackKeyInput)

            if arcPath == "" then
                setStatus("錯誤：請先選擇封包檔案！")
                return
            end
            if outDir == "" then
                setStatus("錯誤：請指定輸出資料夾！")
                return
            end

            local ok, msg = GpkExtractAll(arcPath, outDir, key)
            setStatus(msg or (ok and "解壓縮完成" or "解壓縮失敗"))
        end
    )

    -- 5. 封包內容預覽框
    local previewBox = CreateUI("Panel", {x = 24, y = 328, w = 864, h = 142}, page)
    UI.previewText =
        CreateUI("Text", {x = 12, y = 10, text = "尚未載入封包內容（點擊「驗證密碼並檢視清單」可預覽內部檔案）", fontSize = 15}, previewBox)
end

function OnSceneEnter()
    -- 頂部主分頁切換列
    UI.tabPackBtn = CreateUI("Button", {x = 24, y = 24, w = 180, h = 40, text = "加密打包 (Pack)"})
    UI.tabUnpackBtn = CreateUI("Button", {x = 216, y = 24, w = 180, h = 40, text = "解密解包 (Unpack)"})

    UI_OnClick(
        UI.tabPackBtn,
        function()
            switchTab("pack")
        end
    )
    UI_OnClick(
        UI.tabUnpackBtn,
        function()
            switchTab("unpack")
        end
    )

    -- 建立兩個分頁
    buildPackPage(nil)
    buildUnpackPage(nil)

    -- 底部狀態列
    local statusBar = CreateUI("Panel", {x = 24, y = 580, w = 912, h = 40})
    UI.statusText = CreateUI("Text", {x = 14, y = 10, text = "狀態: 就緒", fontSize = 16}, statusBar)

    -- 初始化預設狀態
    selectArchiveType("imz")
    switchTab("pack")
end

function OnSceneExit()
end
