/*
 * ============================================================
 *  ESP32-S3-N16R8 课堂答题系统 (ClassQuizServer)
 * ============================================================
 * 局域网 HTTP 网络服务器，一套三个 Web 端：
 *   /        学生端：纯 HTTP，不出现题目，A/B/C/D 选项按键 + 提交按键
 *   /screen  教室大屏统计端：首页 → 题目页 → 提交统计页 → … → 考试结束页
 *   /teacher 教师工作台：班级栏（新建班级）/ 题库栏（上传题目和答案）
 *                        /统计栏（按日期和班级归档答案准确率）
 * 分数（每题答案分布、准确率）在考试结束时自动归档到 LittleFS。
 *
 * 网络：
 *   - config.h 配置了路由器账号 → 接入教室局域网（推荐，支持全班学生）
 *   - 未配置 → 自建热点 ClassQuiz / 12345678，访问 192.168.4.1
 *     （SoftAP 最多约 8 台设备同时在线，仅适合小规模演示）
 *
 * 硬件：ESP32-S3-N16R8 开发板（16MB Flash + 8MB PSRAM），无需外接器件。
 */
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <nvs_flash.h>
#include "config.h"
#include "util.h"
#include "exam.h"
#include "storage.h"
#include "pages_student.h"
#include "pages_screen.h"
#include "pages_teacher.h"

WebServer server(80);
Exam exam;
String hostIp = "192.168.4.1";

// ==================== NVS 自愈 ====================
// WiFi 驱动依赖 NVS；若 NVS 分区数据损坏（错误 4353 / ESP_ERR_NVS_NOT_INITIALIZED），
// 先擦除再重建，否则 esp_wifi_init 会失败、热点无法开启。
void nvsHeal() {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_OK) return;
  Serial.printf("[NVS] 初始化失败(0x%X)，擦除后重建...\n", err);
  nvs_flash_erase();
  err = nvs_flash_init();
  Serial.printf("[NVS] 重建结果: 0x%X (%s)\n", err, err == ESP_OK ? "成功" : "仍失败");
}

// ==================== WiFi ====================
void wifiSetup() {
  WiFi.persistent(false);
  if (strlen(STA_SSID) > 0) {
    Serial.printf("[WiFi] 正在连接路由器 %s ", STA_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(STA_SSID, STA_PASS);
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000UL) {
      delay(250);
      Serial.print(".");
    }
    Serial.println();
  }
  if (WiFi.status() == WL_CONNECTED) {
    hostIp = WiFi.localIP().toString();
    Serial.printf("[WiFi] 已接入教室局域网，访问: http://%s/\n", hostIp.c_str());
    // 有互联网时可自动校时（中国时区）
    configTime(8 * 3600, 0, "ntp.aliyun.com", "ntp.tencent.com", "pool.ntp.org");
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL, 0, 8);
    hostIp = WiFi.softAPIP().toString();
    Serial.printf("[WiFi] 自建热点: SSID=%s 密码=%s\n", AP_SSID, AP_PASS);
    Serial.printf("[WiFi] 连接该热点后访问: http://%s/\n", hostIp.c_str());
  }
}

// ==================== 页面 ====================
void handleRoot()      { server.send(200, "text/html", PAGE_STUDENT); }
void handleScreen()    { server.send(200, "text/html", PAGE_SCREEN); }
void handleTeacher()   { server.send(200, "text/html", PAGE_TEACHER); }

// ==================== 学生端提交 ====================
void apiAnswer() {
  server.client().setNoDelay(true);   // 关闭 Nagle，消除小响应 ~200ms 延迟
  String ch = server.arg("choice");
  String ip = server.client().remoteIP().toString();
  const char* msg = "已发送成功";
  if (ch.length() != 1 || ch[0] < 'A' || ch[0] > 'D') {
    msg = "无效答案";
  } else {
    int r = exam.submit(ip, ch[0]);
    if (r == 1)      msg = "每人只能提交一次";
    else if (r == 2) msg = (exam.phase == PH_COLLECT) ? "本题统计已结束" : "当前不在答题时间";
    else if (r == 3) msg = "提交已达上限";
  }
  server.send(200, "text/plain", msg);
}

