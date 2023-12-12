
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