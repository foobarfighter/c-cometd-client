var Faye   = require('faye')
  , http   = require('http')
  , bayeux = new Faye.NodeAdapter({mount: '/cometd', timeout: 3});

var Inspect = {
  incoming: function (message, callback){
    console.log("incoming message\n================");
    console.log(message);
    console.log();
    return callback(message);
  },

  outgoing: function (message, callback){
    console.log("outgoing message\n================");
    console.log(message);
    console.log();
    return callback(message);
  }
}

var server = http.createServer();

bayeux.addExtension(Inspect);
bayeux.attach(server);
server.listen(8089);

console.log("server started\n");
