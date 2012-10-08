var Faye   = require('faye')
  , server = new Faye.NodeAdapter({mount: '/cometd'});

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

server.addExtension(Inspect);
server.listen(8089);

console.log("server started\n");
