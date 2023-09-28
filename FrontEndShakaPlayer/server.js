
var socketio = require('socket.io');
const http = require('http');
var fs = require('fs'); 
const hostname = '127.0.0.1';
const port = 3000;

const express = require('express');
const app = express();


var io = null;
app.get('/', function(req, res) {
  fs.readFile('./index.html', function(error, html){
	if(error) throw error;
	res.statusCode = 200;
  	res.setHeader('Content-Type', 'text/html');
  	res.write(html);
	res.end();
	if(io!==null)
               io.sockets.emit('l', 'content');
  });
});

app.get('/myapp.js', function(req,res){
	fs.readFile('./myapp.js', function(error, jsFile){
		if(error) throw error;
		res.writeHeader(200, {"Content-Type": "text/javascript"});
		res.write(jsFile);
		res.end();
	});
});

//format of this type of url will be http://127.0.0.1:3000/play?q=foo+bar
app.get('/playNew', function(req, res){

	if(io!==null)
               io.sockets.emit('playNew', req.query.q);
	console.log(req.query.q);
        res.end(0);

});

app.get('/play', function(req, res){

	if(io!==null)
               io.sockets.emit('play', '');

        res.end(0);

});

app.get('/pause', function(req, res){

	if(io!==null)
               io.sockets.emit('pause', '');

        res.end(0);

});


var server = http.createServer(app);


server.listen(port, hostname, () => {
  		console.log(`Server running at http://${hostname}:${port}/`);
	});



io = require('socket.io')(server);
