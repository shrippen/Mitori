-- Mitori-Heft: setzt das Prüfungsprogramm aus build/daten/*.lua.
--
-- Die Daten erzeugt tools/daten.py aus daten/*.yaml (geprüft, Kanji schon
-- eingesetzt). Hier nur der Satz: eine Übersicht und zwei Ansichten.
--   grade      Grade mit Gruppen und Wartezeit, eine Seite je Disziplin
--   tusche     ein Grad je Seite, zweispaltig (Entwurf 1)
--   matrix     Angriff × Grad, Querformat (Entwurf 2)
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
-- Übersicht: Grade mit Gruppen und Wartezeit, wie Shinki Toho

-- Inhalt eines Grades: "Kata, Ukemi, Tachi-waza · Mae-seme (12), Theorie"
local function grad_inhalt(teil, gid)
  local t = {}
  for _, gr in ipairs(teil.gruppen) do
    local n = 0
    for _, a in ipairs(gr.angriffe) do n = n + #(a.techniken[gid] or {}) end
    if n > 0 then
      local zahl = (gr.ohne_angriff or gr.text) and "" or (" (" .. n .. ")")
      t[#t + 1] = esc(gr.titel) .. zahl
    end
  end
  return table.concat(t, ", ")
end

function M.grade(art)
  local d = laden(art)
  out("\\tuscheseite{" .. esc(d.titel) .. "}{" .. opt(d.untertitel) .. "}{Grade \\& Programm}{位階}"
      .. "{Prüfungsprogramm Kyu- und Dan-Grade}{" .. esc(d.stand) .. "}{" .. esc(d.titel) .. "}")
  out("\\begin{stleiter}")
  for _, teil in ipairs(d.teile) do
    for _, gid in ipairs(teil.grade) do
      local g = grad(d, gid)
      out("\\stsprosse{" .. esc(g.name) .. "}{" .. g.jp .. "}{}{" .. grad_inhalt(teil, gid) .. "}{" .. opt(g.abstand) .. "}")
    end
  end
  out("\\end{stleiter}")
  out("\\tfuss{In Klammern: Zahl der Techniken der Gruppe im Grad. Rechts: Mindestwartezeit nach dem vorherigen Grad.}")
  out(d.vorschlag and "\\tuscheende[1]" or "\\tuscheende")
end

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
-- Ansicht 2: Matrix Angriff × Grad

function M.matrix(art, nr)
  local d = laden(art)
  local teil = d.teile[nr]
  local n = #teil.grade
  out("\\matrixanfang{" .. esc(d.titel) .. "}{Programm für " .. esc(teil.titel) .. "}{" .. (d.jp or "")
      .. "}{" .. n .. "}{" .. rubrik(teil) .. "}")
  out("\\begin{longtable}{@{}M{\\mangriff}*{" .. n .. "}{M{\\mspalte}}@{}}")
  local kopf = { "\\mkopf{Angriff}{}" }
  for _, gid in ipairs(teil.grade) do
    local g = grad(d, gid)
    kopf[#kopf + 1] = "\\mkopf{" .. esc(g.kurzname or g.name) .. "}{" .. g.jp .. "}"
  end
  out(table.concat(kopf, "&") .. "\\mkopfende\\endhead")
  for _, gr in ipairs(teil.gruppen) do
    out("\\mgruppe{" .. (n + 1) .. "}{" .. esc(gr.titel) .. frage(gr.frage) .. "}{" .. jp(d, gr.jp) .. "}")
    for _, a in ipairs(gr.angriffe) do
      local zellen = { ohne_angriff(gr, a) and frage(a.frage) or angriff_tex(d, a) }
      for _, gid in ipairs(teil.grade) do
        local t = {}
        for _, e in ipairs(a.techniken[gid] or {}) do
          t[#t + 1] = "\\mtech{" .. esc(e.name) .. fragen(e.fragen) .. "}{" .. opt(e.zusatz) .. "}{" .. jp(d, e.jp) .. "}"
        end
        zellen[#zellen + 1] = table.concat(t)
      end
      out(table.concat(zellen, "&") .. "\\mzeilenende")
    end
    if gr.fuss then out("\\mfuss{" .. (n + 1) .. "}{" .. esc(gr.fuss) .. "}") end
  end
  out("\\end{longtable}")
  out("\\matrixende{" .. esc(d.stand) .. "}{" .. (d.vorschlag and "1" or "0") .. "}")
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

-- Ô-dachi als Gesamttabelle (Matrix-Ansicht)
function M.shinkitoho_matrix()
  local d = laden("shinki-toho")
  out("\\matrixanfang{" .. esc(d.titel) .. "}{Ô-dachi tai Ô-dachi · alle Formen}{大太刀}{0}{Shinki Toho}")
  out("\\begin{longtable}{@{}r@{\\hspace{3mm}}M{62mm}M{46mm}*{4}{M{\\stspalte}}@{}}")
  out("\\stmkopf\\endhead")
  local letzte
  for _, f in ipairs(d.odachi) do
    local st = stufe_von(f.nr)
    if st ~= letzte then
      local g = d.grade[st]
      out("\\stmstufe{" .. esc(g.name) .. "}{" .. g.jp .. "}{" .. esc(g.inhalt) .. "}")
      letzte = st
    end
    out(f.nr .. "&" .. esc(f.name) .. form_fragen(f) .. "&" .. form_kanji(f) .. "&" .. esc(f.uchidachi) .. "&"
        .. esc(f.shidachi) .. "&" .. esc(f.treffer) .. "&" .. esc(f.zanshin) .. "\\mzeilenende")
  end
  out("\\end{longtable}")
  out("\\matrixende{" .. esc(d.stand) .. "}{0}")
end

-------------------------------------------------------------------------------
-- Einstieg aus LaTeX: \ansicht{<ansicht>-<disziplin>[-<teil>]}

local ANSICHTEN = {
  ["grade-aikido"]          = { M.grade, "aikido" },
  ["tusche-aikido-1"]       = { M.tusche, "aikido", 1 },
  ["tusche-aikido-2"]       = { M.tusche, "aikido", 2 },
  ["matrix-aikido-1"]       = { M.matrix, "aikido", 1 },
  ["matrix-aikido-2"]       = { M.matrix, "aikido", 2 },
  ["grade-daitoryu"]        = { M.grade, "daitoryu" },
  ["tusche-daitoryu-1"]     = { M.tusche, "daitoryu", 1 },
  ["tusche-daitoryu-2"]     = { M.tusche, "daitoryu", 2 },
  ["matrix-daitoryu-1"]     = { M.matrix, "daitoryu", 1 },
  ["matrix-daitoryu-2"]     = { M.matrix, "daitoryu", 2 },
  ["shinkitoho-grade"]      = { M.shinkitoho_grade },
  ["shinkitoho-odachi"]     = { M.shinkitoho_odachi },
  ["shinkitoho-matrix"]     = { M.shinkitoho_matrix },
}

function M.ansicht(name)
  local a = ANSICHTEN[name] or error("unbekannte Ansicht " .. name)
  a[1](table.unpack(a, 2))
end

return M
