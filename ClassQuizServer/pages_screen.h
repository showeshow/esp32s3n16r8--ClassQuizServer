#pragma once

// ============ 教室大屏统计端页面 ============
// 结构：首页 + 第一题页 + 提交统计页 + 第二题页 + 提交统计页 + … + 考试结束页
// 由 /api/state 驱动（每 0.8s 轮询），控制指令发送到 /api/control。
static const char PAGE_SCREEN[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="zh"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>教室大屏统计端</title>
<style>
*{box-sizing:border-box}
:root{--fs:1}
html,body{height:100%}
body{margin:0;font-family:"Microsoft YaHei",sans-serif;background:#fff;color:#222;display:flex;flex-direction:column;overflow:hidden}
#main{flex:1;display:flex;flex-direction:column;align-items:center;justify-content:center;padding:16px;overflow:auto}
#foot{display:flex;gap:24px;padding:18px;justify-content:center;flex-wrap:wrap}
.btn{font-size:28px;padding:16px 48px;border:none;border-radius:10px;cursor:pointer;background:#40527a;color:#fff}
.btn.green{background:#2e7d32}
.modebtn{display:block;width:460px;max-width:90vw;margin:12px auto;font-size:30px;padding:18px;border-radius:10px;border:3px solid #98a6bf;background:#f2f4f8;color:#333;cursor:pointer;font-weight:bold}
.modebtn.on{background:#e53935;color:#fff;border-color:#b71c1c}
select{font-size:28px;padding:10px;border-radius:8px;max-width:38vw}
.hint{font-size:34px;font-weight:bold;margin-bottom:24px;color:#1f3a5f}
.qwrap{width:88%;max-width:1400px}
.qnum{font-size:30px;color:#888;margin-bottom:14px;text-align:center}
.qtitle{font-size:calc(44px*var(--fs));font-weight:bold;line-height:1.3;white-space:pre-wrap;text-align:center}
.opts{margin-top:20px;font-size:calc(38px*var(--fs));line-height:1.5}
.ctitle{font-size:52px;font-weight:bold}
.csub{font-size:36px;color:#555;margin-top:12px}
.grid{display:flex;gap:26px;margin-top:30px;width:94%;max-width:1500px;align-items:stretch}
.box{border:3px solid #333;border-radius:14px;display:flex;flex-direction:column;align-items:center;justify-content:center;padding:8px}
#chartbox{flex:5;height:42vh}
#ansbox{flex:2;height:42vh}
#accbox{flex:2;height:42vh}
.lbl{font-size:24px;color:#999}
#ans{font-size:110px;font-weight:bold;color:#e53935}
#acc{font-size:80px;font-weight:bold;color:#1f3a5f}
.bars{flex:1;display:flex;align-items:flex-end;justify-content:center;gap:56px;width:100%;padding:6px 30px 0}
.bcol{display:flex;flex-direction:column;align-items:center;width:110px;height:100%;justify-content:flex-end}
.bnum{font-size:30px;font-weight:bold;margin-bottom:4px}
.bar{width:100%;border-radius:8px 8px 0 0}
.blab{font-size:32px;font-weight:bold;margin-top:8px}
#endtxt{font-size:110px;font-weight:bold;letter-spacing:12px}
/* 题目页：底部透明悬浮，不挡题目；右下角 下一页，其上方靠右 + / - 字体键 */
.fzlayer{position:fixed;left:0;right:0;bottom:0;z-index:20;pointer-events:none}
.fzlayer>*{pointer-events:auto}
.qnext{position:fixed;right:40px;bottom:28px;font-size:32px;padding:22px 72px}
.qa{position:fixed;right:40px;bottom:128px;display:flex;gap:14px}
.fzbtn{width:84px;height:84px;font-size:46px;font-weight:bold;border:none;border-radius:14px;background:#40527a;color:#fff;cursor:pointer}
</style></head><body>
<div id="main"></div>
<div id="foot"></div>
<script>
var MODES=["一答计一","多答计一","多答计多"];
var mode=0,classes=[],papers=[],state={phase:"loading"},lastKey="";
function $(id){return document.getElementById(id);}
function esc(s){return String(s||"").replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/"/g,"&quot;");}
function CN(n){var d="零一二三四五六七八九";if(n>0&&n<10)return d[n];if(n>=10&&n<20)return "十"+(n%10?d[n%10]:"");if(n>=20&&n<100)return d[Math.floor(n/10)]+"十"+(n%10?d[n%10]:"");return ""+n;}
function ctrl(a,extra){
 var body="action="+a;extra=extra||{};
 for(var k in extra)body+="&"+k+"="+encodeURIComponent(extra[k]);
 fetch("/api/control",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:body}).catch(function(){}).then(poll,poll);
}
function poll(){
 fetch("/api/state").then(function(r){return r.json()}).then(function(s){state=s;render();}).catch(function(){});
}
function loadLists(){
 fetch("/api/classes").then(function(r){return r.json()}).then(function(j){classes=j||[];if(state.phase=="home")build();});
 fetch("/api/papers").then(function(r){return r.json()}).then(function(j){papers=j||[];if(state.phase=="home")build();});
}
function render(){
 var key=state.phase+"|"+state.q+"|"+(state.statsShown?1:0)+"|"+state.qcount;
 if(key!=lastKey){lastKey=key;build();}
 update();
}
function build(){
 var m=$("main"),f=$("foot");
 m.innerHTML="";f.innerHTML="";
 if(state.phase=="home"){
  m.style.justifyContent="center";m.style.paddingTop="16px";
  f.style.display="flex";f.style.justifyContent="center";
  var h="";
  for(var i=0;i<3;i++)h+="<button class='modebtn"+(mode==i?" on":"")+"' onclick='setMode("+i+")'>"+MODES[i]+"</button>";
  h+="<button class='modebtn' style='background:#1f3a5f;color:#fff;border-color:#1f3a5f' onclick='startExam()'>下一页</button>";
  m.innerHTML=h;
  var cs="<select id='selcls'><option value=''>选择班级</option>";
  classes.forEach(function(c){cs+="<option>"+esc(c)+"</option>";});
  cs+="</select>";
  var ps="<select id='selppr'><option value=''>选择试卷</option>";
  papers.forEach(function(p){ps+="<option>"+esc(p)+"</option>";});
  ps+="</select>";
  f.innerHTML="<div style='font-size:26px;display:flex;align-items:center;gap:8px'>班级 "+cs+"</div>"+
              "<div style='font-size:26px;display:flex;align-items:center;gap:8px'>试卷 "+ps+"</div>";
 }else if(state.phase=="question"){
  m.style.justifyContent="center";m.style.paddingTop="16px";m.style.paddingBottom="120px";
  f.style.display="block";f.style.background="none";f.style.padding="0";
  m.innerHTML="<div class='qwrap'><div class='qnum'>第 "+state.q+" 题 / 共 "+state.qcount+" 题</div>"+
   "<div class='qtitle'>"+esc(state.qtext)+"</div>"+
   "<div class='opts'>"+(state.opt||[]).map(function(o,i){return "<div><b>"+"ABCD"[i]+" . </b>"+esc(o)+"</div>";}).join("")+"</div></div>";
  f.innerHTML="<div class='qa'>"+
   "<button class='fzbtn' onclick='chgFs(0.1)'>+</button>"+
   "<button class='fzbtn' onclick='chgFs(-0.1)'>-</button></div>"+
   "<button class='btn green qnext' onclick='ctrl(\"collect\")'>下一页</button>";
 }else if(state.phase=="collect"){
  m.style.justifyContent="flex-start";m.style.paddingTop="30px";
  f.style.display="flex";f.style.justifyContent="center";f.style.padding="18px";
  m.innerHTML="<div class='ctitle'>第"+CN(state.q)+"题</div>"+
   "<div class='csub' id='csub'>正在答题...已收到 0 份</div>"+
   "<div class='grid'>"+
    "<div class='box' id='chartbox'><div class='lbl' id='chartlbl'>统计图（点击“结束并统计”后显示）</div><div class='bars' id='bars'></div></div>"+
    "<div class='box' id='ansbox'><div class='lbl'>答案</div><div id='ans'></div></div>"+
    "<div class='box' id='accbox'><div class='lbl'>准确率</div><div id='acc'></div></div>"+
   "</div>";
  f.innerHTML="<button class='btn' style='background:#c62828' onclick='ctrl(\"home\")'>返回首页</button>"+
   "<button class='btn' onclick='ctrl(\"stats\")'>结束并统计</button>"+
   "<button class='btn green' onclick='ctrl(\"next\")'>下一题</button>";
  update();
 }else if(state.phase=="end"){
  m.style.justifyContent="center";m.style.paddingTop="16px";
  f.style.display="flex";f.style.justifyContent="center";
  m.innerHTML="<div id='endtxt'>考试结束</div>";
  f.innerHTML="<button class='btn' onclick='ctrl(\"home\")'>返回首页</button>";
 }
}
function update(){
 if(state.phase=="collect"){
  var el=$("csub");if(el)el.textContent="正在答题...已收到 "+(state.received||0)+" 份";
  if(state.statsShown){
   var cols=["#e53935","#fb8c00","#43a047","#1e88e5"];
   var c=state.counts||[0,0,0,0];
   var mx=Math.max(c[0],c[1],c[2],c[3],1);
   var h="";
   for(var i=0;i<4;i++)h+="<div class='bcol'><div class='bnum'>"+c[i]+"</div><div class='bar' style='height:"+Math.round(c[i]*88/mx)+"%;background:"+cols[i]+"'></div><div class='blab'>"+"ABCD"[i]+"</div></div>";
   $("bars").innerHTML=h;
   var l=$("chartlbl");if(l)l.textContent="人数统计";
   $("ans").textContent=state.ans||"";
   $("acc").textContent=(state.accuracy!=null?state.accuracy+"%":"");
  }
 }
}
function setMode(i){mode=i;build();}
function chgFs(d){var r=document.documentElement;var v=parseFloat(getComputedStyle(r).getPropertyValue("--fs"))||1;v=Math.max(0.6,Math.min(2.2,v+d));r.style.setProperty("--fs",v);}
function startExam(){
 var c=$("selcls")?$("selcls").value:"",p=$("selppr")?$("selppr").value:"";
 if(!c){alert("请选择班级");return;}
 if(!p){alert("请选择试卷");return;}
 ctrl("start",{class:c,paper:p,mode:mode});
}
loadLists();
poll();
setInterval(poll,1500);
</script>
</body></html>
)rawliteral";
