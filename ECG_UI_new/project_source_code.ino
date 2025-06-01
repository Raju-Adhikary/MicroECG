/*
 * @file            micro_ecg.ino
 * @author          RAJU ADHIKARY
 * @date            17-08-2024
 * @last_modified   23-02-2025
 * @version         3.8.0
 * @brief           Portable ECG Machine using AD8232 & ESP8266
 * @details         A compact, low-cost ECG device designed for easy cardiac monitoring. It uses AD8232 for signal amplification, ESP8266 for wireless data transfer and computation needs. The system processes and displays ECG data in real time via a web interface. This project simplifies heart health monitoring, making it more accessible and affordable.
 */
 
 
#include <DNSServer.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
// #include <ESP8266mDNS.h>

const byte DNS_PORT = 53;
IPAddress apIP(172, 217, 28, 1);
DNSServer dnsServer;
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);


// ADC buffer and indexing variables
int *adcBuffer = nullptr;    // Dynamic buffer pointer
int sampleIndex = 0;
int sps = 10;                // Default saadcBuffermples per second
unsigned long previousMillis = 0;
const long sendInterval = 300;   // Time interval to send data (333ms)

unsigned long lastSampleTime = 0;  // Time of last sample
long sampleInterval = 1000 / sps;  // Interval between each sample based on SPS


