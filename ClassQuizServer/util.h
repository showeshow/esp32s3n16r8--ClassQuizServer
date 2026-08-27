#pragma once
#include <Arduino.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include "config.h"

// ---------- JSON 转义（用于生成 JSON，UTF-8 字节原样保留） ----------
inline String jsonEsc(const String& s) {
  String o;
  o.reserve(s.length() + 8);
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if ((uint8_t)c < 0x20) { o += ' '; }
    else o += c;
  }
  return o;
}

// ---------- 名称清洗（班级名 / 试卷名：去掉换行、分隔符、引号等危险字符） ----------
inline String sanitize(const String& in) {
  String s = in;
  s.trim();
  s.replace('/', '_');  s.replace('\\', '_');
  s.replace('\n', '_'); s.replace('\r', '_');
  s.replace('|', '_');  s.replace('\x1F', '_');
  s.replace('"', '_');  s.replace('\'', '_');
  if (s.length() > MAX_NAME_LEN) s = s.substring(0, MAX_NAME_LEN);
  return s;
}

// ---------- 题干/选项字段清洗（换行转空格，限制长度） ----------
inline String fld(const String& in) {
  String s = in;
  s.replace('\r', ' ');
  s.replace('\n', ' ');
  if (s.length() > MAX_FIELD_LEN) s = s.substring(0, MAX_FIELD_LEN);
  return s;
}

// ---------- 多行文本按 \n 拆分（去除空行与首尾空白） ----------
inline int splitLines(const String& all, String* out, int maxN) {
  int n = 0, idx = 0;
  while (idx < (int)all.length() && n < maxN) {
    int e = all.indexOf('\n', idx);
    if (e < 0) e = all.length();
    String line = all.substring(idx, e);
    line.trim();
    if (line.length()) out[n++] = line;
    idx = e + 1;
  }
  return n;
}

// ---------- 时间工具（中国时区 UTC+8） ----------
inline void initTZ() { setenv("TZ", "CST-8", 1); tzset(); }

inline String nowStr() {
  time_t t = time(nullptr);
  struct tm tv;
  localtime_r(&t, &tv);
  char b[24];
  strftime(b, sizeof(b), "%Y-%m-%d %H:%M", &tv);
  return String(b);
}

inline String fileStamp() {
  time_t t = time(nullptr);
  struct tm tv;
  localtime_r(&t, &tv);
  char b[20];
  strftime(b, sizeof(b), "%Y%m%d_%H%M%S", &tv);
  return String(b);
}

// 时间是否已被设置（晚于 2020-01-01 视为有效）
inline bool timeOK() { return (unsigned long long)time(nullptr) > 1577836800ULL; }
