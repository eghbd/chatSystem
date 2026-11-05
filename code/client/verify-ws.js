const crypto = require('crypto');
const http = require('http');
const net = require('net');

function varint(value) {
  const bytes = [];
  while (value > 0x7f) {
    bytes.push((value & 0x7f) | 0x80);
    value >>>= 7;
  }
  bytes.push(value);
  return Buffer.from(bytes);
}

function fieldString(fieldNumber, value) {
  const data = Buffer.from(value);
  return Buffer.concat([varint((fieldNumber << 3) | 2), varint(data.length), data]);
}

function fieldMessage(fieldNumber, value) {
  return Buffer.concat([varint((fieldNumber << 3) | 2), varint(value.length), value]);
}

function makeFrame(payload) {
  const mask = crypto.randomBytes(4);
  let header;
  if (payload.length < 126) {
    header = Buffer.from([0x82, 0x80 | payload.length]);
  } else if (payload.length <= 0xffff) {
    header = Buffer.alloc(4);
    header[0] = 0x82;
    header[1] = 0x80 | 126;
    header.writeUInt16BE(payload.length, 2);
  } else {
    throw new Error('test payload is too large');
  }

  const masked = Buffer.alloc(payload.length);
  for (let i = 0; i < payload.length; ++i) {
    masked[i] = payload[i] ^ mask[i % 4];
  }
  return Buffer.concat([header, mask, masked]);
}

const key = crypto.randomBytes(16).toString('base64');
const socket = net.connect(8001, '127.0.0.1');
let buffer = Buffer.alloc(0);
let upgraded = false;
let finished = false;

function finish(ok, message) {
  if (finished) return;
  finished = true;
  console.log(message);
  socket.destroy();
  process.exit(ok ? 0 : 1);
}

function postMessage(content) {
  const stringMessage = fieldString(1, content);
  const messageContent = fieldMessage(2, stringMessage);
  const request = Buffer.concat([
    fieldString(1, 'Rverify'),
    fieldString(3, 'sess'),
    fieldString(4, '2000'),
    fieldMessage(5, messageContent),
  ]);

  const response = http.request({
    host: '127.0.0.1',
    port: 8000,
    path: '/service/message_transmit/new_message',
    method: 'POST',
    headers: {
      'Content-Type': 'application/x-protobuf',
      'Content-Length': request.length,
    },
  }, (res) => {
    res.resume();
    res.on('end', () => console.log(`HTTP ${res.statusCode}`));
  });

  response.on('error', (error) => finish(false, `HTTP failed: ${error.message}`));
  response.end(request);
}

function consumeFrames() {
  while (buffer.length >= 2) {
    const second = buffer[1];
    let offset = 2;
    let length = second & 0x7f;
    if (length === 126) {
      if (buffer.length < offset + 2) return;
      length = buffer.readUInt16BE(offset);
      offset += 2;
    } else if (length === 127) {
      if (buffer.length < offset + 8) return;
      length = Number(buffer.readBigUInt64BE(offset));
      offset += 8;
    }

    if (second & 0x80) {
      if (buffer.length < offset + 4) return;
      offset += 4;
    }
    if (buffer.length < offset + length) return;

    const opcode = buffer[0] & 0x0f;
    const payload = buffer.subarray(offset, offset + length);
    buffer = buffer.subarray(offset + length);

    if (opcode === 2 && payload.includes(Buffer.from('send-test'))) {
      finish(true, `Received WebSocket push, bytes=${payload.length}`);
    }
  }
}

socket.on('connect', () => {
  socket.write([
    'GET /ws HTTP/1.1',
    'Host: 127.0.0.1:8001',
    'Upgrade: websocket',
    'Connection: Upgrade',
    `Sec-WebSocket-Key: ${key}`,
    'Sec-WebSocket-Version: 13',
    '',
    '',
  ].join('\r\n'));
});

socket.on('data', (chunk) => {
  buffer = Buffer.concat([buffer, chunk]);

  if (!upgraded) {
    const headerEnd = buffer.indexOf('\r\n\r\n');
    if (headerEnd === -1) return;
    const header = buffer.subarray(0, headerEnd).toString();
    if (!header.startsWith('HTTP/1.1 101')) {
      finish(false, `WebSocket upgrade failed: ${header}`);
      return;
    }
    upgraded = true;
    buffer = buffer.subarray(headerEnd + 4);
    socket.write(makeFrame(Buffer.from('auth')));
    postMessage('send-test');
  }

  consumeFrames();
});

socket.on('error', (error) => finish(false, `WebSocket failed: ${error.message}`));

setTimeout(() => {
  finish(false, upgraded ? 'Timed out waiting for WebSocket push' : 'Timed out during WebSocket handshake');
}, 5000);
