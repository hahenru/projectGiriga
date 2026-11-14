-- assets/scripts/packer_tool.lua
-- Project Girigara 資源封包加解密工具 (GUI 雙分頁版)

local State = {
    activeTab = "pack",      -- "pack" | "unpack"
    selectedType = "imz",    -- "imz" | "luz" | "dtz" | "auz"
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
    statusText = nil,
}

local TYPE_DEFS = {
    { id = "imz", label = "圖片 (.imz)" },
    { id = "luz", label = "腳本 (.luz)" },
    { id = "dtz", label = "資料 (.dtz)" },
    { id = "auz", label = "音訊 (.auz)" },
}

-- 輔助：建立帶指定字體大小的按鈕
local function createStyledButton(props, fontSize, parent)
local btn = CreateUI("Button", props, parent)
if btn and fontSize then
    UI_SetFontSize(btn, fontSize)
    end
    return btn
    end

    local function setStatus(msg)
    if UI.statusText then
        UI_SetText(UI.statusText, "狀態: " .. msg)
        end
        end

        local function extractBaseName(path)
        if not path or path == "" then return "archive" end
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

                UI_SetButtonLocked(UI.tabPackBtn, isPack)
                UI_SetButtonLocked(UI.tabUnpackBtn, not isPack)

                setStatus(isPack and "請拖入來源資料夾或點擊瀏覽選擇目錄" or "請拖入 .imz / .luz / .dtz / .auz 封包檔")
                end

                local function selectArchiveType(typeId)
                State.selectedType = typeId
                for id, btn in pairs(UI.typeBtns) do
                    UI_SetButtonLocked(btn, id == typeId)
                    end

                    if UI.packOutInput then
                        local curOut = UI_GetText(UI.packOutInput)
                        if curOut ~= "" then
                            UI_SetText(UI.packOutInput, replaceExtension(curOut, typeId))
                            end
                            end
                            end

                            -- 1. 替換原本的 OnFileDrop(paths)：
                            function OnFileDrop(paths)
                            if not paths or #paths == 0 then return end
                                local firstPath = paths[1]
                                local kind = FsStatPath(firstPath)

                                if kind == "directory" then
                                    if State.activeTab ~= "pack" then
                                        switchTab("pack")
                                            end
                                            UI_SetText(UI.packSrcInput, firstPath)
                                            local curOut = UI_GetText(UI.packOutInput)
                                            if curOut == "" or curOut == "./assets_pack." .. State.selectedType then
                                                local base = extractBaseName(firstPath)
                                                UI_SetText(UI.packOutInput, "./" .. base .. "." .. State.selectedType)
                                                end
                                                setStatus("已載入來源資料夾: " .. firstPath)
                                                elseif kind == "file" then
                                                    if State.activeTab ~= "unpack" then
                                                        switchTab("unpack")
                                                            end
                                                            UI_SetText(UI.unpackFileInput, firstPath)
                                                            local curOutDir = UI_GetText(UI.unpackOutInput)
                                                            if curOutDir == "" or curOutDir == "./extracted_output" then
                                                                local base = extractBaseName(firstPath):gsub("%.[%w]+$", "")
                                                                UI_SetText(UI.unpackOutInput, "./extracted_" .. base)
                                                                end
                                                                setStatus("已載入封包檔案: " .. firstPath)
                                                                else
                                                                    setStatus("錯誤：無法識別拖入的路徑: " .. tostring(firstPath))
                                                                    end
                                                                    end

                                                local function buildPackPage(root, pageW, pageH)
                                                local page = CreateUI("Panel", { x = 32, y = 96, w = pageW, h = pageH }, root)
                                                UI.packPage = page

                                                local pad = 32
                                                local browseW = 190
                                                local gap = 16
                                                local inputW = pageW - pad * 2 - browseW - gap

                                                -- 1. 來源資料夾
                                                CreateUI("Text", { x = pad, y = 28, text = "來源資料夾（支援直接拖放資料夾至此視窗）：", fontSize = 22 }, page)
                                                UI.packSrcInput = CreateUI("InputField", { x = pad, y = 64, w = inputW, h = 44, fontSize = 20 }, page)
                                                local browseSrcBtn = createStyledButton({ x = pad + inputW + gap, y = 64, w = browseW, h = 44, text = "瀏覽資料夾..." }, 20, page)
                                                UI_OnClick(browseSrcBtn, function()
                                                local dir = OpenFolderDialog("選擇要加密打包的來源資料夾")
                                                if dir then
                                                    OnFileDrop({ dir })
                                                    end
                                                    end)

                                                -- 2. 封包類型選擇
                                                CreateUI("Text", { x = pad, y = 132, text = "封包資源類型：", fontSize = 22 }, page)
                                                local typeBtnW = 180
                                                for i, t in ipairs(TYPE_DEFS) do
                                                    local bx = pad + (i - 1) * (typeBtnW + 18)
                                                    local btn = createStyledButton({ x = bx, y = 168, w = typeBtnW, h = 44, text = t.label }, 20, page)
                                                    UI.typeBtns[t.id] = btn
                                                    UI_OnClick(btn, function()
                                                    selectArchiveType(t.id)
                                                    end)
                                                    end

                                                    -- 3. 輸出檔案名稱 / 路徑
                                                    CreateUI("Text", { x = pad, y = 236, text = "輸出封包檔案名稱（含路徑）：", fontSize = 22 }, page)
                                                    UI.packOutInput = CreateUI("InputField", { x = pad, y = 272, w = inputW, h = 44, fontSize = 20 }, page)
                                                    UI_SetText(UI.packOutInput, "./assets_pack.imz")

                                                    local browseOutDirBtn = createStyledButton({ x = pad + inputW + gap, y = 272, w = browseW, h = 44, text = "選擇輸出目錄" }, 20, page)
                                                    UI_OnClick(browseOutDirBtn, function()
                                                    local outDir = OpenFolderDialog("選擇輸出目錄")
                                                    if outDir then
                                                        local base = extractBaseName(UI_GetText(UI.packSrcInput))
                                                        UI_SetText(UI.packOutInput, outDir .. "/" .. base .. "." .. State.selectedType)
                                                        end
                                                        end)

                                                    -- 4. 加密密碼
                                                    CreateUI("Text", { x = pad, y = 340, text = "ChaCha20 加密密碼（支援任意密碼短語或 64 位 Hex 金鑰，留空則不加密）：", fontSize = 22 }, page)
                                                    UI.packKeyInput = CreateUI("InputField", { x = pad, y = 376, w = pageW - pad * 2, h = 44, fontSize = 20 }, page)

                                                    -- 5. 執行打包按鈕
                                                    local execPackBtn = createStyledButton({ x = pad, y = 456, w = 280, h = 54, text = "開始壓縮並加密打包" }, 22, page)
                                                    UI_OnClick(execPackBtn, function()
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
                                                            end)
                                                    end

                                                    local function buildUnpackPage(root, pageW, pageH)
                                                    local page = CreateUI("Panel", { x = 32, y = 96, w = pageW, h = pageH }, root)
                                                    UI.unpackPage = page
                                                    UI_SetVisible(page, false)

                                                    local pad = 32
                                                    local browseW = 190
                                                    local gap = 16
                                                    local inputW = pageW - pad * 2 - browseW - gap

                                                    -- 1. 來源封包檔案
                                                    CreateUI("Text", { x = pad, y = 28, text = "目標封包檔案（支援直接拖放 .imz / .luz / .dtz / .auz 至此視窗）：", fontSize = 22 }, page)
                                                    UI.unpackFileInput = CreateUI("InputField", { x = pad, y = 64, w = inputW, h = 44, fontSize = 20 }, page)
                                                    local browseFileBtn = createStyledButton({ x = pad + inputW + gap, y = 64, w = browseW, h = 44, text = "選擇封包..." }, 20, page)
                                                    UI_OnClick(browseFileBtn, function()
                                                    local file = OpenFileDialog("選擇要解密解壓縮的封包檔案")
                                                    if file then
                                                        OnFileDrop({ file })
                                                        end
                                                        end)

                                                    -- 2. 解壓縮輸出目錄
                                                    CreateUI("Text", { x = pad, y = 132, text = "解壓縮輸出資料夾：", fontSize = 22 }, page)
                                                    UI.unpackOutInput = CreateUI("InputField", { x = pad, y = 168, w = inputW, h = 44, fontSize = 20 }, page)
                                                    UI_SetText(UI.unpackOutInput, "./extracted_output")
                                                    local browseExtractDirBtn = createStyledButton({ x = pad + inputW + gap, y = 168, w = browseW, h = 44, text = "瀏覽目錄..." }, 20, page)
                                                    UI_OnClick(browseExtractDirBtn, function()
                                                    local dir = OpenFolderDialog("選擇解壓縮輸出資料夾")
                                                    if dir then
                                                        UI_SetText(UI.unpackOutInput, dir)
                                                        end
                                                        end)

                                                    -- 3. 解密密碼
                                                    CreateUI("Text", { x = pad, y = 236, text = "解密密碼（若封包未加密則留空）：", fontSize = 22 }, page)
                                                    UI.unpackKeyInput = CreateUI("InputField", { x = pad, y = 272, w = pageW - pad * 2, h = 44, fontSize = 20 }, page)

                                                    -- 4. 預覽與解包操作按鈕
                                                    local listBtn = createStyledButton({ x = pad, y = 340, w = 260, h = 48, text = "驗證密碼並檢視清單" }, 20, page)
                                                    UI_OnClick(listBtn, function()
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

                                                            local previewItems = {}
                                                            local maxShow = math.min(#files, 6)
                                                            for i = 1, maxShow do
                                                                table.insert(previewItems, files[i])
                                                                end
                                                                local suffix = (#files > maxShow) and string.format(" ... (共 %d 個檔案)", #files) or string.format(" (共 %d 個檔案)", #files)
                                                                UI_SetText(UI.previewText, string.format("[.%s] %s%s", tostring(arcType), table.concat(previewItems, " , "), suffix))
                                                                setStatus("驗證成功，共讀取 " .. #files .. " 筆資源")
                                                                end)

                                                    local extractBtn = createStyledButton({ x = pad + 280, y = 340, w = 260, h = 48, text = "解密並全部解壓縮" }, 20, page)
                                                    UI_OnClick(extractBtn, function()
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
                                                            end)

                                                    -- 5. 封包內容預覽框
                                                    local previewBoxH = math.max(80, pageH - 416 - 24)
                                                    local previewBox = CreateUI("Panel", { x = pad, y = 412, w = pageW - pad * 2, h = previewBoxH }, page)
                                                    UI.previewText = CreateUI("Text", { x = 16, y = 18, text = "尚未載入封包內容（點擊「驗證密碼並檢視清單」可預覽內部檔案）", fontSize = 19 }, previewBox)
                                                    end

                                                    function OnSceneEnter()
                                                    local screenW, screenH = UI_GetScreenSize()
                                                    local contentW = math.max(900, screenW - 64)
                                                    local pageH = math.max(540, screenH - 176)

                                                    -- 頂部主分頁切換列
                                                    UI.tabPackBtn = createStyledButton({ x = 32, y = 32, w = 230, h = 48, text = "加密打包 (Pack)" }, 21)
                                                    UI.tabUnpackBtn = createStyledButton({ x = 278, y = 32, w = 230, h = 48, text = "解密解包 (Unpack)" }, 21)

                                                    UI_OnClick(UI.tabPackBtn, function() switchTab("pack") end)
                                                    UI_OnClick(UI.tabUnpackBtn, function() switchTab("unpack") end)

                                                    -- 建立兩個分頁
                                                    buildPackPage(nil, contentW, pageH)
                                                    buildUnpackPage(nil, contentW, pageH)

                                                    -- 底部狀態列
                                                    local statusY = 96 + pageH + 16
                                                    local statusBar = CreateUI("Panel", { x = 32, y = statusY, w = contentW, h = 48 })
                                                    UI.statusText = CreateUI("Text", { x = 18, y = 13, text = "狀態: 就緒", fontSize = 20 }, statusBar)

                                                    -- 初始化預設狀態
                                                    selectArchiveType("imz")
                                                    switchTab("pack")
                                                        RegisterFileDropCallback(OnFileDrop)
                                                        end

                                                        -- 3. 在 OnSceneExit() 中解除註冊：
                                                        function OnSceneExit()
                                                        RegisterFileDropCallback(nil)
                                                        end