// ==================== 大屏端状态轮询 ====================
void apiState() {
  String s;
  s.reserve(1400);
  s += "{\"phase\":\"";
  switch (exam.phase) {
    case PH_HOME:     s += "home";     break;
    case PH_QUESTION: s += "question"; break;
    case PH_COLLECT:  s += "collect";  break;
    case PH_END:      s += "end";      break;
  }
  s += "\",\"q\":";      s += String(exam.qIndex + 1);
  s += ",\"qcount\":";   s += String(exam.qCount);
  s += ",\"received\":"; s += String(exam.rawCount);
  s += ",\"mode\":";     s += String(exam.mode);
  s += ",\"statsShown\":"; s += String(exam.statsShown ? "true" : "false");
  s += ",\"class\":\"";  s += jsonEsc(exam.className);
  s += "\",\"paper\":\""; s += jsonEsc(exam.paperName);
  s += "\"";
  if (exam.phase == PH_QUESTION && exam.qCount > 0) {
    PaperQ& q = exam.qs[exam.qIndex];
    s += ",\"qtext\":\""; s += jsonEsc(q.text); s += "\"";
    s += ",\"opt\":[\""; s += jsonEsc(q.a); s += "\",\"";
    s += jsonEsc(q.b);   s += "\",\""; s += jsonEsc(q.c);
    s += "\",\"";        s += jsonEsc(q.d); s += "\"]";
  }
  if (exam.phase == PH_COLLECT && exam.statsShown) {
    QStat& st = exam.stats[exam.qIndex];
    int acc = st.total > 0 ? (int)(100.0 * st.correct / st.total + 0.5) : 0;
    s += ",\"ans\":\"";  s += String(exam.qs[exam.qIndex].ans); s += "\"";
    s += ",\"counts\":["; s += String(st.c[0]) + "," + String(st.c[1]) + "," +
                               String(st.c[2]) + "," + String(st.c[3]) + "]";
    s += ",\"total\":";  s += String(st.total);
    s += ",\"correct\":"; s += String(st.correct);
    s += ",\"accuracy\":"; s += String(acc);
  }
  s += "}";
  server.send(200, "application/json", s);
}

// ==================== 大屏端控制 ====================
void apiControl() {
  String action = server.arg("action");
  if (action == "start") {
    String cls = sanitize(server.arg("class"));
    String ppr = sanitize(server.arg("paper"));
    int mode = server.arg("mode").toInt();
    if (!cls.length() || !ppr.length()) { server.send(200, "text/plain", "请先选择班级和试卷"); return; }
    if (!loadPaperInto(ppr, exam))      { server.send(200, "text/plain", "试卷不存在"); return; }
    exam.className = cls;
    exam.mode = (mode >= MODE_ONCE && mode <= MODE_MULTI) ? (uint8_t)mode : (uint8_t)MODE_ONCE;
    exam.qIndex = 0;
    exam.phase = PH_QUESTION;   // 进入第一题页面
    Serial.printf("[考试] 开始: 班级=%s 试卷=%s 模式=%d 题数=%d\n",
                  exam.className.c_str(), exam.paperName.c_str(), exam.mode, exam.qCount);
    server.send(200, "text/plain", "OK");
  } else if (action == "collect") {
    // 由题目页点击"下一页"，开始收集学生提交的答案
    if (exam.phase == PH_QUESTION) exam.phase = PH_COLLECT;
    server.send(200, "text/plain", "OK");
  } else if (action == "stats") {
    // "结束并统计"：出柱状图 / 答案 / 准确率，并停止接收本题答案
    if (exam.phase == PH_COLLECT && !exam.statsShown) {
      exam.finalizeCurrent();
      exam.statsShown = true;
    }
    server.send(200, "text/plain", "OK");
  } else if (action == "next") {
    // "下一题"：最后一题则进入考试结束页并归档
    if (exam.phase == PH_COLLECT) {
      exam.finalizeCurrent();
      exam.qIndex++;
      if (exam.qIndex >= exam.qCount) {
        exam.phase = PH_END;
        bool saved = writeRecord(exam);
        Serial.printf("[考试] 结束并归档: %s\n", saved ? "成功" : "失败");
      } else {
        exam.phase = PH_QUESTION;
        exam.resetSubs();
      }
    }
    server.send(200, "text/plain", "OK");
  } else if (action == "home") {
    // "返回首页"：终止本次考试并返回首页（不归档）
    exam.phase = PH_HOME;
    server.send(200, "text/plain", "OK");
  } else {
    server.send(200, "text/plain", "未知操作");
  }
}

