var Faye    = require('faye')
  , http    = require('http')
  , express = require('express')
  , bayeux  = new Faye.NodeAdapter({mount: '/cometd', timeout: 3});

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

var startPort = 8089
  , server = http.createServer();

bayeux.addExtension(Inspect);
bayeux.attach(server);
server.listen(startPort);

var app = express();

app.use(express.logger());
app.post('/bad_json', function (req, res){
  res.setHeader('Content-Type', 'application/json');
  res.send('{ this_is_messed_up_json }');
});

app.post('/long_request', function (req, res, next){
  setTimeout(function (){
    res.setHeader('Content-Type', 'application/json');
    res.send('[]');
  }, 30000);
});

app.get('/heynow', function (req, res, next){
  res.send('hey now');
});

app.listen(startPort+1);

console.log("server started\n");
