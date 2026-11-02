-- Mitori-Heft: setzt das Prüfungsprogramm aus build/daten/*.lua.
--
-- Die Daten erzeugt tools/daten.py aus daten/*.yaml (geprüft, Kanji schon
-- eingesetzt). Hier nur der Satz, in zwei Ansichten:
--   tusche     ein Grad je Seite, zweispaltig (Entwurf 1)
--   landkarte  Technik × Angriff je Gruppe, Querformat (Entwurf 5)
-- Shinki Toho hat eigene Seiten (shinkitoho-…), weil es keine Angriffe gibt.
-- Aufruf aus LaTeX: \ansicht{tusche-aikido-1} → mitori.ansicht("tusche-aikido-1").
-- Die Makros stehen in stil/shinki.sty.
local M = {}

M.daten = "../build/daten/"

local cache = {}

-------------------------------------------------------------------------------
-- Hilfen

local function out(s) tex.sprint(s) end

local function esc(s)
  s = s:gsub("\\", "\\textbackslash{}")
  s = s:gsub("([&%%#_{}$])", "\\%1")
  s = s:gsub("~", "\\textasciitilde{}"):gsub("%^", "\\textasciicircum{}")
  return s
end

local function laden(art)
  if not cache[art] then cache[art] = dofile(M.daten .. art .. ".lua") end
  return cache[art]
end

-- Kanji setzen: Daitoryu als Vorschlag (heller), sonst gesichert
local function jp(d, k)
  if not k or k == "" then return "" end
  return (d.vorschlag and "\\jpv{" or "\\jp{") .. k .. "}"
end

local function frage(id)
  if not id then return "" end
  return "\\frage{" .. id .. "}"
end

local function fragen(liste)
  local t = {}
  for _, id in ipairs(liste or {}) do t[#t + 1] = frage(id) end
  return table.concat(t)
end

local function grad(d, id)
  for _, g in ipairs(d.grade) do if g.id == id then return g end end
end

local function rubrik(teil) return esc((teil.titel:gsub("%-Grade", "-Programm"))) end

local function opt(s) return s and esc(s) or "" end

-- Angriff als \angriff{Name}{Zusatz}{Frage}{Kanji}; ・ darf umbrechen
local function angriff_tex(d, a)
  local k = a.jp and a.jp:gsub("・", "・\\allowbreak{}")
  return "\\angriff{" .. opt(a.angriff) .. "}{" .. opt(a.zusatz) .. "}{" .. frage(a.frage) .. "}{" .. jp(d, k) .. "}"
end

local function ohne_angriff(gr, a) return gr.ohne_angriff or gr.text or not a.angriff end

-------------------------------------------------------------------------------
-- Ansicht 1: Tusche, ein Grad je Seite

function M.tusche(art, nr)
  local d = laden(art)
  local teil = d.teile[nr]
  for _, gid in ipairs(teil.grade) do
    local g = grad(d, gid)
    out("\\tuscheseite{" .. esc(d.titel) .. "}{" .. opt(d.untertitel) .. "}{" .. esc(g.name) .. "}{" .. g.jp
        .. "}{Prüfungsprogramm " .. esc(teil.titel) .. "}{" .. esc(d.stand) .. "}{" .. rubrik(teil) .. "}")
    out("\\begin{tuschespalten}")
    for _, gr in ipairs(teil.gruppen) do
      local zeilen = {}
      for _, a in ipairs(gr.angriffe) do if a.techniken[gid] then zeilen[#zeilen + 1] = a end end
      if #zeilen > 0 then
        out("\\tgruppe{" .. esc(gr.titel) .. frage(gr.frage) .. "}{" .. jp(d, gr.jp) .. "}")
        for _, a in ipairs(zeilen) do
          local t = {}
          for _, e in ipairs(a.techniken[gid]) do
            t[#t + 1] = "\\ttech{" .. esc(e.name) .. fragen(e.fragen) .. "}{" .. opt(e.zusatz) .. "}{" .. jp(d, e.jp) .. "}"
          end
          if ohne_angriff(gr, a) then
            out("\\tzeilevoll{" .. frage(a.frage) .. table.concat(t) .. "}")
          else
            out("\\tzeile{" .. angriff_tex(d, a) .. "}{" .. table.concat(t) .. "}")
          end
        end
        if gr.fuss then out("\\tfuss{" .. esc(gr.fuss) .. "}") end
      end
    end
    out("\\end{tuschespalten}")
    out(d.vorschlag and "\\tuscheende[1]" or "\\tuscheende")
  end
end

-------------------------------------------------------------------------------
-- Ansicht 5: Technik-Landkarte je Gruppe

-- Farbstufe 1..5 für die Position eines Grades (früh = hell)
local function stufe(pos, n)
  if n <= 1 then return 5 end
  return math.floor((pos - 1) * 4 / (n - 1) + 0.5) + 1
end

local function grad_kurz(g)
  if g.id:match("^[kd]%d") then return g.id:sub(2) end
  return "O"
end

local function landkarte_gruppe(d, teil, gr, minVorkommen)
  local spalten = {}
  for _, a in ipairs(gr.angriffe) do
    if a.angriff and next(a.techniken) then spalten[#spalten + 1] = a end
  end
  if gr.ohne_angriff or gr.text or #spalten < 3 then return end

  local pos = {}
  for i, gid in ipairs(teil.grade) do pos[gid] = i end
  local karte, folge = {}, {}
  for ci, a in ipairs(spalten) do
    for _, gid in ipairs(teil.grade) do
      for _, e in ipairs(a.techniken[gid] or {}) do
        if not karte[e.name] then
          karte[e.name] = { zellen = {}, erst = 99, zahl = 0, jp = e.jp }
          folge[#folge + 1] = e.name
        end
        local z = karte[e.name]
        if not z.zellen[ci] then z.zellen[ci] = {}; z.zahl = z.zahl + 1 end
        local zelle = z.zellen[ci]
        if zelle[#zelle] ~= gid then zelle[#zelle + 1] = gid end
        z.erst = math.min(z.erst, pos[gid])
      end
    end
  end
  local zeilen = {}
  for i, name in ipairs(folge) do
    if karte[name].zahl >= minVorkommen then zeilen[#zeilen + 1] = { name = name, i = i } end
  end
  table.sort(zeilen, function(x, y)
    local a, b = karte[x.name], karte[y.name]
    if a.erst ~= b.erst then return a.erst < b.erst end
    if a.zahl ~= b.zahl then return a.zahl > b.zahl end
    return x.i < y.i
  end)
  if #zeilen < 3 then return end -- zu wenig für eine Karte

  local n = #teil.grade
  out("\\landkarteanfang{" .. esc(d.titel) .. "}{" .. esc(gr.titel) .. " · " .. esc(teil.titel) .. "}{"
      .. jp(d, gr.jp) .. "}{" .. #spalten .. "}{" .. rubrik(teil) .. "}{" .. #zeilen .. "}")
  out("\\begin{longtable}{@{}T*{" .. #spalten .. "}{Z}S@{}}")
  local kopf = { "" }
  for _, a in ipairs(spalten) do
    kopf[#kopf + 1] = "\\lkopf{" .. esc(a.angriff) .. (a.zusatz and (" (" .. esc(a.zusatz) .. ")") or "") .. "}"
  end
  kopf[#kopf + 1] = "\\lkopf{Summe}"
  out(table.concat(kopf, "&") .. "\\lkopfende\\endhead")
  for _, z in ipairs(zeilen) do
    local k = karte[z.name]
    local zeile = { "\\ltech{" .. esc(z.name) .. "}{" .. jp(d, k.jp) .. "}" }
    for ci = 1, #spalten do
      local zelle = k.zellen[ci]
      if zelle then
        local kurz, erst = {}, 99
        for _, gid in ipairs(zelle) do
          kurz[#kurz + 1] = grad_kurz(grad(d, gid)); erst = math.min(erst, pos[gid])
        end
        zeile[#zeile + 1] = "\\lzelle{" .. stufe(erst, n) .. "}{" .. table.concat(kurz, "·") .. "}"
      else
        zeile[#zeile + 1] = ""
      end
    end
    zeile[#zeile + 1] = "\\lsumme{" .. k.zahl .. "}"
    out(table.concat(zeile, "&") .. "\\lzeilenende")
  end
  out("\\end{longtable}")
  local leg = {}
  for i, gid in ipairs(teil.grade) do
    leg[#leg + 1] = "\\lschluessel{" .. stufe(i, n) .. "}{" .. grad_kurz(grad(d, gid)) .. "}{" .. esc(grad(d, gid).name) .. "}"
  end
  out("\\landkarteende{" .. esc(teil.titel:match("^(%a+)")) .. "}{" .. table.concat(leg) .. "}{"
      .. (minVorkommen > 1 and ("Nur Techniken, die bei mindestens " .. minVorkommen .. " Angriffen vorkommen.") or "") .. "}")
end

function M.landkarte(art, nr, minVorkommen)
  local d = laden(art)
  local teil = d.teile[nr]
  out("\\clearpage")
  for _, gr in ipairs(teil.gruppen) do landkarte_gruppe(d, teil, gr, minVorkommen or 1) end
end

-------------------------------------------------------------------------------
-- Shinki Toho

local STUFEN = { { 1, 11 }, { 12, 21 }, { 22, 32 }, { 33, 43 } } -- Shoden … Jun-Menkyo

local function stufe_von(nr)
  for i, s in ipairs(STUFEN) do if nr >= s[1] and nr <= s[2] then return i end end
end

local function form_kanji(f) return f.kanji and ("\\jp{" .. f.kanji .. "}") or "\\keinkanji" end

local function form_fragen(f)
  local t = {}
  for id in (f.frage or ""):gmatch("%S+") do t[#t + 1] = id end
  return fragen(t)
end

-- Grade, Zulassung, Kamae, Suburi
function M.shinkitoho_grade()
  local d = laden("shinki-toho")
  out("\\tuscheseite{" .. esc(d.titel) .. "}{}{Grade \\& Programm}{位階}{Prüfungsprogramm Shinki Toho}{"
      .. esc(d.stand) .. "}{Shinki Toho}")
  out("\\begin{stleiter}")
  for _, g in ipairs(d.grade) do
    out("\\stsprosse{" .. esc(g.name) .. "}{" .. g.jp .. "}{" .. esc(g.bedeutung) .. "}{"
        .. opt(g.inhalt) .. frage(g.frage) .. "}{" .. opt(g.abstand) .. "}")
  end
  out("\\end{stleiter}")
  local z = {}
  for _, s in ipairs(d.zulassung) do z[#z + 1] = "\\item " .. esc(s) end
  out("\\stzulassung{" .. table.concat(z) .. "}")
  out("\\begin{tuschespalten}")
  out("\\tgruppe{Kamae-Übung}{\\jp{構}}")
  local k = {}
  for i, x in ipairs(d.kamae) do k[#k + 1] = "\\stkamae{" .. i .. "}{" .. esc(x.name) .. "}{" .. x.kanji .. "}" end
  out("\\stkamaeliste{" .. table.concat(k) .. "}")
  out("\\tfuss{Ein Ablauf; Seigan kommt darum zweimal vor.}")
  out("\\tgruppe{Suburi}{\\jp{素振り}}")
  for _, s in ipairs(d.suburi) do out("\\stuebung{" .. esc(s.name) .. "}{\\jp{" .. s.kanji .. "}}{" .. esc(s.text) .. "}") end
  local ki = d.kiriotoshi
  out("\\tgruppe{" .. esc(ki.name) .. "}{\\jp{" .. ki.kanji .. "}}")
  local text = esc(ki.text)
  for _, c in ipairs({ "真", "行", "草" }) do text = text:gsub(c, "\\jp{" .. c .. "}") end
  out("\\stuebung{}{}{" .. text .. "}")
  out("\\end{tuschespalten}")
  out("\\tuscheende[2]")
end

-- Ô-dachi je Stufe, Tusche-Seiten
function M.shinkitoho_odachi()
  local d = laden("shinki-toho")
  for i, s in ipairs(STUFEN) do
    local g = d.grade[i]
    out("\\tuscheseite{" .. esc(d.titel) .. "}{}{" .. esc(g.name) .. "}{" .. g.jp .. "}{Ô-dachi tai Ô-dachi · Formen "
        .. s[1] .. "–" .. s[2] .. "}{" .. esc(g.inhalt) .. "}{Shinki Toho}")
    out("\\begin{stformen}")
    local ohne = false
    for _, f in ipairs(d.odachi) do
      if f.nr >= s[1] and f.nr <= s[2] then
        ohne = ohne or not f.kanji
        out("\\stform{" .. f.nr .. "}{" .. esc(f.name) .. form_fragen(f) .. "}{" .. form_kanji(f) .. "}{" .. esc(f.uchidachi)
            .. "}{" .. esc(f.shidachi) .. "}{" .. esc(f.treffer) .. "}{" .. esc(f.zanshin) .. "}")
      end
    end
    out("\\end{stformen}")
    if ohne then out("\\stfussnote{– Kanji der Form: noch keine Quelle.}") end
    out("\\tuscheende[2]")
  end
end

-- Landkarte: Kamae Uchidachi × Kamae Shidachi, Zellen = Formnummern
function M.shinkitoho_landkarte()
  local d = laden("shinki-toho")
  local zeilen, spalten, zs, ss, karte = {}, {}, {}, {}, {}
  for _, f in ipairs(d.odachi) do
    local u, s = f.uchidachi, f.shidachi
    if not zs[u] then zs[u] = true; zeilen[#zeilen + 1] = u end
    if not ss[s] then ss[s] = true; spalten[#spalten + 1] = s end
    karte[u] = karte[u] or {}
    karte[u][s] = karte[u][s] or {}
    table.insert(karte[u][s], f.nr)
  end
  local function zeilensumme(u) local n = 0; for _, v in pairs(karte[u]) do n = n + #v end; return n end
  local function spaltensumme(s) local n = 0; for _, u in ipairs(zeilen) do n = n + #(karte[u][s] or {}) end; return n end
  table.sort(zeilen, function(a, b) return zeilensumme(a) > zeilensumme(b) end)
  table.sort(spalten, function(a, b) return spaltensumme(a) > spaltensumme(b) end)

  out("\\landkarteanfang{" .. esc(d.titel) .. "}{Ô-dachi: Kamae von Uchidachi und Shidachi}{\\jp{構}}{"
      .. #spalten .. "}{Shinki Toho}{" .. #zeilen .. "}")
  out("\\begin{longtable}{@{}T*{" .. #spalten .. "}{Z}S@{}}")
  local kopf = { "\\lecke{Uchidachi $\\downarrow$\\quad Shidachi $\\rightarrow$}" }
  for _, s in ipairs(spalten) do kopf[#kopf + 1] = "\\lkopf{" .. esc(s) .. "}" end
  kopf[#kopf + 1] = "\\lkopf{Summe}"
  out(table.concat(kopf, "&") .. "\\lkopfende\\endhead")
  for _, u in ipairs(zeilen) do
    local z = { "\\ltech{" .. esc(u) .. "}{}" }
    for _, s in ipairs(spalten) do
      local v = karte[u][s]
      if v then
        local nums = {}
        for _, x in ipairs(v) do nums[#nums + 1] = tostring(x) end
        z[#z + 1] = "\\lzelle{" .. stufe(stufe_von(v[1]), #STUFEN) .. "}{" .. table.concat(nums, "·\\allowbreak{}") .. "}"
      else
        z[#z + 1] = ""
      end
    end
    z[#z + 1] = "\\lsumme{" .. zeilensumme(u) .. "}"
    out(table.concat(z, "&") .. "\\lzeilenende")
  end
  out("\\end{longtable}")
  local leg = {}
  for i, s in ipairs(STUFEN) do
    leg[#leg + 1] = "\\lschluessel{" .. stufe(i, #STUFEN) .. "}{" .. s[1] .. "}{" .. esc(d.grade[i].name) .. " (" .. s[1] .. "–" .. s[2] .. ")}"
  end
  out("\\landkarteende{Stufe}{" .. table.concat(leg)
      .. "}{Die Zahlen sind die Formnummern; die Farbe zeigt die Stufe der ersten Form in der Zelle. Summe = Zahl der Formen mit dieser Kamae von Uchidachi.}")
end

-------------------------------------------------------------------------------
-- Einstieg aus LaTeX: \ansicht{<ansicht>-<disziplin>[-<teil>]}

local ANSICHTEN = {
  ["tusche-aikido-1"]       = { M.tusche, "aikido", 1 },
  ["tusche-aikido-2"]       = { M.tusche, "aikido", 2 },
  ["landkarte-aikido-1"]    = { M.landkarte, "aikido", 1, 1 },
  ["landkarte-aikido-2"]    = { M.landkarte, "aikido", 2, 1 },
  ["tusche-daitoryu-1"]     = { M.tusche, "daitoryu", 1 },
  ["tusche-daitoryu-2"]     = { M.tusche, "daitoryu", 2 },
  ["landkarte-daitoryu-1"]  = { M.landkarte, "daitoryu", 1, 2 },
  ["landkarte-daitoryu-2"]  = { M.landkarte, "daitoryu", 2, 2 },
  ["shinkitoho-grade"]      = { M.shinkitoho_grade },
  ["shinkitoho-odachi"]     = { M.shinkitoho_odachi },
  ["shinkitoho-landkarte"]  = { M.shinkitoho_landkarte },
}

function M.ansicht(name)
  local a = ANSICHTEN[name] or error("unbekannte Ansicht " .. name)
  a[1](table.unpack(a, 2))
end

return M
