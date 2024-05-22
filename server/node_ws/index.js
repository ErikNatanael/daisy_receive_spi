const WebSocket = require('ws');
const server = new WebSocket.Server({
  host: "0.0.0.0",
  port: 8080
});

let sockets = [];
server.on('connection', function(socket) {
  console.log('Connection from ' + socket);
  sockets.push(socket);

  // When you receive a message, send that message to every socket.
  socket.on('message', function(msg) {
    console.log('Received \"' + msg + "\"");
    sockets.forEach(s => s.send(msg));

  });

  // When a socket closes, or disconnects, remove it from the array.
  socket.on('close', function() {
    console.log('Socket closed');
    sockets = sockets.filter(s => s !== socket);
  });

  socket.on('error', console.error);
});

server.on('error', console.error);