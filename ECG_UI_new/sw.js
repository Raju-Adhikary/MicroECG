/*
const CACHE_NAME = "offlineServe-v1.0.0";

self.addEventListener("install", (event) => {
    self.skipWaiting();
});

self.addEventListener("activate", (event) => {
    event.waitUntil(
        caches.keys().then((keys) =>
            Promise.all(keys.map((key) => key !== CACHE_NAME ? caches.delete(key) : null))
        )
    );
});

self.addEventListener("fetch", (event) => {
    event.respondWith(
        caches.match(event.request).then((cached) =>
            cached ||
            fetch(event.request)
                .then((response) => {
                    return caches.open(CACHE_NAME).then((cache) => {
                        cache.put(event.request, response.clone());
                        return response;
                    });
                })
                .catch(() => new Response("You are offline. Please check your connection.", {
                    status: 200,
                    headers: { "Content-Type": "text/plain" }
                }))
        )
    );
});
//*/

//Ready to upload - minified
const CACHE_NAME='offlineServe-v1.0.0';self.addEventListener('install',e=>{self.skipWaiting()}),self.addEventListener('activate',e=>{e.waitUntil(caches.keys().then(e=>Promise.all(e.map(e=>e!==CACHE_NAME?caches.delete(e):null))))}),self.addEventListener('fetch',n=>{n.respondWith(caches.match(n.request).then(e=>e||fetch(n.request).then(t=>caches.open(CACHE_NAME).then(e=>(e.put(n.request,t.clone()),t))).catch(()=>new Response('You are offline. Please check your connection.',{status:200,headers:{'Content-Type':'text/plain'}}))))});