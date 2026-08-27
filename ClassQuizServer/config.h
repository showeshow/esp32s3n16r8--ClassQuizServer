#pragma once

// ================= WiFi 配置 =================
// 方式一（推荐，适合大班级）：ESP32 接入教室路由器（STA 模式）。
//   学生手机 / 教师电脑 / 大屏电脑连同一个路由器 WiFi，
//   在串口监视器中查看 ESP32 获得的 IP（也可用 http://classquiz.local）。
//   注意：路由器需关闭"AP 隔离/客户端隔离"，否则设备间无法互访。
// 方式二（免路由器）：STA_SSID 留空 ""，ESP32 自建热点（SoftAP）。
//   学生连接热点 ClassQuiz（密码 12345678），访问 http://192.168.4.1
//   注意：SoftAP 模式最多约 8 台设备同时在线，仅适合小规模演示。
#define STA_SSID ""          // 教室路由器 WiFi 名称，留空则使用自建热点
#define STA_PASS ""          // 教室路由器 WiFi 密码

#define AP_SSID   "ClassQuiz"
#define AP_PASS   "12345678"
#define AP_CHANNEL 6

// ================= 容量限制 =================
#define MAX_QUESTIONS  60     // 每份试卷最多题数
#define MAX_SUBS       1000   // 每题最大提交记录数（含"多答计多"的重复提交）
#define MAX_FIELD_LEN  600    // 题干/选项字段最大字节数
#define MAX_NAME_LEN   64     // 班级/试卷名称最大长度

// 计分方式
#define MODE_ONCE  0   // 一答计一：每人只能提交一次
#define MODE_LAST  1   // 多答计一：可多次提交，最后一次才被记录统计
#define MODE_MULTI 2   // 多答计多：可多次提交，多次提交多次统计
