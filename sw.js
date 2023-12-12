
self.addEventListener('load', function(){
    Notification.requestPermission().then(function(permission) {
        if (permission === 'granted') {
          console.log('Notification permission granted.');
        } else {
          console.error('Notification permission denied.');
        }
      });
})
// // Trigger a push notification
function triggerPushNotification(text) {
    const options = {
      body: text,
      icon: 'CareFallLogo.png',
    };
  
    navigator.serviceWorker.ready.then(function(registration) {
      registration.showNotification('CareFall', options);
    });
  }
// Handle notification click event
self.addEventListener('notificationclick', function(event) {
    event.notification.close();  // Close the notification
  });
self.addEventListener('install', event => {
  event.waitUntil(
    caches.open('your-cache-name').then(cache => {
      return cache.addAll([
        '/',
        '/index.html',
        '/add-device.html',
        '/device-info.html',
        '/styles.css',
        '/script.js',
        '/CareFallLogo.png'
      ]);
    })
  );
});
  
self.addEventListener('fetch', event => {
  event.respondWith(
    caches.match(event.request).then(response => {
      return response || fetch(event.request);
    })
  );
});
  