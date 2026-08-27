#pragma once

// ================== 教师工作台页面 ==================
// 班级栏：新建班级
// 题库栏：上传题目和答案（供教室大屏统计端展示题目和答案）
// 统计栏：按日期和班级归档答案准确率
static const char PAGE_TEACHER[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="zh"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>教师工作台</title>
<style>
body{font-family:"Microsoft YaHei",sans-serif;margin:0;background:#f0f2f5;color:#222}
header{background:#1f3a5f;color:#fff;padding:12px 22px;display:flex;align-items:center;gap:18px;flex-wrap:wrap}
header h1{margin:0;font-size:21px}
#timebox{font-size:13px;color:#cdd8ec}
#timebox input{font-size:13px;padding:4px}
#addr{font-size:13px;color:#9fb4d8;margin-top:3px}
nav{margin-left:auto;display:flex;gap:6px}
nav button{font-size:16px;padding:9px 24px;border:none;border-radius:6px;background:#33507e;color:#fff;cursor:pointer}
nav button.on{background:#fff;color:#1f3a5f;font-weight:bold}
section{display:none;padding:22px;max-width:1050px;margin:auto}
section.on{display:block}
.card{background:#fff;border-radius:10px;padding:18px 20px;margin-bottom:18px;box-shadow:0 1px 4px rgba(0,0,0,.12)}
.card h3{margin:0 0 14px}
input,select,textarea{font-size:15px;padding:8px;border:1px solid #bbb;border-radius:6px;font-family:inherit}
textarea{width:100%}
button.act{font-size:15px;padding:9px 22px;border:none;border-radius:6px;background:#1f3a5f;color:#fff;cursor:pointer}
button.danger{background:#e53935}
table{border-collapse:collapse;width:100%;margin-top:10px}
th,td{border:1px solid #ddd;padding:8px 12px;font-size:14px;text-align:center}
th{background:#eef2f8}
.qcard{border:1px solid #d5dcea;border-radius:8px;padding:12px 14px;margin:12px 0;background:#fafbfd}
.qcard .row{display:flex;gap:10px;margin-top:10px;flex-wrap:wrap;align-items:center}
.qcard input.opt{flex:1;min-width:180px}
.tip{color:#888;font-size:13px}
</style></head><body>
<header>
 <h1>教师工作台</h1>
 <div>
  <div id="timebox">设备时间：--</div>
  <div id="addr"></div>
 </div>
 <div style="display:flex;gap:6px;align-items:center">
  <input type="datetime-local" id="dt">
  <button class="act" onclick="setTime()">设置时间</button>
 </div>
 <nav>
  <button id="tabcls" class="on" onclick="tab('cls')">班级</button>
  <button id="tabpaper" onclick="tab('paper')">题库</button>
  <button id="tabstat" onclick="tab('stat')">统计</button>
 </nav>
</header>

<section id="sec-cls" class="on">
 <div class="card">
  <h3>班级栏 · 新建班级</h3>
  <input id="clsname" placeholder="输入班级名称，如：三年二班">
  <button class="act" onclick="addClass()">新建班级</button>
  <p class="tip">班级将出现在教室大屏统计端首页的班级选择框中。</p>
 </div>
 <div class="card"><h3>已有班级</h3><div id="clslist">加载中...</div></div>
</section>

<section id="sec-paper">
 <div class="card">
  <h3>题库栏 · 用 TXT 上传题目和答案</h3>
  <p class="tip">TXT 格式：题目一行，四个选项各占一行，再一行【答案】X（X 为 A/B/C/D）。多题连续排列，可上传 txt 文件，也可直接粘贴内容。</p>
  <div class="qcard"><b>格式示例</b>
<pre style="background:#fff;border:1px solid #e3e8f2;border-radius:6px;padding:10px;margin:8px 0 0;font-size:13px;white-space:pre-wrap">中国的首都是哪里？
北京
上海
广州
深圳
【答案】A
一年有几个季节？
二个
三个
四个
五个
【答案】C</pre>
  </div>
  <div style="margin-top:12px">
   试卷名称：<input id="pprname" placeholder="如：期中数学测验" style="width:260px">
  </div>
  <div style="margin-top:10px">
   <input type="file" id="txtfile" accept=".txt,text/plain" onchange="readTxt(this)">
  </div>
  <textarea id="txt" rows="12" style="margin-top:10px" placeholder="上传 txt 文件后自动填入，或把内容粘贴到这里"></textarea>
  <div style="margin-top:10px">
   <button class="act" onclick="parsePreview()">解析预览</button>
   <button class="act" onclick="savePaper()">保存试卷</button>
  </div>
  <div id="qpreview" class="tip" style="margin-top:10px"></div>
 </div>
 <div class="card"><h3>已有试卷</h3><div id="pprlist">加载中...</div></div>
</section>

<section id="sec-stat">
 <div class="card">
  <h3>统计栏 · 答案准确率归档（按日期和班级）</h3>
  日期：<select id="fdate"></select>　班级：<select id="fcls"></select>
  <button class="act" onclick="renderRecords()">查询</button>
  <div id="rectable">加载中...</div>
 </div>
 <div class="card"><h3>归档详情</h3><div id="recdetail" class="tip">点击上方“查看”显示某次考试的每题准确率与答案分布。</div></div>
</section>

<script>
var qs=[],classes=[],papers=[],records=[];
function $(id){return document.getElementById(id);}
function esc(s){return String(s||"").replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/"/g,"&quot;");}
function getJSON(url,cb){fetch(url).then(function(r){return r.json()}).then(function(j){cb(j)}).catch(function(){cb(null)});}
function post(u,p,cb){
 var b=Object.keys(p).map(function(k){return k+"="+encodeURIComponent(p[k])}).join("&");
 fetch(u,{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:b})
 .then(function(r){return r.text()})
 .then(function(t){cb&&cb(t);refresh();})
 .catch(function(){cb&&cb(null);});
}
function refresh(){
 if($("sec-cls").classList.contains("on"))loadCls();
 if($("sec-paper").classList.contains("on"))loadPprs();
}
function tab(t){
 ["cls","paper","stat"].forEach(function(x){
  $("sec-"+x).classList.toggle("on",x==t);
  $("tab"+x).classList.toggle("on",x==t);
 });
 if(t=="cls")loadCls();
 if(t=="paper")loadPprs();
 if(t=="stat")loadRecords();
}

/* ---------- 班级栏 ---------- */
function loadCls(){getJSON("/api/classes",function(j){if(j){classes=j;renderCls();}});}
function renderCls(){
 var h;
 if(!classes.length){h="<p class='tip'>暂无班级，请先新建。</p>";}
 else{
  h="<table><tr><th>班级</th><th>操作</th></tr>";
  classes.forEach(function(c){h+="<tr><td>"+esc(c)+"</td><td><button class='act danger' onclick=\"delCls('"+c+"')\">删除</button></td></tr>";});
  h+="</table>";
 }
 $("clslist").innerHTML=h;
}
function addClass(){
 var n=$("clsname").value.trim();
 if(!n){alert("请输入班级名称");return;}
 post("/api/classes",{action:"add",name:n},function(t){
  if(t=="OK"){$("clsname").value="";}else{alert(t||"操作失败");}
 });
}
function delCls(n){if(confirm("删除班级《"+n+"》？"))post("/api/classes",{action:"del",name:n});}

/* ---------- 题库栏（TXT 上传） ---------- */
function readTxt(inp){
 var f=inp.files&&inp.files[0];
 if(!f)return;
 loadFile(f,"UTF-8",function(t){
  if(t.indexOf("\uFFFD")>=0){loadFile(f,"GBK",function(t2){fillTxt(t2.length&&t2.indexOf("\uFFFD")<0?t2:t);});}
  else fillTxt(t);
 });
}
function loadFile(f,enc,cb){
 var r=new FileReader();
 r.onload=function(){cb(r.result);};
 r.readAsText(f,enc);
}
function fillTxt(t){
 $("txt").value=t;
 parsePreview();
}
// 解析 TXT：题目一行 + 四个选项各一行 + 【答案】X
function parseTxt(t){
 var lines=t.split(/\r?\n/).map(function(s){return s.trim();}).filter(function(s){return s.length;});
 var out=[],buf=[];
 function stripOpt(s){return s.replace(/^[A-Da-d]\s*[.、．:：]\s*/,"").replace(/^[A-Da-d]\s+/,"");}
 for(var i=0;i<lines.length;i++){
  var L=lines[i];
  var m=L.match(/^[【\[]?\s*答案\s*[】\]]?\s*[:：]?\s*([A-Da-d])/);
  if(m){
   if(buf.length>=5){out.push({t:buf[0],a:stripOpt(buf[1]),b:stripOpt(buf[2]),c:stripOpt(buf[3]),d:stripOpt(buf[4]),ans:m[1].toUpperCase()});}
   buf=[];
  }else{
   buf.push(L);
  }
 }
 if(buf.length>=5){ // 末题缺答案行时默认 A
  out.push({t:buf[0],a:stripOpt(buf[1]),b:stripOpt(buf[2]),c:stripOpt(buf[3]),d:stripOpt(buf[4]),ans:"A"});
 }
 return out;
}
function parsePreview(){
 qs=parseTxt($("txt").value);
 var h;
 if(!qs.length){h="未解析到题目，请检查格式（题目一行、四个选项各一行、【答案】X 一行）。";}
 else{
  h="已解析 <b>"+qs.length+"</b> 道题：<table><tr><th>#</th><th>题干</th><th>答案</th></tr>";
  qs.slice(0,10).forEach(function(q,i){h+="<tr><td>"+(i+1)+"</td><td style='text-align:left'>"+esc(q.t.length>40?q.t.slice(0,40)+"…":q.t)+"</td><td>"+q.ans+"</td></tr>";});
  h+="</table>";
  if(qs.length>10)h+="<p class='tip'>…其余 "+(qs.length-10)+" 题略</p>";
 }
 $("qpreview").innerHTML=h;
}
function savePaper(){
 var name=$("pprname").value.trim();
 if(!name){alert("请输入试卷名称");return;}
 if(!qs.length)parsePreview();
 if(!qs.length){alert("未解析到题目，请检查 TXT 格式");return;}
 for(var i=0;i<qs.length;i++){
  if(!qs[i].t){alert("第"+(i+1)+"题题干为空");return;}
  if(!qs[i].a||!qs[i].b||!qs[i].c||!qs[i].d){alert("第"+(i+1)+"题选项不完整（需四个选项各占一行）");return;}
 }
 var p={action:"add",name:name,qn:qs.length};
 for(var i=0;i<qs.length;i++){
  p["q"+i]=qs[i].t;p["a"+i]=qs[i].a;p["b"+i]=qs[i].b;p["c"+i]=qs[i].c;p["d"+i]=qs[i].d;p["ans"+i]=qs[i].ans;
 }
 post("/api/papers",p,function(t){
  if(t=="OK"){alert("试卷已保存（"+qs.length+" 题）");qs=[];$("pprname").value="";$("txt").value="";$("qpreview").innerHTML="";}
  else alert(t||"保存失败");
 });
}
function loadPprs(){getJSON("/api/papers",function(j){if(j){papers=j;renderPprs();}});}
function renderPprs(){
 var h;
 if(!papers.length){h="<p class='tip'>暂无试卷，请在上方新建。</p>";}
 else{
  h="<table><tr><th>试卷</th><th>操作</th></tr>";
  papers.forEach(function(p){
   h+="<tr><td>"+esc(p)+"</td><td><button class='act' onclick=\"editPpr('"+p+"')\">编辑</button> <button class='act danger' onclick=\"delPpr('"+p+"')\">删除</button></td></tr>";
  });
  h+="</table>";
 }
 $("pprlist").innerHTML=h;
}
function editPpr(n){
 getJSON("/api/paper?name="+encodeURIComponent(n),function(j){
  if(j&&j.length){
   var t=j.map(function(q){return q.t+"\n"+q.a+"\n"+q.b+"\n"+q.c+"\n"+q.d+"\n【答案】"+q.ans;}).join("\n");
   $("pprname").value=n;$("txt").value=t;parsePreview();window.scrollTo(0,0);
  }
 });
}
function delPpr(n){if(confirm("删除试卷《"+n+"》？"))post("/api/papers",{action:"del",name:n});}

/* ---------- 统计栏 ---------- */
function loadRecords(){getJSON("/api/records",function(j){if(j){records=j;fillFilters();renderRecords();}});}
function fillFilters(){
 var ds={},cs={};
 records.forEach(function(r){if(r.time&&r.time.indexOf(" ")>0)ds[r.time.split(" ")[0]]=1;cs[r.cls]=1;});
 $("fdate").innerHTML="<option value=''>全部日期</option>"+Object.keys(ds).sort().reverse().map(function(x){return "<option>"+x+"</option>";}).join("");
 $("fcls").innerHTML="<option value=''>全部班级</option>"+Object.keys(cs).sort().map(function(x){return "<option>"+esc(x)+"</option>";}).join("");
}
function renderRecords(){
 var dv=$("fdate").value||"",cv=$("fcls").value||"";
 var MN=["一答计一","多答计一","多答计多"];
 var list=records.filter(function(r){return (!dv||r.time.indexOf(dv)==0)&&(!cv||r.cls==cv);});
 var h;
 if(!list.length){h="<p class='tip'>暂无符合条件的归档记录。考试在大屏端进入“考试结束”页时自动归档。</p>";}
 else{
  h="<table><tr><th>时间</th><th>班级</th><th>试卷</th><th>计分方式</th><th>题数</th><th>总体准确率</th><th></th></tr>";
  list.forEach(function(r){
   h+="<tr><td>"+esc(r.time)+"</td><td>"+esc(r.cls)+"</td><td>"+esc(r.paper)+"</td><td>"+MN[r.mode]+"</td><td>"+r.q+"</td><td>"+r.acc+"%</td><td><button class='act' onclick=\"viewRec('"+r.f+"')\">查看</button></td></tr>";
  });
  h+="</table>";
 }
 $("rectable").innerHTML=h;
}
function viewRec(f){
 fetch("/api/record?file="+encodeURIComponent(f)).then(function(r){return r.text()}).then(function(t){
  var rows=[],meta={};
  t.split("\n").forEach(function(l){
   l=l.trim();if(!l)return;
   var k=l.split("=")[0];
   if(k=="Q")rows.push(l.substring(2).split(","));
   else if(k=="TIME"||k=="CLASS"||k=="PAPER"||k=="MODE"||k=="TOTAL")meta[k]=l.substring(k.length+1);
  });
  var h="<div>时间："+esc(meta.TIME||"")+"　班级："+esc(meta.CLASS||"")+"　试卷："+esc(meta.PAPER||"")+"</div>";
  h+="<table><tr><th>题号</th><th>正确答案</th><th>收到份数</th><th>正确份数</th><th>准确率</th><th>A</th><th>B</th><th>C</th><th>D</th></tr>";
  rows.forEach(function(p){
   var tot=parseInt(p[2])||0,cor=parseInt(p[3])||0;
   var acc=tot>0?Math.round(cor*1000/tot)/10:0;
   h+="<tr><td>"+p[0]+"</td><td>"+p[1]+"</td><td>"+tot+"</td><td>"+cor+"</td><td>"+acc+"%</td><td>"+p[4]+"</td><td>"+p[5]+"</td><td>"+p[6]+"</td><td>"+p[7]+"</td></tr>";
  });
  h+="</table>";
  $("recdetail").innerHTML=h;
 });
}

/* ---------- 时间 ---------- */
function loadTime(){
 getJSON("/api/time",function(j){
  if(!j)return;
  $("timebox").innerHTML="设备时间："+esc(j.t)+(j.ok?"":" <span style='color:#ffcc66'>（未设置，请设置，否则归档日期不准）</span>");
  if(j.ip)$("addr").textContent="学生: http://"+j.ip+"/  大屏: http://"+j.ip+"/screen";
 });
}
function setTime(){
 var v=$("dt").value;
 if(!v){alert("请选择日期时间");return;}
 post("/api/settime",{t:v},function(t){
  if(t=="OK"){alert("时间已设置");loadTime();}
  else alert(t||"设置失败");
 });
}

loadCls();loadPprs();loadTime();
</script>
</body></html>
)rawliteral";
