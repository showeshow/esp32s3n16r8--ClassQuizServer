#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "config.h"
#include "util.h"
#include "exam.h"

// ============ LittleFS 数据存储 ============
// /classes.txt                 班级列表（每行一个）
// /papers/<试卷名>.txt          试卷（首行试卷名，其后每行一题，字段以 0x1F 分隔）
// /records/R_<时间戳>.txt       考试归档（按日期和班级保存答案准确率）

inline bool storageInit() { return LittleFS.begin(true); }

// ---------------- 班级 ----------------
inline String getClasses() {
  File f = LittleFS.open("/classes.txt", "r");
  String out;
  if (!f) return out;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length()) out += line + "\n";
  }
  f.close();
  return out;
}

inline bool addClass(const String& name) {
  String n = sanitize(name);
  if (!n.length()) return false;
  String lines[64];
  int cnt = splitLines(getClasses(), lines, 64);
  for (int i = 0; i < cnt; i++)
    if (lines[i] == n) return true;  // 已存在
  File f = LittleFS.open("/classes.txt", "a");
  if (!f) return false;
  f.println(n);
  f.close();
  return true;
}

inline bool delClass(const String& name) {
  String lines[64];
  int cnt = splitLines(getClasses(), lines, 64);
  bool found = false;
  String out;
  for (int i = 0; i < cnt; i++) {
    if (lines[i] == name) { found = true; continue; }
    out += lines[i] + "\n";
  }
  if (!found) return false;
  File f = LittleFS.open("/classes.txt", "w");
  if (!f) return false;
  f.print(out);
  f.close();
  return true;
}

// ---------------- 试卷 ----------------
inline String paperPath(const String& name) { return "/papers/" + sanitize(name) + ".txt"; }

inline bool savePaper(const String& name, PaperQ* qs, int n) {
  String nm = sanitize(name);
  if (!nm.length() || n < 1 || n > MAX_QUESTIONS) return false;
  LittleFS.mkdir("/papers");
  File f = LittleFS.open(paperPath(nm), "w");
  if (!f) return false;
  f.println(nm);
  for (int i = 0; i < n; i++) {
    f.print("Q\x1F");
    f.print(fld(qs[i].text)); f.print('\x1F');
    f.print(fld(qs[i].a));    f.print('\x1F');
    f.print(fld(qs[i].b));    f.print('\x1F');
    f.print(fld(qs[i].c));    f.print('\x1F');
    f.print(fld(qs[i].d));    f.print('\x1F');
    f.println(qs[i].ans);
  }
  f.close();
  return true;
}

inline bool delPaper(const String& name) { return LittleFS.remove(paperPath(name)); }

// 解析一行题目："Q\x1F题干\x1FA\x1FB\x1FC\x1FD\x1F答案"
inline bool splitQ(const String& line, String out[6]) {
  if (line.length() < 2 || line[0] != 'Q' || line[1] != '\x1F') return false;
  int idx = 2;
  for (int i = 0; i < 6; i++) {
    int e = (i == 5) ? (int)line.length() : line.indexOf('\x1F', idx);
    if (e < 0) {
      if (i < 5) return false;
      e = line.length();
    }
    out[i] = line.substring(idx, e);
    out[i].trim();
    idx = e + 1;
  }
  return true;
}

inline bool loadPaperInto(const String& name, Exam& ex) {
  File f = LittleFS.open(paperPath(name), "r");
  if (!f) return false;
  String nm = f.readStringUntil('\n');
  nm.trim();
  ex.clearAll();
  ex.paperName = nm.length() ? nm : sanitize(name);
  int n = 0;
  String fields[6];
  while (f.available() && n < MAX_QUESTIONS) {
    String line = f.readStringUntil('\n');
    if (splitQ(line, fields)) {
      ex.qs[n].text = fields[0];
      ex.qs[n].a = fields[1];
      ex.qs[n].b = fields[2];
      ex.qs[n].c = fields[3];
      ex.qs[n].d = fields[4];
      char a = fields[5].length() ? fields[5][0] : 'A';
      if (a < 'A' || a > 'D') a = 'A';
      ex.qs[n].ans = a;
      n++;
    }
  }
  f.close();
  ex.qCount = n;
  return n > 0;
}