// ==================== 教师工作台：班级 ====================
void apiClasses() {
  if (server.method() == HTTP_GET) {
    String lines[64];
    int n = splitLines(getClasses(), lines, 64);
    String j = "[";
    for (int i = 0; i < n; i++) {
      if (i) j += ",";
      j += "\"" + jsonEsc(lines[i]) + "\"";
    }
    j += "]";
    server.send(200, "application/json", j);
    return;
  }
  String action = server.arg("action");
  String name = server.arg("name");
  bool ok = (action == "del") ? delClass(name) : addClass(name);
  server.send(200, "text/plain", ok ? "OK" : "操作失败（名称无效或文件错误）");
}

// ==================== 教师工作台：题库 ====================
void apiPapersGet() { server.send(200, "application/json", listPapersJson()); }

void apiPapersPost() {
  String action = server.arg("action");
  if (action == "del") {
    server.send(200, "text/plain", delPaper(server.arg("name")) ? "OK" : "删除失败");
    return;
  }
  // 新建/覆盖试卷：字段 name, qn, q{i}, a{i}, b{i}, c{i}, d{i}, ans{i}
  String name = sanitize(server.arg("name"));
  int n = server.arg("qn").toInt();
  if (!name.length())          { server.send(200, "text/plain", "试卷名称无效"); return; }
  if (n < 1 || n > MAX_QUESTIONS) { server.send(200, "text/plain", "题目数量无效"); return; }
  PaperQ pqs[MAX_QUESTIONS];
  for (int i = 0; i < n; i++) {
    String pre = String(i);
    pqs[i].text = server.arg("q" + pre);
    pqs[i].a = server.arg("a" + pre);
    pqs[i].b = server.arg("b" + pre);
    pqs[i].c = server.arg("c" + pre);
    pqs[i].d = server.arg("d" + pre);
    String ans = server.arg("ans" + pre);
    ans.trim();
    pqs[i].ans = (ans.length() && ans[0] >= 'A' && ans[0] <= 'D') ? ans[0] : 'A';
  }
  server.send(200, "text/plain", savePaper(name, pqs, n) ? "OK" : "保存失败");
}

void apiPaperDetail() {
  String name = server.arg("name");
  File f = LittleFS.open(paperPath(name), "r");
  if (!f) { server.send(200, "application/json", "[]"); return; }
  f.readStringUntil('\n');  // 跳过首行试卷名
  String j = "[";
  bool first = true;
  String fields[6];
  while (f.available()) {
    String line = f.readStringUntil('\n');
    if (splitQ(line, fields)) {
      if (!first) j += ",";
      first = false;
      j += "{\"t\":\"" + jsonEsc(fields[0]) + "\",\"a\":\"" + jsonEsc(fields[1]) +
           "\",\"b\":\"" + jsonEsc(fields[2]) + "\",\"c\":\"" + jsonEsc(fields[3]) +
           "\",\"d\":\"" + jsonEsc(fields[4]) + "\",\"ans\":\"" + fields[5].substring(0, 1) + "\"}";
    }
  }
  f.close();
  j += "]";
  server.send(200, "application/json", j);
}

