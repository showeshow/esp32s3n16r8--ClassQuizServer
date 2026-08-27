#pragma once
#include <Arduino.h>
#include "config.h"

// 考试阶段：首页 → 题目页 → 提交统计页 → … → 考试结束页
enum Phase : uint8_t { PH_HOME = 0, PH_QUESTION, PH_COLLECT, PH_END };

// 一道题（单选题）
struct PaperQ {
  String text, a, b, c, d;
  char ans = 'A';   // 'A'..'D'
};

// 一道题的统计结果
struct QStat {
  int total = 0;         // 计入统计的份数
  int correct = 0;       // 正确份数
  int c[4] = {0, 0, 0, 0};  // A/B/C/D 人数
  bool finalized = false;
};

// 一场考试（全部状态保存在内存，归档时写入 LittleFS）
class Exam {
 public:
  Phase phase = PH_HOME;
  uint8_t mode = MODE_ONCE;     // 0 一答计一 / 1 多答计一 / 2 多答计多
  int qCount = 0;
  int qIndex = 0;               // 0-based 当前题号
  bool statsShown = false;      // 当前题是否已"结束并统计"（统计后不再收答案）
  String className, paperName;

  PaperQ qs[MAX_QUESTIONS];
  QStat stats[MAX_QUESTIONS];

  // 当前题的提交记录（以学生 IP 识别）
  String subIp[MAX_SUBS];
  uint8_t subCh[MAX_SUBS];
  int subCount = 0;   // 计入统计的记录数
  int rawCount = 0;   // 实际收到的提交份数（含"多答计一"的重复提交）

  void resetSubs() {
    for (int i = 0; i < subCount; i++) subIp[i] = String();
    subCount = 0;
    rawCount = 0;
    statsShown = false;
  }

  void clearAll() {
    qCount = 0;
    qIndex = 0;
    phase = PH_HOME;
    statsShown = false;
    className = String();
    paperName = String();
    mode = MODE_ONCE;
    resetSubs();
    for (int i = 0; i < MAX_QUESTIONS; i++) { stats[i] = QStat(); qs[i] = PaperQ(); }
  }

  int findSub(const String& ip) {
    for (int i = 0; i < subCount; i++)
      if (subIp[i] == ip) return i;
    return -1;
  }

  // 学生提交答案（ch: 'A'..'D'）
  // 返回：0 成功  1 重复提交(一答计一)  2 当前不在收题/已统计  3 已达上限
  int submit(const String& ip, char ch) {
    if (phase != PH_COLLECT || statsShown) return 2;
    uint8_t c = (uint8_t)(ch - 'A');
    if (mode == MODE_ONCE) {          // 一答计一：每人只能提交一次
      if (findSub(ip) >= 0) return 1;
      if (subCount >= MAX_SUBS) return 3;
      subIp[subCount] = ip; subCh[subCount] = c; subCount++; rawCount++;
      return 0;
    }
    if (mode == MODE_LAST) {          // 多答计一：最后一次提交才被记录统计
      int i = findSub(ip);
      if (i >= 0) { subCh[i] = c; rawCount++; return 0; }
      if (subCount >= MAX_SUBS) return 3;
      subIp[subCount] = ip; subCh[subCount] = c; subCount++; rawCount++;
      return 0;
    }
    // 多答计多：多次提交多次统计
    if (subCount >= MAX_SUBS) return 3;
    subIp[subCount] = String(); subCh[subCount] = c; subCount++; rawCount++;
    return 0;
  }

  // 结算当前题（若尚未结算）
  void finalizeCurrent() {
    if (qIndex < 0 || qIndex >= qCount) return;
    QStat& st = stats[qIndex];
    if (st.finalized) return;
    for (int i = 0; i < subCount; i++) {
      st.c[subCh[i]]++;
      st.total++;
      if (subCh[i] == (uint8_t)(qs[qIndex].ans - 'A')) st.correct++;
    }
    st.finalized = true;
  }
};