String webServerIndexPage = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><title>Micro ECG</title><style>*{margin:0;padding:0;box-sizing:border-box;font-family:Arial,sans-serif}body{background:#f4f4f9;color:#333}button,input{padding:3px 9px;margin:5px;background-color:#f4f4f9;border:.1px solid #00f;box-shadow:2px 2px 2px gray}button:hover,input:hover{box-shadow:none}.pushButton[active]{background-color:#0f05}.container{max-width:1200px;margin:20px auto;padding:20px;background:#fff;border-radius:10px;box-shadow:0 4px 8px #0000001a}.header{text-align:center;padding:10px 0;border-bottom:2px solid #e0e0e0}.header h1{font-size:2rem;color:#007bff}.snap-container{width:100%;margin:auto}.canvas-container{margin-top:20px;text-align:center;position:sticky;top:0;pointer-events:none;background-color:#fff5}canvas,.snap-container img{border:1px solid #ddd;width:100%;height:100px}.tabs{display:flex;flex-wrap:wrap;gap:10px}.tabs button{margin:0;flex:1 1 auto}.tabs button[active]{box-shadow:none;border:none}.tab-contents>div:not([active]){display:none}.parameters{display:flex;flex-wrap:wrap;gap:20px;padding:20px 0}.parameter{flex:1 1 auto;background:#f9f9f9;padding:10px;border-radius:5px;box-shadow:0 2px 4px #0000001a;text-align:center;min-width:30%}.parameter span{font-size:1rem;color:#555;font-weight:700;white-space:nowrap}.controls,.settings,.flexbox{display:flex;flex-wrap:wrap;justify-content:center;align-items:center;gap:10px}.control,.flexbox div,.setting{flex:1 1 auto;text-align:center;padding:10px;box-shadow:0 2px 4px #0000001a}.ai-assistant{margin:20px 0;background:#e7f5ff;padding:15px;border-radius:8px;text-align:center;box-shadow:0 2px 4px #0000001a}.ai-assistant h2{font-size:1.5rem;color:#00f}.ai-assistant p{font-size:1rem;color:#444;margin-top:10px}.footer{text-align:center;margin-top:20px;font-size:.7rem;color:#333}.danger{color:red!important;border-color:red!important}@media (max-width: 668px){.parameters{flex-direction:column}.flexbox{width:100%}}</style></head><body><noscript>Please Enable JavaScript in your browser...!!</noscript><div class=container><div class=header><h1>Micro ECG ♥(Alpha)</h1></div><div class=snap-container id=snap-container></div><div class=canvas-container><canvas id=ecgCanvas></canvas></div><div class=tabs><button data-id=parameters active>PARAMETERS</button> <button data-id=ai-assistant>AI</button> <button data-id=controls>CONTROLS</button> <button data-id=recordings>RECORDINGS</button> <button data-id=settings disabled>SETTINGS</button></div><div class=tab-contents><div class=parameters active><div class=parameter><span>Result: <span id=param-result>•_°</span></span></div><div class=parameter><span>Heart Rate: <span id=param-hr>•_° </span>bpm</span></div><div class=parameter><span>QRS Duration: <span id=param-qrs>•_° </span>ms</span></div><div class=parameter><span>QT Interval: <span id=param-qt>•_° </span>ms</span></div><div class=parameter><span>PR Interval: <span id=param-pr>•_° </span>ms</span></div><div class=parameter><span>ST Interval: <span id=param-st>•_° </span>ms</span></div></div><div class=ai-assistant><h2>AI Assistant</h2><p id=ai-output></p><button id=ai-assistBtn>⟳ Get Assistance</button></div><div class=controls><div class=control style=width:100%></div><div class='control pushButton'data-toggle=isHold><span>HOLD</span></div><div class='control pushButton'id=recordBtn><span>RECORD</span></div><div class=control onclick='data_plot=[]'><span>CLEAR</span></div><div class=control id=takeSnapBtn><span>SNAP</span></div><div class=control style=width:100%></div><div class=control><span>TIME/DIV</span><br>− <input type=range class='multiturn minZero'data-default=50 max=100 min=0 oninput='scale.x=this.dataset.value/10'value=50> +</div><div class=control><span>VOLT/DIV</span><br>− <input type=range class=multiturn data-default=120 max=100 min=0 oninput='scale.y=this.dataset.value/10'value=50> +</div><div class=control><span>POSITION VERTICAL</span><br>▲ <input type=range class=multiturn data-default=13 max=100 min=0 oninput='move.y=10*this.dataset.value'value=50> ▼</div><div class=control><span>POSITION HORIZONTAL</span><br>⏮ <input type=range class='multiturn minZero'data-default=0 max=100 min=0 oninput='move.x=10*this.dataset.value'value=50> ⏭</div><div class=control style=width:100%></div></div><div class='flexbox recordings'><div class=flexbox style=width:100%><label for=''>IMPORT(.json)</label><input type=file accept=.json id=recordingImport></div><div class=flexbox id=recordings><div class=flexbox><div class=pushButton>NO RECORD FOUND<br><sub>mm:hh dd-mm-yyyy</sub></div><div class=download>⤓</div><div class=download>x</div></div></div></div></div><div class=footer><a href='/test' target='_blank'>Open in External Browser</a><p style=display:block;background-color:#eee><b>Disclaimer:</b> The project is not designed nor certified to diagnose any serious problem. We, the creators, developers, and distributors of this App/Website/device are not liable for any inaccuracies, misinterpretations, or adverse outcomes resulting from its use. Use of the App/Website/device is at your own risk and discretion.</div></div><script type=\"text/javascript\" charset=\"utf-8\">(()=>{document.head.appendChild(Object.assign(document.createElement(\"style\"),{innerHTML:\".notify-container {width: 100%; max-height: 230px; overflow: scroll; display: flex; flex-direction: column;align-items: center; position: fixed; top: 10px; left: 0; pointer-events: none;}.notify-container > div {max-width: 400px; margin: 10px; border-radius: 5px; text-align: center; padding: 10px 20px;background: #eef; color: #00f; position: relative; box-shadow: 0 2px 5px rgba(0, 0, 0, 0.2);transition: transform 0.5s; pointer-events: auto;}.notify-container > div:hover { transform: translateY(10%); }.notifyClose { position: absolute; right: 5px; bottom: 25%; font-weight: bold; cursor: pointer; }\"}));let e=document.body.appendChild(Object.assign(document.createElement(\"div\"),{className:\"notify-container\"}));document.notify=(t=\"\",a,n)=>{let r=Object.assign(document.createElement(\"div\"),{innerText:t});r.appendChild(Object.assign(document.createElement(\"div\"),{className:\"notifyClose\",innerText:\"\\xd7\",onclick:()=>r.remove()})),r.style=({error:\"background:#fdd;color:#f00;\",warning:\"background:#ffc;color:#f00;\",success:\"background:#cfc;color:#080;\"})[a]||\"\",setTimeout(()=>r.remove(),n||200*(e.innerText.length+t.length)),e.appendChild(r)}})();class IDBManager{constructor({dbName:e,dbVersion:t,onUpgrade:a}){this.dbName=e,this.dbVersion=t,this.onUpgrade=a,this.db=null,this.changeEventListeners=[]}open(){return this.db?Promise.resolve(this.db):new Promise((e,t)=>{let a=indexedDB.open(this.dbName,this.dbVersion);a.onerror=()=>t(a.error),a.onblocked=()=>t(Error(\"Database upgrade blocked\")),a.onsuccess=t=>{this.db=t.target.result,e(this.db)},a.onupgradeneeded=e=>{this.db=e.target.result,\"function\"==typeof this.onUpgrade&&this.onUpgrade(this.db,e.oldVersion,e.newVersion,e.target.transaction)}})}async add(e,t){return await this.open(),this.changeEventListener(e),this._executeTransaction(e,\"readwrite\",e=>e.add(t))}async get(e,t){return await this.open(),this._executeTransaction(e,\"readonly\",e=>e.get(t))}async update(e,t,a){return await this.open(),this.changeEventListener(e),this._executeTransaction(e,\"readwrite\",async e=>{(await e.get(t)).onsuccess=t=>{var n=t.target.result;Object.assign(n,a),e.put(n)}})}async delete(e,t){return await this.open(),this.changeEventListener(e),this._executeTransaction(e,\"readwrite\",e=>e.delete(t))}async getAll(e){return await this.open(),this._executeTransaction(e,\"readonly\",e=>e.getAll())}async clear(e){return await this.open(),this.changeEventListener(e),this._executeTransaction(e,\"readwrite\",e=>e.clear())}changeEventListener(e){this.changeEventListeners.forEach(t=>t.storeName==e&&t.action())}addChangeEventListener(e,t){this.changeEventListeners.push({action:e,storeName:t})}async _executeTransaction(e,t,a){return new Promise((n,r)=>{let i=this.db.transaction(e,t),s=i.objectStore(e),d=a(s);d.onerror=()=>r(d.error),i.oncomplete=e=>{n(d.result)},i.onerror=()=>r(i.error)})}close(){this.db&&(this.db.close(),this.db=null)}}async function updateRecordingUI(){let e=await dbManager.getAll(\"metadata\"),t=document.getElementById(\"recordings\");t.innerText=e.length?\"\":\"NO DATA FOUND!!\",e.forEach(e=>{let a=document.createElement(\"div\");a.className=\"flexbox\";let n=document.createElement(\"div\");n.innerHTML=`${\"snapshot\"==e.type?\"&#128248;\":\"\"} ${e.name} <sub> [${e.totalChunks||\"#\"}]</sub> <br /><sub>${e.startTime}</sub>`,n.addEventListener(\"click\",async t=>{if(\"snapshot\"==e.type){let a=document.createElement(\"img\");a.src=e.snapURL,a.addEventListener(\"click\",a.remove),document.getElementById(\"snap-container\").appendChild(a)}if(\"dataStream\"==e.type){isHold=!0,data_plot=[];let n=e.dataChunkId;for(;n;){let r=await dbManager.get(\"dataChunks\",n);if(!r)break;data_plot.push(...r.data),n=r.nextChunkId||null}}});let r=document.createElement(\"div\");r.className=\"danger\",r.innerHTML=\"x\",r.addEventListener(\"click\",async()=>{if(isRecording)return document.notify(\"can't delete while recording!!\",\"error\"),!1;dbManager.delete(\"metadata\",e.id);let t=e.dataChunkId;for(;t;){let a=await dbManager.get(\"dataChunks\",t);if(!a)break;dbManager.delete(\"dataChunks\",t),t=a.nextChunkId||null}});let i=document.createElement(\"div\");i.innerHTML=\"&#x2913;\",i.addEventListener(\"click\",()=>{exportData(e.id)}),a.appendChild(n),a.appendChild(i),a.appendChild(r),t.appendChild(a)})}document.getElementById(\"recordingImport\").addEventListener(\"change\",async e=>{let t=new FileReader;t.onload=async e=>{let{metadata:t,chunks:a}=JSON.parse(e.target.result);if(delete t.id,t.dataChunkId){var n=a.find(e=>e.id==t.dataChunkId);delete n.id,t.dataChunkId=await dbManager.add(\"dataChunks\",n)}await dbManager.add(\"metadata\",t)},t.readAsText(e.target.files[0])});var ECGinterpreter=function(e,t){var a,n={min:Math.min(...e),max:Math.max(...e),sps:t,hr:[],R:[],PQ:[],QR:[],RS:[],ST:[]};n.R=(e=>{let t=n.min,a=(n.max-t)*.8,r=[],i=-1/0,s=-1;for(let d in e)e[d]>t+a?e[d]>i&&(i=e[d],s=d):-1!==s&&(r.push(Number(s)),i=-1/0,s=-1);return -1!==s&&r.push(Number(s)),r})(e),n.hr=Math.round((n.R.length-1)*t*60/(n.R.at(-1)-n.R[0]));for(var r=0;r<n.R.length-1;r++){var i=e.slice(n.R[r],n.R[r+1]),s=i.indexOf(Math.min(...i));n.RS.push(s);var d=i.slice(s,s+i.length/2),o=d.indexOf(Math.max(...d));n.ST.push(o);var l=i.slice(s+i.length/2,-1),c=0;for(let h=l.length-1;h>=0&&l.at(h)-l.at(h-1)>=0;h--)c=l.length-h;n.QR.push(c);var u=l.slice(0,-c),p=u.length-u.indexOf(Math.max(...u));n.PQ.push(p)}return(e=>{for(var t in e)document.querySelector(t).innerText=(\"number\"==typeof e[t]?Math.round(e[t]):e[t])||\"•_\\xb0\"})({\"#param-result\":(a=n).hr<5?\"Rest In Peace \\uD83D\\uDD4A️\":a.hr<60?\"Abnormal: Bradycardia\":a.hr>300?\"Invalid Data...!!\":a.hr>100?\"Abnormal: Tachycardia\":\"Normal ECG\",\"#param-hr\":n.hr,\"#param-qrs\":(n.QR[0]+n.RS[0])*1e3/t,\"#param-qt\":(n.QR[0]+n.RS[0]+n.ST[0])*1e3/t,\"#param-pr\":(n.PQ[0]+n.QR[0])*1e3/t,\"#param-st\":1e3*n.ST[0]/t}),n};(()=>{var e=!1;let t=\"user_01JMF7KR0AJGRW7PJSA947E0JZ\",a=\"https://chat.botpress.cloud/49f49b18-dc9a-4e9e-abbc-a106f9190a04\",n=document.getElementById(\"ai-output\"),r={accept:\"application/json\",\"x-user-key\":\"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpZCI6InVzZXJfMDFKTUY3S1IwQUpHUlc3UEpTQTk0N0UwSloiLCJpYXQiOjE3Mzk5NzMwNTh9.CTDxkazNNM4NJk7sF2Vbg8xro53h61Z1IyDeMc9leKg\",\"content-type\":\"application/json\"};async function i(){try{e=!0;let i=(await fetch(a+\"/conversations/\"+t+\"/listen\",{method:\"GET\",headers:r})).body.getReader(),s=new TextDecoder;for(;;){let{value:d,done:o}=await i.read();if(o)break;s.decode(d)?.match(/\\{.*\\}/)?.forEach(e=>{let t=JSON.parse(e).data;t&&t.payload.text&&t.isBot&&(n.innerHTML=t.payload.text.replace(/\\*\\*(.*?)\\*\\*/g,\"<b>$1</b>\"))})}}catch(l){e=!1,document.notify(\"AI Assistant is not responding...!!\",\"error\")}}document.getElementById(\"ai-assistBtn\").addEventListener(\"click\",async()=>{var s;n.innerText=\"In progress...!!\",await fetch(a+\"/messages\",{method:\"POST\",headers:r,body:JSON.stringify({payload:{type:\"text\",text:ECGResult.hr>300?\"Invalid data!!\":`User's heart rate is ${ECGResult.hr} BPM. Detected ECG parameters:\\nP-Q duration = ${1e3*ECGResult.PQ[0]/ECGResult.sps} ms\\nQ-R duration = ${1e3*ECGResult.QR[0]/ECGResult.sps} ms\\nR-S duration = ${1e3*ECGResult.RS[0]/ECGResult.sps} ms\\nS-T duration = ${1e3*ECGResult.ST[0]/ECGResult.sps} ms`},conversationId:t})}).catch(e=>[document.notify(\"Failed to connect AI Assistant...!!\",\"error\"),n.innerText=\"NO Internet Connection!!\"]),e||i()})})();var data_plot=Array(1600).fill(2400);document.querySelectorAll(\".pushButton\").forEach(e=>{e.addEventListener(\"click\",t=>{e.hasAttribute(\"active\")?e.removeAttribute(\"active\"):e.setAttribute(\"active\",\"\"),window[e.dataset.toggle]=e.hasAttribute(\"active\")},!0)}),document.querySelectorAll(\".multiturn\").forEach(e=>{let t=Number(e.dataset.default);var a=Number(e.max),n=Number(e.min);e.addEventListener(\"change\",()=>{e.value<=n&&!(e.classList.contains(\"minZero\")&&t<=0)&&(e.value=a/2,t-=a/2),e.value>=a&&(e.value=a/2,t+=a/2)}),e.addEventListener(\"input\",()=>e.classList.contains(\"minZero\")&&t+Number(e.value-a/2)<=0?0:void(e.dataset.value=t+Number(e.value-a/2)))}),(()=>{let e=document.createElement(\"div\");e.style=\"position:fixed;top:0;left:0;width:100%;height:100%;background:#0005;display:flex;justify-content:center;align-items:center;visibility:hidden;z-index:1000;\";let t=document.createElement(\"div\");t.style=\"background:white;padding:20px;border-radius:5px;text-align:center;box-shadow:0 2px 5px rgba(0,0,0,0.3);width:250px;\";let a=document.createElement(\"p\"),n=document.createElement(\"input\");n.type=\"text\",n.style=\"display:none;margin-bottom:15px;padding:5px;width:100%;\";let r=document.createElement(\"button\");r.textContent=\"OK\",r.style=\"background:#007bff;color:white;\";let i=document.createElement(\"button\");i.textContent=\"Cancel\",i.style=\"background:#dc3545;color:white;display:none;\",t.append(a,n,r,i),e.appendChild(t),document.body.appendChild(e),window.alert=t=>{a.textContent=t,n.style.display=\"none\",i.style.display=\"none\",r.onclick=()=>e.style.visibility=\"hidden\",e.style.visibility=\"visible\"},window.prompt=(t,s=e=>!1,d=e=>!1)=>{a.textContent=t,n.style.display=\"block\",i.style.display=\"inline-block\",n.value=\"\",r.onclick=()=>{e.style.visibility=\"hidden\",s(n.value)},i.onclick=()=>{e.style.visibility=\"hidden\",d()},e.style.visibility=\"visible\"}})();var recordingQue={count:0,data:[]};async function feedData(e){isHold||data_plot.push(...e),isRecording&&(recordingQue.data.push(...e),recordingQue.count++,recordingQue.count%10||(logData(recordingQue.data),recordingQue.data=[]))}const wSocket=new WebSocket(\"ws://\"+location.hostname+\":81\");wSocket.binaryType=\"arraybuffer\",wSocket.onopen=function(){document.notify(\"WebSocket: Connection established!\",\"success\"),wSocket.send(300)},wSocket.onmessage=function(e){var t=new Uint16Array(e.data);let a=[];for(let n=0;n<t.length;n+=2){var r=t[n]|t[n+1]<<8;a.push(1.4*r+1600)}feedData(a,300)},wSocket.onerror=function(e){document.notify(\"WebSocket: Error \"+e,\"error\")},wSocket.onclose=function(){document.notify(\"WebSocket: Connection closed\",\"warning\")};var graphCanvas=document.getElementById(\"ecgCanvas\"),ctx=graphCanvas.getContext(\"2d\"),scale={x:5,y:12},move={x:0,y:130},isHold=!1,isRecording=!1,recording=[],ECGResult=[];function render(){ctx.clearRect(0,0,graphCanvas.width,graphCanvas.height);var e=(e=data_plot.slice(-graphCanvas.width*scale.x-move.x,-1-move.x)).map(e=>e/scale.y-move.y);ECGResult=ECGinterpreter(e,300),ctx.beginPath(),ctx.moveTo(...a(0,e[0]));for(var t=0;t<e.length;t++)ctx.lineTo(...a(t/scale.x,e[t]));function a(e,t){return[e,graphCanvas.height-t]}ctx.strokeStyle=\"red\",ctx.stroke(),requestAnimationFrame(render)}graphCanvas.width=graphCanvas.offsetParent.offsetWidth,addEventListener(\"resize\",()=>{graphCanvas.width=graphCanvas.offsetParent.offsetWidth}),\"serviceWorker\"in navigator&&navigator.serviceWorker.register(\"./sw.js\").catch(console.log),setTimeout(render,100),(()=>{let e=document.querySelectorAll(\".tabs>button\"),t=document.querySelectorAll(\".tab-contents>div\");e.forEach(a=>{a.addEventListener(\"click\",()=>{e.forEach(e=>e.removeAttribute(\"active\")),t.forEach(e=>e.removeAttribute(\"active\")),a.setAttribute(\"active\",\"\"),document.querySelector(\".\"+a.dataset.id).setAttribute(\"active\",\"\")})})})(),document.getElementById(\"takeSnapBtn\").addEventListener(\"click\",()=>{let e=document.createElement(\"img\");e.src=graphCanvas.toDataURL(\"image/png\"),e.addEventListener(\"click\",e.remove),document.getElementById(\"snap-container\").appendChild(e),prompt(\"Save SNAPSHOT As:\",t=>{dbManager.add(\"metadata\",{name:t||\"SNAPSHOT\",startTime:(new Date).toISOString(),snapURL:e.src,type:\"snapshot\"})})});const dbManager=new IDBManager({dbName:\"EcgRecordings\",dbVersion:1,onUpgrade(e,t,a,n){e.objectStoreNames.contains(\"metadata\")||e.createObjectStore(\"metadata\",{keyPath:\"id\",autoIncrement:!0}),e.objectStoreNames.contains(\"dataChunks\")||e.createObjectStore(\"dataChunks\",{keyPath:\"id\",autoIncrement:!0})}});var dbVariables={chunkNo:0,metadataID:null,prevChunkID:null};async function startRecording(e){dbVariables.metadataID=await dbManager.add(\"metadata\",{name:e||\"RECORD\",startTime:(new Date).toISOString(),endTime:null,dataChunkId:null,type:\"dataStream\"}),dbVariables.chunkNo=0,dbVariables.prevChunkID=null,isRecording=!0}async function logData(e){dbVariables.chunkNo++,e=await dbManager.add(\"dataChunks\",{data:e,chunkNo:dbVariables.chunkNo}),1==dbVariables.chunkNo?await dbManager.update(\"metadata\",dbVariables.metadataID,{dataChunkId:e}):await dbManager.update(\"dataChunks\",dbVariables.prevChunkID,{nextChunkId:e}),await dbManager.update(\"metadata\",dbVariables.metadataID,{totalChunks:dbVariables.chunkNo,endTime:(new Date).toISOString()}),dbVariables.prevChunkID=e}async function exportData(e){let t=await dbManager.get(\"metadata\",e),a=[],n=t.dataChunkId;for(;n;){var r=await dbManager.get(\"dataChunks\",n);if(!r)break;0<a.length?a[0].data.push(...r.data):a.push(r),n=r.nextChunkId||null}n&&(t.totalChunks=1,delete a[0].nextChunkId),e=new Blob([JSON.stringify({metadata:t,chunks:a})],{type:\"application/json\"});let i=Object.assign(document.createElement(\"a\"),{href:URL.createObjectURL(e),download:\"ECGREC-\"+t.name+t.startTime+\".json\"});i.click()}document.getElementById(\"recordBtn\").addEventListener(\"click\",e=>{let t=e.currentTarget;isRecording=!1,t.hasAttribute(\"active\")&&prompt(\"Save RECORDING As:\",startRecording,()=>t.removeAttribute(\"active\"))}),updateRecordingUI(),dbManager.addChangeEventListener(updateRecordingUI,\"metadata\");alert('If the page does not open in your browser, please open it in your browser.  172.217.28.1')</script></body></html> ";
String webServerSW = "const CACHE_NAME='offlineServe-v1.0.0';self.addEventListener('install',e=>{self.skipWaiting()}),self.addEventListener('activate',e=>{e.waitUntil(caches.keys().then(e=>Promise.all(e.map(e=>e!==CACHE_NAME?caches.delete(e):null))))}),self.addEventListener('fetch',n=>{n.respondWith(caches.match(n.request).then(e=>e||fetch(n.request).then(t=>caches.open(CACHE_NAME).then(e=>(e.put(n.request,t.clone()),t))).catch(()=>new Response('You are offline. Please check your connection.',{status:200,headers:{'Content-Type':'text/plain'}}))))});";

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {

    switch (type) {
        case WStype_DISCONNECTED:
        Serial.printf("[%u] Disconnected!\n", num);
        break;
        case WStype_CONNECTED:
        {
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        }
        break;
        case WStype_TEXT:
        Serial.printf("[%u] get Text: %s\n", num, payload);

        //int arrayLen = payload - 0;
        int newSps = atoi((const char*)payload);

        // Try to parse the samples per second (SPS) from the message
        if (newSps > 0 && newSps != sps) {
            // Adjust the number of samples per second if needed
            sps = newSps;

            // Reallocate the ADC buffer with the new size
            if (adcBuffer != nullptr) {
                free(adcBuffer); // Free old buffer
            }
            adcBuffer = (int *)malloc(sps * sizeof(int)); // Allocate new buffer

            // Update sampling interval based on SPS
            sampleInterval = 1000 / sps;

            Serial.print("New SPS: ");
            Serial.println(sps);
        }

        break;
    }
}

void setup() {
    Serial.begin(115200);

    // Serial.setDebugOutput(true);

    Serial.println();
    Serial.println();
    Serial.println();

    for (uint8_t t = 4; t > 0; t--) {
        Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
        Serial.flush();
        delay(1000);
    }

    adcBuffer = (int *)malloc(sps * sizeof(int));

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("Micro_ECG_RANMPGroupF");
    // start webSocket server
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    // if (MDNS.begin("esp8266")) {
    //   Serial.println("MDNS responder started");
    // }
    dnsServer.start(DNS_PORT, "*", apIP);
    
    // handle index
    server.on("/", []() {
        server.send(200, "text/html", webServerIndexPage);
    });
    // handle Service Worker
    server.on("/sw.js", []() {
        server.send(200, "application/javascript", webServerSW);
    });
    // for testing
    server.on("/test", []() {
        server.sendHeader("Location", "https://github.io/", true);
        server.send(302, "text/plane", "Redirecting...");
    });
    // handle 404
    server.onNotFound([]() {
        server.sendHeader("Location", "/", true); //Redirect to our home page
        server.send(301, "text/plane", "");
    });
    
    server.begin();
    
    // // Add service to MDNS
    // MDNS.addService("http", "tcp", 80);
    // MDNS.addService("ws", "tcp", 81);
    
}

unsigned long last_10sec = 0;
unsigned int counter = 0;

void loop() {
    unsigned long t = millis();
    webSocket.loop();
    dnsServer.processNextRequest();
    server.handleClient();

    // if ((t - last_10sec) > 10 * 1000) {
    //     counter++;
    //     bool ping = (counter % 2);
    //     int i = webSocket.connectedClients(ping);
    //     Serial.printf("%d Connected websocket clients ping: %d\n", i, ping);
    //     last_10sec = millis();
    // }

    // Check if it's time to sample ADC data based on SPS
    unsigned long currentMillis = millis();

    if (currentMillis - lastSampleTime >= sampleInterval) {
        // Read ADC and store it in buffer
        adcBuffer[sampleIndex] = analogRead(A0);
        sampleIndex++;
        // Reset buffer and send data every 333ms
        if (sampleIndex >= sps) {
            Serial.printf("147................................");
            sendSamplesThroughWebSocket(adcBuffer, sps);
            sampleIndex = 0; // Reset buffer index
        }

        lastSampleTime = currentMillis;
    }
    // //Send data every 333ms
    if (currentMillis - previousMillis >= sendInterval) {
        if (sampleIndex > 0) {
            sendSamplesThroughWebSocket(adcBuffer, sampleIndex);
            sampleIndex = 0; // Reset buffer index
        }
        previousMillis = currentMillis;
    }
}

// Function to send ADC samples via WebSocket
void sendSamplesThroughWebSocket(int *samples, int count) {
  // Create a buffer to hold raw data (binary format)
  uint8_t buffer[count * sizeof(int)];
  memcpy(buffer, samples, count * sizeof(int));  // Copy ADC samples to buffer

  // Send the buffer as a binary message over WebSocket
  webSocket.broadcastBIN(buffer, count * sizeof(int));
}