// ==================== 教师工作台：统计归档 ====================
void apiRecords() { server.send(200, "application/json", listRecordsJson()); }

void apiRecordDetail() {
  String fn = server.arg("file");
  if (fn.length() < 3 || fn[0] != 'R' || fn.indexOf('/') >= 0 || fn.indexOf('\\') >= 0) {
    server.send(200, "text/plain", "无效文件名");
    return;
  }
  File f = LittleFS.open("/records/" + fn, "r");
  if (!f) { server.send(200, "text/plain", "记录不存在"); return; }
  String body = f.readString();
  f.close();
  server.send(200, "text/plain", body);
}

// ==================== 教师工作台：时间 ====================
void apiTime() {
  String j = "{\"t\":\"" + nowStr() + "\",\"ok\":" + String(timeOK() ? "true" : "false") +
             ",\"ip\":\"" + hostIp + "\"}";
  server.send(200, "application/json", j);
}

void apiSetTime() {
  String t = server.arg("t");  // 格式 2026-08-27T09:30
  int y, mo, d, h, mi;
  if (sscanf(t.c_str(), "%d-%d-%dT%d:%d", &y, &mo, &d, &h, &mi) == 5) {
    struct tm tv = {};
    tv.tm_year = y - 1900;
    tv.tm_mon = mo - 1;
    tv.tm_mday = d;
    tv.tm_hour = h;
    tv.tm_min = mi;
    time_t tt = mktime(&tv);
    struct timeval nowtv;
    nowtv.tv_sec = tt;
    nowtv.tv_usec = 0;
    settimeofday(&nowtv, nullptr);
    Serial.printf("[时间] 教师设置时间: %s\n", nowStr().c_str());
    server.send(200, "text/plain", "OK");
  } else {
    server.send(200, "text/plain", "时间格式错误");
  }
}

// ==================== 启动 ====================
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n[ClassQuiz] ESP32-S3 课堂答题系统启动中...");
  initTZ();
  nvsHeal();   // 必须在 WiFi 初始化之前
  if (!storageInit()) Serial.println("[FS] LittleFS 初始化失败!");

  wifiSetup();
  if (MDNS.begin("classquiz")) MDNS.addService("http", "tcp", 80);

  exam.clearAll();

  server.on("/",            HTTP_GET,  handleRoot);
  server.on("/student",     HTTP_GET,  handleRoot);
  server.on("/screen",      HTTP_GET,  handleScreen);
  server.on("/teacher",     HTTP_GET,  handleTeacher);
  server.on("/api/answer",  HTTP_POST, apiAnswer);
  server.on("/api/state",   HTTP_GET,  apiState);
  server.on("/api/control", HTTP_POST, apiControl);
  server.on("/api/classes", HTTP_GET,  apiClasses);
  server.on("/api/classes", HTTP_POST, apiClasses);
  server.on("/api/papers",  HTTP_GET,  apiPapersGet);
  server.on("/api/papers",  HTTP_POST, apiPapersPost);
  server.on("/api/paper",   HTTP_GET,  apiPaperDetail);
  server.on("/api/records", HTTP_GET,  apiRecords);
  server.on("/api/record",  HTTP_GET,  apiRecordDetail);
  server.on("/api/time",    HTTP_GET,  apiTime);
  server.on("/api/settime", HTTP_POST, apiSetTime);
  server.onNotFound([]() { server.send(404, "text/plain", "404 Not Found"); });
  server.begin();

  Serial.println("[HTTP] 服务器已启动:");
  Serial.printf("  学生答题端:   http://%s/\n", hostIp.c_str());
  Serial.printf("  教室大屏统计: http://%s/screen\n", hostIp.c_str());
  Serial.printf("  教师工作台:   http://%s/teacher\n", hostIp.c_str());
}

void loop() {
  server.handleClient();
  delay(2);
}
