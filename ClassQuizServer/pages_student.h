#pragma once

// ================== 学生端页面（纯 HTTP，不出现题目） ==================
// 顶部文字输入框显示所选答案；ABCD 四个选项按键 + 提交按键，各占一行。
static const char PAGE_STUDENT[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="zh"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>学生答题</title>
<style>
body{font-family:"Microsoft YaHei",sans-serif;margin:0 auto;padding:16px 14px;background:#f5f6fa;max-width:480px;min-height:100vh;box-sizing:border-box}
h2{text-align:center;font-size:20px;color:#333}
#box{width:100%;box-sizing:border-box;font-size:24px;padding:16px;text-align:center;border:2px solid #34495e;border-radius:10px;background:#fff;color:#222;margin:14px 0 22px}
button{display:block;width:100%;box-sizing:border-box;font-size:28px;font-weight:bold;padding:20px;margin:12px 0;border:none;border-radius:10px;color:#fff}
.bA{background:#e53935}.bB{background:#fb8c00}.bC{background:#43a047}.bD{background:#1e88e5}
#send{background:#2c3e50;font-size:24px}
</style></head><body>
<h2>课堂答题</h2>
<input id="box" readonly placeholder="请选择答案">
<button class="bA" onclick="pick('A')">A</button>
<button class="bB" onclick="pick('B')">B</button>
<button class="bC" onclick="pick('C')">C</button>
<button class="bD" onclick="pick('D')">D</button>
<button id="send" onclick="send()">提交</button>
<script>
var cur="",b=document.getElementById("box");
function pick(c){cur=c;b.value="答案："+c;b.style.color="#222";}
function send(){
 if(!cur){b.value="请先选择答案";return;}
 b.value="发送中...";b.style.color="#222";
 fetch("/api/answer",{method:"POST",keepalive:true,headers:{"Content-Type":"application/x-www-form-urlencoded"},body:"choice="+cur})
 .then(function(r){return r.text();})
 .then(function(t){b.value=t||"已发送成功";})
 .catch(function(){b.value="发送失败";b.style.color="#e53935";});
}
</script>
</body></html>
)rawliteral";