inline String listPapersJson() {
  String j = "[";
  bool first = true;
  File root = LittleFS.open("/papers");
  if (root) {
    File ff = root.openNextFile();
    while (ff) {
      if (!ff.isDirectory()) {
        String nm = String(ff.name());
        int slash = nm.lastIndexOf('/');
        if (slash >= 0) nm = nm.substring(slash + 1);
        if (nm.endsWith(".txt")) {
          nm = nm.substring(0, nm.length() - 4);
          if (!first) j += ",";
          first = false;
          j += "\"" + jsonEsc(nm) + "\"";
        }
      }
      ff = root.openNextFile();
    }
  }
  j += "]";
  return j;
}

// ---------------- 考试归档 ----------------
// 记录文件格式：
//   TIME=2026-08-27 10:30
//   CLASS=三年二班
//   PAPER=期中测验
//   MODE=1                       (0 一答计一 / 1 多答计一 / 2 多答计多)
//   Q=题号,正确答案,收到份数,正确份数,A人数,B人数,C人数,D人数
//   TOTAL=总份数,总正确份数
inline bool writeRecord(Exam& ex) {
  LittleFS.mkdir("/records");
  String base = "/records/R_" + fileStamp();
  String path = base + ".txt";
  int k = 2;
  while (LittleFS.exists(path)) path = base + "_" + String(k++) + ".txt";
  File f = LittleFS.open(path, "w");
  if (!f) return false;
  f.println("TIME=" + nowStr());
  f.println("CLASS=" + ex.className);
  f.println("PAPER=" + ex.paperName);
  f.println("MODE=" + String(ex.mode));
  int tot = 0, cor = 0;
  for (int i = 0; i < ex.qCount; i++) {
    QStat& st = ex.stats[i];
    f.printf("Q=%d,%c,%d,%d,%d,%d,%d,%d\n", i + 1, ex.qs[i].ans,
             st.total, st.correct, st.c[0], st.c[1], st.c[2], st.c[3]);
    tot += st.total;
    cor += st.correct;
  }
  f.printf("TOTAL=%d,%d\n", tot, cor);
  f.close();
  return true;
}

// 列出全部归档记录（元数据 JSON，供教师工作台"统计"栏筛选展示）
inline String listRecordsJson() {
  String j = "[";
  bool first = true;
  File root = LittleFS.open("/records");
  if (root) {
    File ff = root.openNextFile();
    int cnt = 0;
    while (ff && cnt < 400) {
      if (!ff.isDirectory()) {
        String nm = String(ff.name());
        int slash = nm.lastIndexOf('/');
        if (slash >= 0) nm = nm.substring(slash + 1);
        if (nm.startsWith("R_") && nm.endsWith(".txt")) {
          File rf = LittleFS.open(String("/records/") + nm, "r");
          if (rf) {
            String time = "?", cls = "", ppr = "";
            int mode = 0, qn = 0, tot = 0, cor = 0;
            while (rf.available()) {
              String line = rf.readStringUntil('\n');
              line.trim();
              if (line.startsWith("TIME=")) time = line.substring(5);
              else if (line.startsWith("CLASS=")) cls = line.substring(6);
              else if (line.startsWith("PAPER=")) ppr = line.substring(6);
              else if (line.startsWith("MODE=")) mode = line.substring(5).toInt();
              else if (line.startsWith("Q=")) qn++;
              else if (line.startsWith("TOTAL=")) {
                int c = line.indexOf(',');
                tot = line.substring(6, c).toInt();
                cor = line.substring(c + 1).toInt();
              }
            }
            rf.close();
            float acc = tot > 0 ? (100.0 * cor / tot) : 0.0;
            if (!first) j += ",";
            first = false;
            j += "{\"f\":\"" + jsonEsc(nm) + "\",\"time\":\"" + jsonEsc(time) +
                 "\",\"cls\":\"" + jsonEsc(cls) + "\",\"paper\":\"" + jsonEsc(ppr) +
                 "\",\"mode\":" + String(mode) + ",\"q\":" + String(qn) +
                 ",\"acc\":" + String(acc, 1) + "}";
            cnt++;
          }
        }
      }
      ff = root.openNextFile();
    }
  }
  j += "]";
  return j;
}
